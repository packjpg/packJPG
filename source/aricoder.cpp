#include "aricoder.h"

#include "bitops.h"

#include <algorithm>
#include <functional>

ArithmeticEncoder::ArithmeticEncoder(Writer& stream) : sptr(stream) {}

ArithmeticEncoder::~ArithmeticEncoder() {
	// due to clow < CODER_LIMIT050, and chigh >= CODER_LIMIT050
	// there are only two possible cases
	if (clow < CODER_LIMIT025) { // case a.) 
		write_bit<0>();
		// write remaining bits
		write_bit<1>();
		writeNrbitsAsOne();
	}
	else { // case b.), clow >= CODER_LIMIT025
		write_bit<1>();
	} // done, zeroes are auto-read by the decoder

	  // pad code with zeroes
	while (cbit > 0) {
		write_bit<0>();
	}
}

void ArithmeticEncoder::encode(const Symbol& s)
{
	// Make local copies of clow_ and chigh_ for cache performance:
	uint32_t clow_local = clow;
	uint32_t chigh_local = chigh;
	// update steps, low count, high count
	cstep = (chigh_local - clow_local + 1) / s.scale;
	chigh_local = clow_local + (cstep * s.high_count) - 1;
	clow_local = clow_local + (cstep * s.low_count);

	// e3 scaling is performed for speed and to avoid underflows
	// if both, low and high are either in the lower half or in the higher half
	// one bit can be safely shifted out
	while (clow_local >= CODER_LIMIT050 || chigh_local < CODER_LIMIT050) {
		if (chigh_local < CODER_LIMIT050) {	// this means both, high and low are below, and 0 can be safely shifted out
											// write 0 bit
			write_bit<0>();
			// shift out remaing e3 bits
			writeNrbitsAsOne();
		}
		else { // if the first wasn't the case, it's clow >= CODER_LIMIT050
			   // write 1 bit
			write_bit<1>();
			clow_local &= CODER_LIMIT050 - 1;
			chigh_local &= CODER_LIMIT050 - 1;
			// shift out remaing e3 bits
			writeNrbitsAsZero();
		}
		clow_local <<= 1;
		chigh_local <<= 1;
		chigh_local++;
	}

	// e3 scaling, to make sure that theres enough space between low and high
	while ((clow_local >= CODER_LIMIT025) && (chigh_local < CODER_LIMIT075)) {
		nrbits++;
		clow_local &= CODER_LIMIT025 - 1;
		chigh_local ^= CODER_LIMIT025 + CODER_LIMIT050;
		// clow  -= CODER_LIMIT025;
		// chigh -= CODER_LIMIT025;
		clow_local <<= 1;
		chigh_local <<= 1;
		chigh_local++;
	}

	clow = clow_local;
	chigh = chigh_local;
}

void ArithmeticEncoder::writeNrbitsAsZero() {
	if (nrbits + cbit >= 8) {
		int remainingBits = 8 - cbit;
		nrbits -= remainingBits;
		bbyte <<= remainingBits;
		sptr.write_byte(bbyte);
		cbit = 0;
	}

	constexpr std::uint8_t zero = 0;
	while (nrbits >= 8) {
		sptr.write_byte(zero);
		nrbits -= 8;
	}
	/*
	No need to check if cbits is 8, since nrbits is strictly less than 8
	and cbit is initially 0 here:
	*/
	bbyte <<= nrbits;
	cbit += nrbits;
	nrbits = 0;
}

void ArithmeticEncoder::writeNrbitsAsOne() {
	constexpr std::uint8_t all_ones = std::numeric_limits<std::uint8_t>::max();
	if (nrbits + cbit >= 8) {
		int remainingBits = 8 - cbit;
		nrbits -= remainingBits;
		bbyte <<= remainingBits;
		bbyte |= all_ones >> (8 - remainingBits);
		sptr.write_byte(bbyte);
		cbit = 0;
	}

	while (nrbits >= 8) {
		sptr.write_byte(all_ones);
		nrbits -= 8;
	}

	/*
	No need to check if cbits is 8, since nrbits is strictly less than 8
	and cbit is initially 0 here:
	*/
	bbyte = (bbyte << nrbits) | (all_ones >> (8 - nrbits));
	cbit += nrbits;
	nrbits = 0;
}

