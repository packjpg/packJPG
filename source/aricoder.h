#ifndef ARICODER_H
#define ARICODER_H

#include <algorithm>
#include <cstdint>
#include <vector>
#include "reader.h"
#include "writer.h"

constexpr uint32_t CODER_USE_BITS = 31;
constexpr uint32_t CODER_LIMIT100 = uint32_t(1 << CODER_USE_BITS);
constexpr uint32_t CODER_LIMIT025 = CODER_LIMIT100 / 4;
constexpr uint32_t CODER_LIMIT050 = (CODER_LIMIT100 / 4) * 2;
constexpr uint32_t CODER_LIMIT075 = (CODER_LIMIT100 / 4) * 3;
constexpr uint32_t CODER_MAXSCALE = CODER_LIMIT025 - 1;
constexpr uint32_t ESCAPE_SYMBOL = CODER_LIMIT025;

struct Symbol {
	uint32_t low_count;
	uint32_t high_count;
	uint32_t scale;
};

// The table type associated with BinaryModel, contains the information needed for one context.
struct BinaryTable {
	// counts for each symbol contained in the table
	std::vector<std::uint16_t> counts;
	// links to higher order contexts
	std::vector<BinaryTable*> links;
	// accumulated counts
	uint32_t scale = uint32_t(0);

	// Deletes this and all tables linked by this table.
	~BinaryTable() {
		for (auto& link : links) {
			if (link != nullptr) {
				delete link;
			}
		}
	}

	// Creates the count of symbols if it has not already been created.
	void create_counts_if_empty() {
		// check if counts are available
		if (counts.empty()) {
			// setup counts for current table
			counts.resize(2, std::uint16_t(1));
			// set scale
			scale = uint32_t(2);
		}
	}

	// Resizes the table by rightshifting each symbol count by one.
	void rescale_table() {
		// Do nothing if counts is not set:
		if (!counts.empty()) {
			// Scale the table by bitshifting each count, be careful not to set any count zero:
			counts[0] = std::max(std::uint16_t(1), std::uint16_t(counts[0] >> 1));
			counts[1] = std::max(std::uint16_t(1), std::uint16_t(counts[1] >> 1));
			scale = counts[0] + counts[1];
		}
	}

	// Rescales this and all linked tables.
	void recursive_flush() {
		for (auto& link : links) {
			if (link != nullptr) {
				link->recursive_flush();
			}
		}
		// rescale specific table
		rescale_table();
	}
};

// The table associated with UniversalModel, implemented for a fast totalize_table,
// and contains the information needed for one context.
struct UniversalTable {
	// counts for each symbol contained in the table
	std::vector<std::uint16_t> counts;
	// links to higher order contexts
	std::vector<UniversalTable*> links;
	// speedup info
	std::uint16_t max_count = std::uint16_t(0);
	std::uint16_t max_symbol = std::uint16_t(0);

	// Deletes this and all tables linked by this table.
	~UniversalTable() {
		for (auto& link : links) {
			if (link != nullptr) {
				delete link;
			}
		}
	}

	// Resizes the table by rightshifting each symbol count by one.
	void rescale_table() {
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

	// Rescales this and all linked tables.
	void recursive_flush() {
		for (auto& link : links) {
			if (link != nullptr) {
				link->recursive_flush();
			}
		}

		// rescale specific table
		rescale_table();
	}
};

// An abstract statistical model for arithmetic coding.
class Model {
public:
	Model(int max_c, int max_o, int c_lim) :
		max_context(max_c),
		max_order(max_o + 1),
		max_count(c_lim) {

	}
	virtual ~Model() {}

	virtual void shift_context(int c) = 0;

	// Base case for shifting an arbitrary number of contexts into the model.
	void shift_model() {}

	// Shift an arbitrary number of contexts into the model (at most max_c contexts).
	template <typename C, typename... Cargs>
	void shift_model(C context, Cargs ... contextList) {
		shift_context(context);
		shift_model(contextList...);
	}

protected:
	const int max_context;
	const int max_order;
	const int max_count;
};

/*
A universal statistical model for arithmetic coding.

Boundaries of this model:
Maximum number of symbols (max_s): [1, 1024]
Maximum number of contexts (max_c): [1, 1024]
Maximum order (max_o): [-1, 4]
Maximum count for a symbol (c_lim): [2, 4096]

WARNING: this can be memory intensive, so don't overdo it

For example, max_s == 256; max_c == 256; max_o == 4 would be way too much.
*/
class UniversalModel : public Model
{
public:

	UniversalModel(int max_s, int max_c, int max_o, int c_lim = 255);
	~UniversalModel();

	/*
	1. Updates statistics for a specific valid symbol. Note
	that only contexts that were actually used to encode
	the symbol have their count updated.
	2. Resets to the highest order (use an invalid symbol,
	such as -1, if you want to reset without updating).
	This will undo symbol exclusions (see exclude_symbols).
	*/
	void update_model(int symbol);

	/*
	Shifts in one context (there can be at most max_c number of contexts).
	If max_o is less than one or the provided context is negative,
	this method does nothing.
	*/
	void shift_context(int c) override;

	/*
	Rescales all the contexts in the model.
	*/
	void flush_model();

	/*
	Excludes every symbol above c. Note that update_model undoes symbol exclusions.
	*/
	void exclude_symbols(int c);

