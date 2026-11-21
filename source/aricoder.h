#ifndef ARICODER_H
#define ARICODER_H

#include "bitops.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

// defines for coder
constexpr uint32_t CODER_USE_BITS = 31; // Must never be above 31.
constexpr uint32_t CODER_LIMIT100 = uint32_t(1) << CODER_USE_BITS;
constexpr uint32_t CODER_LIMIT025 = CODER_LIMIT100 / 4;
constexpr uint32_t CODER_LIMIT050 = (CODER_LIMIT100 / 4) * 2;
constexpr uint32_t CODER_LIMIT075 = (CODER_LIMIT100 / 4) * 3;
constexpr uint32_t CODER_MAXSCALE = CODER_LIMIT025 - 1;
constexpr uint32_t ESCAPE_SYMBOL = CODER_LIMIT025;

// symbol struct, used in arithmetic coding
struct symbol {
	uint32_t low_count;
	uint32_t high_count;
	uint32_t scale;
};

// table struct, used in in statistical models,
// holding all info needed for one context
struct table {
	// counts for each symbol contained in the table
	std::vector<uint16_t> counts;
	// links to higher order contexts
	std::vector<std::unique_ptr<table>> links;
	// accumulated counts
	uint32_t scale = uint32_t(0);

	/* -----------------------------------------------
	Checks if counts exist, creating it if it does not.
	----------------------------------------------- */
	inline void check_counts() {
		// check if counts are available
		if (counts.empty()) {
			// setup counts for current table
			counts.resize(2, uint16_t(1));
			// set scale
			scale = uint32_t(2);
		}
	}

	/* -----------------------------------------------
	Resizes the table by rightshifting each count by 1.
	----------------------------------------------- */
	inline void rescale_table() {
		// Do nothing if counts is not set:
		if (!counts.empty()) {
			// Scale the table by bitshifting each count, be careful not to set any count zero:
			counts[0] = std::max(uint16_t(1), uint16_t(counts[0] >> 1));
			counts[1] = std::max(uint16_t(1), uint16_t(counts[1] >> 1));
			scale = counts[0] + counts[1];
		}
	}

	/* -----------------------------------------------
	Recursively runs rescale_table on this and all linked contexts.
	----------------------------------------------- */
	inline void recursive_flush() {
		for (auto& link : links) {
			if (link != nullptr) {
				link->recursive_flush();
			}
		}
		// rescale specific table
		rescale_table();
	}
};

// special table struct, used in in model_s,
// holding additional info for a speedier 'totalize_table'
struct table_s {
	// counts for each symbol contained in the table
	std::vector<uint16_t> counts;
	// links to higher order contexts
	std::vector<std::unique_ptr<table_s>> links;
	// speedup info
	uint16_t max_count = uint16_t(0);
	uint16_t max_symbol = uint16_t(0);

	/* -----------------------------------------------
	Resizes the table by rightshifting each count by 1.
	----------------------------------------------- */
	inline void rescale_table() {
		// Nothing to do if counts has not been set.
		if (counts.empty()) return;

		// now scale the table by bitshifting each count
		int lst_symbol = max_symbol;
		int i;
		for (i = 0; i < lst_symbol; i++) {
			counts[i] >>= 1; // Counts will not become negative since it is an unsigned type.
		}

		// also rescale tables max count
		max_count >>= 1;

		// seek for new last symbol
		for (i = lst_symbol - 1; i >= 0; i--) {
			if (counts[i] > 0) {
				break;
			}
		}
		max_symbol = i + 1;
	}

	/* -----------------------------------------------
	Recursively runs rescale_table on this and all linked contexts.
	----------------------------------------------- */
	inline void recursive_flush() {
		for (auto& link : links) {
			if (link != nullptr) {
				link->recursive_flush();
			}
		}

		// rescale specific table
		rescale_table();
	}
};

class ArithmeticBitWriter {
public:
	template <std::uint8_t bit>
	inline void write_bit() {
		curr_byte_ = (curr_byte_ << 1) | bit;
		curr_bit_++;
		if (curr_bit_ == 8) {
			data_.emplace_back(curr_byte_);
			curr_bit_ = 0;
		}
	}

	void write_n_zero_bits(std::size_t n);

	void write_n_one_bits(std::size_t n);

	void pad();

	std::vector<std::uint8_t> get_data() const;


private:
	std::vector<std::uint8_t> data_;
	std::uint8_t curr_byte_ = 0;
	std::size_t curr_bit_ = 0;
};


/* -----------------------------------------------
	class for arithmetic coding of data to/from iostream
	----------------------------------------------- */

class ArithmeticEncoder
{
	public:
    explicit ArithmeticEncoder(Writer& writer);
	ArithmeticEncoder(const ArithmeticEncoder&) = delete;
	ArithmeticEncoder& operator=(const ArithmeticEncoder&) = delete;
	~ArithmeticEncoder();
	void encode( symbol* s );

    void finalize();
	
	private:
	
	// i/o variables
    
    bool finalized = false;
    Writer& writer_;
    std::unique_ptr<ArithmeticBitWriter> bitwriter_ = std::make_unique<ArithmeticBitWriter>();
	