ArithmeticDecoder::ArithmeticDecoder(Reader& stream) : sptr(stream) {
	// code buffer has to be filled before starting decoding
	for (int i = 0; i < CODER_USE_BITS; i++) {
		ccode = (ccode << 1) | read_bit();
	}
}

ArithmeticDecoder::~ArithmeticDecoder() {}

std::uint32_t ArithmeticDecoder::decode_count(const Symbol& s)
{
	// update cstep, which is needed to remove the symbol from the stream later
	cstep = ((chigh - clow) + 1) / s.scale;

	// return counts, needed to decode the symbol from the statistical model
	return (ccode - clow) / cstep;
}

void ArithmeticDecoder::decode(const Symbol& s)
{
	// no actual decoding takes place, as this has to happen in the statistical model
	// the symbol has to be removed from the stream, though

	// alread have steps updated from decoder_count
	// update low count and high count
	std::uint32_t ccode_local = ccode;
	std::uint32_t clow_local = clow;
	std::uint32_t chigh_local = clow_local + (cstep * s.high_count) - 1;
	clow_local = clow_local + (cstep * s.low_count);

	// e3 scaling is performed for speed and to avoid underflows
	// if both, low and high are either in the lower half or in the higher half
	// one bit can be safely shifted out
	while ((clow_local >= CODER_LIMIT050) || (chigh_local < CODER_LIMIT050)) {
		if (clow_local >= CODER_LIMIT050) {
			clow_local &= CODER_LIMIT050 - 1;
			chigh_local &= CODER_LIMIT050 - 1;
			ccode_local &= CODER_LIMIT050 - 1;
		} // if the first wasn't the case, it's chigh < CODER_LIMIT050
		clow_local <<= 1;
		chigh_local <<= 1;
		chigh_local++;
		ccode_local <<= 1;
		ccode_local |= read_bit();
	}

	// e3 scaling, to make sure that theres enough space between low and high
	while ((clow_local >= CODER_LIMIT025) && (chigh_local < CODER_LIMIT075)) {
		clow_local &= CODER_LIMIT025 - 1;
		chigh_local ^= CODER_LIMIT025 + CODER_LIMIT050;
		// clow  -= CODER_LIMIT025;
		// chigh -= CODER_LIMIT025;
		ccode_local -= CODER_LIMIT025;
		clow_local <<= 1;
		chigh_local <<= 1;
		chigh_local++;
		ccode_local <<= 1;
		ccode_local |= read_bit();
	}
	chigh = chigh_local;
	clow = clow_local;
	ccode = ccode_local;
}

std::uint8_t ArithmeticDecoder::read_bit()
{
	// read in new byte if needed
	if (cbit == 0) {
		if (!sptr.read_byte(&bbyte)) // read next byte if available
			bbyte = 0; // if no more data is left in the stream
		cbit = 8;
	}

	// decrement current bit position
	cbit--;
	// return bit at cbit position
	return bitops::bitn(bbyte, cbit);
}

UniversalModel::UniversalModel(int max_s, int max_c, int max_o, int c_lim) :
	// Copy settings into the model:
	Model(max_c, max_o, c_lim),
	max_symbol(max_s),

	current_order(max_o + 1),
	sb0_count(max_s),

	totals(max_s + 2),
	scoreboard(new bool[max_s]),
	contexts(max_o + 3)
{
	std::fill(scoreboard, scoreboard + max_symbol, false);

	// set up null table
	UniversalTable* null_table = new UniversalTable;
	null_table->counts = std::vector<std::uint16_t>(max_symbol, std::uint16_t(1));  // Set all probabilities to 1.

	// set up internal counts
	null_table->max_count = 1;
	null_table->max_symbol = max_symbol;

	// set up start table
	UniversalTable* start_table = new UniversalTable;
	start_table->links = std::vector<UniversalTable*>(max_context);

	// integrate tables into contexts
	contexts[0] = null_table;
	contexts[1] = start_table;

	// build initial 'normal' tables
	for (int i = 2; i <= max_order; i++) {
		// set up current order table
		contexts[i] = new UniversalTable;
		// build forward links
		if (i < max_order) {
			contexts[i]->links = std::vector<UniversalTable*>(max_context);
		}
		contexts[i - 1]->links[0] = contexts[i];
	}
}

UniversalModel::~UniversalModel()
{
	// clean up each 'normal' table
	delete contexts[1];

	// clean up null table
	delete contexts[0];

	// free everything else
	delete[] scoreboard;
}