	/*
	Converts a Symbol to an int, used only in encoding.
	It returns 0 (the escape symbol) if symbol does not
	exists in the current context, and one otherwise.
	This sets the Symbol low_count, high_count, and scale.
	*/
	int convert_int_to_symbol(int c, Symbol* s);

	/*
	Returns the scale of the current context, used only in decoding.
	*/
	void get_symbol_scale(Symbol* s);

	/*
	* Converts a count to an int, called after get_symbol_scale.
	* 1. Finds the symbol that matches the count,
	* 2. Sets the low- and high-count for the symbol (it has to be removed from the stream).
	*/
	int convert_symbol_to_int(uint32_t count, Symbol* s);

private:
	/*
	1. Totals are calculated by accumulating counts in the current table
	(symbol exclusions are used).
	2. Escape probability is calculated here.

	Note that the accumulated counts must never exceed CODER_MAXSCALE,
	but as CODER_MAXSCALE is 2^29, this shouldn't be an issue, and so
	is not explicitly checked.
	*/
	inline void totalize_table(UniversalTable* context);

	const int max_symbol;

	int current_order;
	int sb0_count;

	std::vector<uint32_t> totals;
	bool* scoreboard;
	std::vector<UniversalTable*> contexts;
};


/*
A binary statistical model for arithmetic coding.

Boundaries of this model
Maximum number of symbols: 2 (0 or 1 )
Maximum number of contexts (max_c): [1, 1024]
Maximum order (max_o): [-1, 4]
*/
class BinaryModel : public Model
{
public:

	BinaryModel(int max_c, int max_o, int c_lim = 255);
	~BinaryModel();

	/*
	* Updates count statistics for a given symbol, rescaling if it takes the symbol's count above
	* the maximum count.
	*/
	void update_model(int symbol);

	/*
	Shifts in one context (there can be at most max_c number of contexts).
	If max_o is less than one or the provided context is negative,
	this method does nothing.
	*/
	void shift_context(int c) override;

	/*
	Rescales all the contexts in the model.
	*/
	void flush_model();

	/*
	* Converts an int to a symbol, used for encoding.
	*/
	int convert_int_to_symbol(int c, Symbol* s);

	/*
	* Returns the scale (sum of counts) of the current context, used for decoding.
	*/
	void get_symbol_scale(Symbol* s);

	/*
	* Converts a count to an int, called after get_symbol_scale.
	*/
	int convert_symbol_to_int(uint32_t count, Symbol* s);

private:

	std::vector<BinaryTable*> contexts;
};

class ArithmeticEncoder {
public:
	ArithmeticEncoder(Writer& stream);
	~ArithmeticEncoder();

	// Generic UniversalModel encoding function.
	void encode(UniversalModel& model, int c) {
		Symbol s;
		int esc;

		do {
			esc = model.convert_int_to_symbol(c, &s);
			encode(&s);
		} while (esc);
		model.update_model(c);
	}

	// Generic BinaryModel encoding function.
	void encode(BinaryModel& model, int c) {
		Symbol s;

		model.convert_int_to_symbol(c, &s);
		encode(&s);
		model.update_model(c);
	}

	/*
	 * Returns whether an error occurred in the writer backing the encoder.
	 */
	bool error() const {
		return sptr.error();
	}

private:
	// Encodes the sybol.
	void encode(const Symbol* s);

	template<std::uint8_t bit>
	void write_bit() {
		// add bit at last position
		bbyte = (bbyte << 1) | bit;
		// increment bit position
		cbit++;

		// write bit if done
		if (cbit == 8) {
			sptr.write_byte(bbyte);
			cbit = 0;
		}
	}

	void writeNrbitsAsZero();
	void writeNrbitsAsOne();

	// io variables:
	Writer& sptr; // Pointer to iostream for writing.
	std::uint8_t bbyte = 0;
	std::uint8_t cbit = 0;

	// Arithmetic coding variables:
	unsigned int clow = 0;
	unsigned int chigh = CODER_LIMIT100 - 1;
	unsigned int cstep = 0;
	unsigned int nrbits = 0;
};

class ArithmeticDecoder {
public:
	ArithmeticDecoder(Reader& stream);
	~ArithmeticDecoder();

	// Generic UniversalModel decoding function.
	int decode(UniversalModel& model) {
		Symbol s;
		uint32_t count;
		int c;

		do {
			model.get_symbol_scale(&s);
			count = decode_count(&s);
			c = model.convert_symbol_to_int(count, &s);
			decode(&s);
		} while (c == ESCAPE_SYMBOL);
		model.update_model(c);

		return c;
	}

	// Generic BinaryModel decoding function.
	int decode(BinaryModel& model)
	{
		Symbol s;

		model.get_symbol_scale(&s);
		uint32_t count = decode_count(&s);
		int c = model.convert_symbol_to_int(count, &s);
		decode(&s);
		model.update_model(c);

		return c;
	}

private:
	void decode(const Symbol* s);
	unsigned int decode_count(const Symbol* s);

	std::uint8_t read_bit();

	// io variables:
	Reader& sptr; // Pointer to iostream for reading.
	std::uint8_t bbyte = 0;
	std::uint8_t cbit = 0;

	// Arithmetic coding variables:
	unsigned int ccode = 0;
	unsigned int clow = 0;
	unsigned int chigh = CODER_LIMIT100 - 1;
	unsigned int cstep = 0;
};

#endif