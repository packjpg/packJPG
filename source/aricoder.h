#ifndef ARICODER_H
#define ARICODER_H

#include <algorithm>
#include <cstdint>
#include <vector>

#include "reader.h"
#include "writer.h"
#include "arithmeticbitwriter.h"

constexpr std::uint32_t CODER_USE_BITS = 31;
constexpr std::uint32_t CODER_LIMIT100 = uint32_t(1 << CODER_USE_BITS);
constexpr std::uint32_t CODER_LIMIT025 = CODER_LIMIT100 / 4;
constexpr std::uint32_t CODER_LIMIT050 = (CODER_LIMIT100 / 4) * 2;
constexpr std::uint32_t CODER_LIMIT075 = (CODER_LIMIT100 / 4) * 3;
constexpr std::uint32_t CODER_MAXSCALE = CODER_LIMIT025 - 1;
constexpr std::uint32_t ESCAPE_SYMBOL = CODER_LIMIT025;

class Symbol {
public:
	std::uint32_t low_count;
	std::uint32_t high_count;
	std::uint32_t scale;
};

// The table type associated with BinaryModel, contains the information needed for one context.
class BinaryTable {
public:
	// counts for each symbol contained in the table
	std::vector<std::uint16_t> counts;
	// links to higher order contexts
	std::vector<BinaryTable*> links;
	// accumulated counts
	std::uint32_t scale = std::uint32_t(0);

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
			scale = std::uint32_t(2);
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
class UniversalTable {
public:
	// counts for each symbol contained in the table
	std::vector<std::uint16_t> counts;
	// links to higher order contexts
	std::vector<UniversalTable*> links;
	// speedup info
	std::uint16_t max_symbol_count = std::uint16_t(0);
	std::int32_t max_symbol = 0;

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
		std::int32_t lst_symbol = max_symbol;
		int i;
		for (i = 0; i < lst_symbol; i++) {
			counts[i] >>= 1; // Counts will not become negative since it is an unsigned type.
		}

		// also rescale tables max count
		max_symbol_count >>= 1;

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
	Model(int max_context, int max_order, int max_symbol_count) :
		max_context_(max_context),
		max_order_(max_order + 1),
		max_symbol_count_(max_symbol_count) {

	}
	virtual ~Model() = default;

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
	const int max_context_;
	const int max_order_;
	const int max_symbol_count_;
};

/*
A universal statistical model for arithmetic coding.

Boundaries of this model:
Maximum number of symbols (max_symbol): [1, 1024]
Maximum number of contexts (max_context): [1, 1024]
Maximum order (max_order): [-1, 4]
Maximum count for a symbol (max_symbol_count): [2, 4096]

WARNING: this can be memory intensive, so don't overdo it

For example, max_s == 256; max_c == 256; max_o == 4 would be way too much.
*/
class UniversalModel : public Model
{
public:

	UniversalModel(int max_symbol, int max_context, int max_order, int max_symbol_count = 255);
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
	void exclude_symbols_above(int c);

	/*
	Converts a Symbol to an int, used only in encoding.
	It returns 0 (the escape symbol) if symbol does not
	exists in the current context, and one otherwise.
	This sets the Symbol low_count, high_count, and scale.
	*/
	int convert_int_to_symbol(int c, Symbol& s);

	/*
	Returns the scale of the current context, used only in decoding.
	*/
	void get_symbol_scale(Symbol& s);

	/*
	* Converts a count to an int, called after get_symbol_scale.
	* 1. Finds the symbol that matches the count,
	* 2. Sets the low- and high-count for the symbol (it has to be removed from the stream).
	*/
	int convert_symbol_to_int(uint32_t count, Symbol& s);

private:
	/*
	1. Totals are calculated by accumulating counts in the current table
	(symbol exclusions are used).
	2. Escape probability is calculated here.

	Note that the accumulated counts must never exceed CODER_MAXSCALE,
	but as CODER_MAXSCALE is 2^29, this shouldn't be an issue, and so
	is not explicitly checked.
	*/
	inline void totalize_table(UniversalTable& context);

	const int max_symbol_;

	int current_order_;
	int num_symbols_;

	std::vector<uint32_t> totals_;
	bool* scoreboard_;
	std::vector<UniversalTable*> contexts_;
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
	int convert_int_to_symbol(int c, Symbol& s);

	/*
	* Returns the scale (sum of counts) of the current context, used for decoding.
	*/
	void get_symbol_scale(Symbol& s);

	/*
	* Converts a count to an int, called after get_symbol_scale.
	*/
	int convert_symbol_to_int(std::uint32_t count, Symbol& s);

private:

	std::vector<BinaryTable*> contexts_;
};

class ArithmeticEncoder {
public:
	ArithmeticEncoder(Writer& stream);
	~ArithmeticEncoder();

	void encode(UniversalModel& model, int c);

	void encode(BinaryModel& model, int c);

	/*
	 * Called when done writing to the ArithmeticEncoder. If this has not been called when the object is deleted,
	 * it will be called in the object's destructor.
	 */
	void finalize();

	/*
	 * Returns whether an error occurred in the writer backing the encoder.
	 */
	bool error() const;

private:
	// Encodes the sybol.
	void encode(const Symbol& s);

	// io variables:
	std::unique_ptr<ArithmeticBitWriter> bitwriter_ = std::make_unique<ArithmeticBitWriter>();
	Writer& writer_; // Reference for final batch write of encoded bits on finalization.

	// Arithmetic coding variables:
	std::uint32_t clow_ = 0;
	std::uint32_t chigh_ = CODER_LIMIT100 - 1;
	std::uint32_t cstep_ = 0;
	int nrbits_ = 0;

	bool finalized_ = false; // Has the encoder been finalized (via a call to finalize())?
};

class ArithmeticDecoder {
public:
	ArithmeticDecoder(Reader& stream);
	~ArithmeticDecoder() = default;

	int decode(UniversalModel& model);

	int decode(BinaryModel& model);

private:
	void decode(const Symbol& s);
	std::uint32_t decode_count(const Symbol& s);

	std::uint8_t read_bit();

	// io variables:
	Reader& reader_; // Pointer to iostream for reading.
	std::uint8_t curr_byte_ = 0;
	int curr_bit_ = 0;

	// Arithmetic coding variables:
	std::uint32_t ccode_ = 0;
	std::uint32_t clow_ = 0;
	std::uint32_t chigh_ = CODER_LIMIT100 - 1;
	std::uint32_t cstep_ = 0;
};

#endif