void UniversalModel::update_model(int symbol)
{
	if (symbol >= 0) {
		for (int local_order = (current_order < 1) ? 1 : current_order;
			local_order <= max_order; local_order++) {
			UniversalTable* context = contexts[local_order];
			auto& count = context->counts[symbol];
			// update count for specific symbol & scale
			count++;
			// store side information for totalize_table
			context->max_count = std::max(count, context->max_count);
			context->max_symbol = std::max(std::uint16_t(symbol + 1), context->max_symbol);
			// if count for that symbol have gone above the maximum count
			// the table has to be resized (scale factor 2)
			if (count == max_count) {
				context->rescale_table();
			}
		}
	}

	// reset scoreboard and current order
	current_order = max_order;
	std::fill(scoreboard, scoreboard + max_symbol, false);
	sb0_count = max_symbol;
}

void UniversalModel::shift_context(int c)
{
	if ((max_order < 2) || (c < 0)) return;

	// shift each orders' context
	for (int i = max_order; i > 1; i--) {
		// this is the new current order context
		UniversalTable* context = contexts[i - 1]->links[c];

		// check if context exists, build if needed
		if (context == nullptr) {
			// reserve memory for next table_s
			context = new UniversalTable;
			// finished here if this is a max order context
			if (i < max_order) {
				// build links to higher order tables otherwise
				context->links.resize(max_context);
			}
			// put context to its right place
			contexts[i - 1]->links[c] = context;
		}

		// switch context
		contexts[i] = context;
	}
}

void UniversalModel::flush_model()
{
	contexts[1]->recursive_flush();
}

void UniversalModel::exclude_symbols(int c)
{
	for (c = c + 1; c < max_symbol; c++) {
		if (!scoreboard[c]) {
			scoreboard[c] = true;
			sb0_count--;
		}
	}
}

int UniversalModel::convert_int_to_symbol(int c, Symbol& s)
{
	// totalize table for the current context
	UniversalTable* context = contexts[current_order];
	totalize_table(*context);

	// finding the scale is easy
	s.scale = totals[0];

	// check if that symbol exists in the current table. send escape otherwise
	if (context->counts[c] > 0) {
		// return high and low count for the current symbol
		s.low_count = totals[c + 2];
		s.high_count = totals[c + 1];
		return 0;
	}

	// return high and low count for the escape symbol
	s.low_count = totals[1];
	s.high_count = totals[0];
	current_order--;
	return 1;
}

void UniversalModel::get_symbol_scale(Symbol& s)
{
	// Getting the scale is easy: totalize the table, and then use the accumulated count:
	totalize_table(*contexts[current_order]);
	s.scale = totals[0];
}

int UniversalModel::convert_symbol_to_int(uint32_t count, Symbol& s)
{
	// go through the totals table, search the symbol that matches the count
	int c;
	for (c = 1; c < totals.size(); c++) {
		if (count >= totals[c]) {
			break;
		}
	}
	// set up the current symbol
	s.low_count = totals[c]; // It is guaranteed that there exists such a symbol.
	s.high_count = totals[c - 1]; // This is guaranteed to not go out of bounds since the search started at index 1 of totals.
								   // send escape if escape symbol encountered
	if (c == 1) {
		current_order--;
		return ESCAPE_SYMBOL;
	}

	// return symbol value
	return c - 2; // Since c is not one and is a positive number, this will be nonnegative.
}

void UniversalModel::totalize_table(UniversalTable& context)
{
	const auto& counts = context.counts;

	// check counts
	if (!counts.empty()) {	// if counts are already set
							// locally store current fill/symbol count
		int local_symb = sb0_count;

		// set the last symbol of the totals to zero
		int i = context.max_symbol - 1;
		totals[i + 2] = 0;

		// (re)set current total
		uint32_t curr_total = 0;

		// go reverse though the whole counts table and accumulate counts
		// leave space at the beginning of the table for the escape symbol
		for (; i >= 0; i--) {
			// only count probability if the current symbol is not 'scoreboard - excluded'
			if (!scoreboard[i]) {
				std::uint16_t curr_count = counts[i];
				if (curr_count > 0) {
					// add counts for the current symbol
					curr_total += curr_count;
					// exclude symbol from scoreboard
					scoreboard[i] = true;
					sb0_count--;
				}
			}
			totals[i + 1] = curr_total;
		}
		// here the escape calculation needs to take place
		uint32_t esc_prob;
		if (local_symb == sb0_count) {
			esc_prob = 1;
		}
		else if (sb0_count == 0) {
			esc_prob = 0;
		}
		else {
			// esc_prob = 1;
			esc_prob = sb0_count * (local_symb - sb0_count);
			esc_prob /= (local_symb * context.max_count);
			esc_prob++;
		}
		// include escape probability in totals table
		totals[0] = totals[1] + esc_prob;
	}
	else { // if counts are not already set
		   // setup counts for current table
		context.counts.resize(max_symbol);
		// set totals table -> only escape probability included
		totals[0] = 1;
		totals[1] = 0;
	}
}