	// arithmetic coding variables
	unsigned int clow = 0;
	unsigned int chigh = CODER_LIMIT100 - 1;
	unsigned int cstep = 0;
	unsigned int nrbits = 0;
};

class ArithmeticDecoder {
	public:
    explicit ArithmeticDecoder(Reader& reader);
	~ArithmeticDecoder() = default;
	unsigned int decode_count( symbol* s );
	void decode( symbol* s );
	
	private:
	unsigned char read_bit();
	
	// i/o variables
    Reader& reader_;
	unsigned char bbyte = 0;
	unsigned char cbit = 0;
	std::size_t eof_zero_bytes_ = 0;
	
	// arithmetic coding variables
	unsigned int ccode = 0;
	unsigned int clow = 0;
	unsigned int chigh = CODER_LIMIT100 - 1;
	unsigned int cstep = 0;
};


/* -----------------------------------------------
	universal statistical model for arithmetic coding
	----------------------------------------------- */
	
class model_s
{	
	public:
	
	model_s( int max_s, int max_c, int max_o, int c_lim );
	
	void update_model( int symbol );
	void shift_context( int c );
	void flush_model();
	void exclude_symbols(int c);
	
	int  convert_int_to_symbol( int c, symbol *s );
	void get_symbol_scale( symbol *s );
	int  convert_symbol_to_int(uint32_t count, symbol *s);
	
	private:

	inline void totalize_table(table_s* context);

	const int max_symbol;
	const int max_context;
	const int max_order;
	const int max_count;

	int current_order;
	int sb0_count;
	
	std::vector<uint32_t> totals;
	std::vector<std::uint8_t> scoreboard;
	std::unique_ptr<table_s> null_table_;
	std::unique_ptr<table_s> start_table_;
	std::vector<table_s*> contexts;
};


/* -----------------------------------------------
	binary statistical model for arithmetic coding
	----------------------------------------------- */
	
class model_b
{	
	public:
	
	model_b( int max_c, int max_o, int c_lim );
	
	void update_model( int symbol );
	void shift_context( int c );
	void flush_model();
	
	int  convert_int_to_symbol( int c, symbol *s );
	void get_symbol_scale( symbol *s );
	int  convert_symbol_to_int(uint32_t count, symbol *s);	
	
	private:
	
	const int max_context;
	const int max_order;
	const int max_count;
	
	std::unique_ptr<table> null_table_;
	std::unique_ptr<table> start_table_;
	std::vector<table*> contexts;
};

// Base case for shifting an arbitrary number of contexts into the model.
template <typename M>
inline void shift_model(const M&) {}

// Shift an arbitrary number of contexts into the model (at most max_c contexts).
template <typename M, typename C, typename... Cargs>
inline void shift_model(const M& model, C context, Cargs ... contextList) {
	model->shift_context(context);
	shift_model(model, contextList...);
}

/* -----------------------------------------------
	generic model_s encoder function
	----------------------------------------------- */
inline void encode_ari( ArithmeticEncoder* encoder, model_s* model, int c )
{
	symbol s;
	int esc;
	
	do {		
		esc = model->convert_int_to_symbol( c, &s );
		encoder->encode( &s );
	} while ( esc );
	model->update_model( c );
}

/* -----------------------------------------------
	generic model_s decoder function
	----------------------------------------------- */	
inline int decode_ari( ArithmeticDecoder* decoder, model_s* model )
{
	symbol s;
	uint32_t count;
	int c;
	
	do{
		model->get_symbol_scale( &s );
		count = decoder->decode_count( &s );
		c = model->convert_symbol_to_int( count, &s );
		decoder->decode( &s );	
	} while ( c == int(ESCAPE_SYMBOL) );
	model->update_model( c );
	
	return c;
}

/* -----------------------------------------------
	generic model_b encoder function
	----------------------------------------------- */	
inline void encode_ari( ArithmeticEncoder* encoder, model_b* model, int c )
{
	symbol s;
	
	model->convert_int_to_symbol( c, &s );
	encoder->encode( &s );
	model->update_model( c );
}

/* -----------------------------------------------
	generic model_b decoder function
	----------------------------------------------- */	
inline int decode_ari( ArithmeticDecoder* decoder, model_b* model )
{
	symbol s;
	
	model->get_symbol_scale( &s );
	uint32_t count = decoder->decode_count( &s );
	int c = model->convert_symbol_to_int( count, &s );
	decoder->decode( &s );	
	model->update_model( c );
	
	return c;
}

inline void encode_ari( ArithmeticEncoder* encoder, const std::unique_ptr<model_s>& model, int c )
{
	encode_ari( encoder, model.get(), c );
}

inline int decode_ari( ArithmeticDecoder* decoder, const std::unique_ptr<model_s>& model )
{
	return decode_ari( decoder, model.get() );
}

inline void encode_ari( ArithmeticEncoder* encoder, const std::unique_ptr<model_b>& model, int c )
{
	encode_ari( encoder, model.get(), c );
}

inline int decode_ari( ArithmeticDecoder* decoder, const std::unique_ptr<model_b>& model )
{
	return decode_ari( decoder, model.get() );
}

#endif