BinaryModel::BinaryModel(int max_c, int max_o, int c_lim) :
	// Copy settings into the model:
	Model(max_c, max_o, c_lim),
	contexts(max_o + 3)
{
	// set up null table
	BinaryTable* null_table = new BinaryTable;
	null_table->counts = std::vector<std::uint16_t>(2, std::uint16_t(1));
	null_table->scale = uint32_t(2);

	// set up start table
	BinaryTable* start_table = new BinaryTable;
	start_table->links = std::vector<BinaryTable*>(max_context);

	// integrate tables into contexts
	contexts[0] = null_table;
	contexts[1] = start_table;

	// build initial 'normal' tables
	for (int i = 2; i <= max_order; i++) {
		// set up current order table
		contexts[i] = new BinaryTable;
		// build forward links
		if (i < max_order) {
			contexts[i]->links = std::vector<BinaryTable*>(max_context);
		}
		contexts[i - 1]->links[0] = contexts[i];
	}
}

BinaryModel::~BinaryModel()
{
	// clean up each 'normal' table
	delete contexts[1];

	// clean up null table
	delete contexts[0];
}

void BinaryModel::update_model(int symbol) {
	// only contexts, that were actually used to encode
	// the symbol get their counts updated
	if (symbol >= 0 && max_order >= 0) {
		BinaryTable* context = contexts[max_order];
		// update count for specific symbol & scale
		context->counts[symbol]++;
		context->scale++;
		// if counts for that symbol have gone above the maximum count
		// the table has to be resized (scale factor 2)
		if (context->counts[symbol] >= max_count)
			context->rescale_table();
	}
}

void BinaryModel::shift_context(int c)
{
	if ((max_order < 2) || (c < 0)) return;

	// shift each orders' context
	for (int i = max_order; i > 1; i--) {
		// this is the new current order context
		BinaryTable* context = contexts[i - 1]->links[c];

		// check if context exists, build if needed
		if (context == nullptr) {
			// reserve memory for next table
			context = new BinaryTable;
			// finished here if this is a max order context
			if (i < max_order) {
				// build links to higher order tables otherwise
				context->links.resize(max_context);
			}
			// put context to its right place
			contexts[i - 1]->links[c] = context;
		}

		// switch context
		contexts[i] = context;
	}
}

void BinaryModel::flush_model()
{
	contexts[1]->recursive_flush();
}

int BinaryModel::convert_int_to_symbol(int c, Symbol& s)
{
	BinaryTable* context = contexts[max_order];

	// check if counts are available
	context->create_counts_if_empty();

	// finding the scale is easy
	s.scale = context->scale;

	// return high and low count for current symbol
	if (c == 0) { // if 0 is to be encoded
		s.low_count = uint32_t(0);
		s.high_count = context->counts[0];
	} else { // if 1 is to be encoded
		s.low_count = context->counts[0];
		s.high_count = context->scale;
	}

	return 1;
}

void BinaryModel::get_symbol_scale(Symbol& s)
{
	BinaryTable* context = contexts[max_order];

	// check if counts are available
	context->create_counts_if_empty();

	// getting the scale is easy
	s.scale = context->scale;
}

int BinaryModel::convert_symbol_to_int(std::uint32_t count, Symbol& s)
{
	BinaryTable* context = contexts[max_order];
	auto counts0 = context->counts[0];

	// set up the current symbol
	if (count < counts0) {
		s.low_count = uint32_t(0);
		s.high_count = counts0;
		return 0;
	} else {
		s.low_count = counts0;
		s.high_count = s.scale;
		return 1;
	}
}