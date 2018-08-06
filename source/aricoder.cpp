#include "aricoder.h"

#include "bitops.h"

#include <algorithm>
#include <functional>

ArithmeticEncoder::ArithmeticEncoder(Writer& stream) : writer_(stream) {}

ArithmeticEncoder::~ArithmeticEncoder() {
	if (!finalized_) {
		finalize();
	}
}

void ArithmeticEncoder::finalize() {
	if (finalized_) {
		return;
	}
	// due to clow < CODER_LIMIT050, and chigh >= CODER_LIMIT050
	// there are only two possible cases
	if (clow_ < CODER_LIMIT025) { // case a.) 
		bitwriter_->write_bit<0>();
		// write remaining bits
		bitwriter_->write_bit<1>();
		bitwriter_->write_n_one_bits(nrbits_);
		nrbits_ = 0;
	} else { // case b.), clow >= CODER_LIMIT025
		bitwriter_->write_bit<1>();
	} // done, zeroes are auto-read by the decoder

	bitwriter_->pad(); // Pad code with zeroes.
	writer_.write(bitwriter_->get_data());
	finalized_ = true;
}

void ArithmeticEncoder::encode(UniversalModel& model, int c) {
	Symbol s;
	int esc;

	do {
		esc = model.convert_int_to_symbol(c, s);
		encode(s);
	} while (esc);
	model.update_model(c);
}

void ArithmeticEncoder::encode(BinaryModel& model, int c) {
	Symbol s;
	model.convert_int_to_symbol(c, s);
	encode(s);
	model.update_model(c);
}

bool ArithmeticEncoder::error() const {
	return writer_.error();
}


void ArithmeticEncoder::encode(const Symbol& s)
{
	// Make local copies of clow_ and chigh_ for cache performance:
	std::uint32_t clow_local = clow_;
	std::uint32_t chigh_local = chigh_;
	// update steps, low count, high count
	cstep_ = (chigh_local - clow_local + 1) / s.scale;
	chigh_local = clow_local + (cstep_ * s.high_count) - 1;
	clow_local = clow_local + (cstep_ * s.low_count);

	// e3 scaling is performed for speed and to avoid underflows
	// if both, low and high are either in the lower half or in the higher half
	// one bit can be safely shifted out
	while (clow_local >= CODER_LIMIT050 || chigh_local < CODER_LIMIT050) {
		if (chigh_local < CODER_LIMIT050) {	// this means both, high and low are below, and 0 can be safely shifted out
											// write 0 bit
			bitwriter_->write_bit<0>();
			// shift out remaing e3 bits
			bitwriter_->write_n_one_bits(nrbits_);
			nrbits_ = 0;
		}
		else { // if the first wasn't the case, it's clow >= CODER_LIMIT050
			   // write 1 bit
			bitwriter_->write_bit<1>();
			clow_local &= CODER_LIMIT050 - 1;
			chigh_local &= CODER_LIMIT050 - 1;
			// shift out remaing e3 bits
			bitwriter_->write_n_zero_bits(nrbits_);
			nrbits_ = 0;
		}
		clow_local <<= 1;
		chigh_local <<= 1;
		chigh_local++;
	}

	// e3 scaling, to make sure that theres enough space between low and high
	while ((clow_local >= CODER_LIMIT025) && (chigh_local < CODER_LIMIT075)) {
		nrbits_++;
		clow_local &= CODER_LIMIT025 - 1;
		chigh_local ^= CODER_LIMIT025 + CODER_LIMIT050;
		// clow  -= CODER_LIMIT025;
		// chigh -= CODER_LIMIT025;
		clow_local <<= 1;
		chigh_local <<= 1;
		chigh_local++;
	}

	clow_ = clow_local;
	chigh_ = chigh_local;
}

ArithmeticDecoder::ArithmeticDecoder(Reader& stream) : reader_(stream) {
	// code buffer has to be filled before starting decoding
	ccode_ = 0;
	for (std::uint32_t i = 0; i < CODER_USE_BITS; i++) {
		ccode_ = (ccode_ << 1) | read_bit();
	}
}

int ArithmeticDecoder::decode(UniversalModel& model) {
	Symbol s;
	int c;

	do {
		model.get_symbol_scale(s);
		std::uint32_t count = decode_count(s);
		c = model.convert_symbol_to_int(count, s);
		decode(s);
	} while (c == ESCAPE_SYMBOL);
	model.update_model(c);

	return c;
}

int ArithmeticDecoder::decode(BinaryModel& model) {
	Symbol s;

	model.get_symbol_scale(s);
	std::uint32_t count = decode_count(s);
	int c = model.convert_symbol_to_int(count, s);
	decode(s);
	model.update_model(c);

	return c;
}

std::uint32_t ArithmeticDecoder::decode_count(const Symbol& s)
{
	// update cstep, which is needed to remove the symbol from the stream later
	cstep_ = ((chigh_ - clow_) + 1) / s.scale;

	// return counts, needed to decode the symbol from the statistical model
	return (ccode_ - clow_) / cstep_;
}

void ArithmeticDecoder::decode(const Symbol& s)
{
	// no actual decoding takes place, as this has to happen in the statistical model
	// the symbol has to be removed from the stream, though

	// alread have steps updated from decoder_count
	// update low count and high count
	std::uint32_t ccode_local = ccode_;
	std::uint32_t clow_local = clow_;
	std::uint32_t chigh_local = clow_local + (cstep_ * s.high_count) - 1;
	clow_local = clow_local + (cstep_ * s.low_count);

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
	chigh_ = chigh_local;
	clow_ = clow_local;
	ccode_ = ccode_local;
}

std::uint8_t ArithmeticDecoder::read_bit()
{
	// read in new byte if needed
	if (curr_bit_ == 0) {
		if (!reader_.read_byte(&curr_byte_)) // read next byte if available
			curr_byte_ = 0; // if no more data is left in the stream
		curr_bit_ = 8;
	}

	// decrement current bit position
	curr_bit_--;
	// return bit at cbit position
	return bitops::bitn(curr_byte_, curr_bit_);
}

UniversalModel::UniversalModel(int max_s, int max_c, int max_o, int c_lim) :
	// Copy settings into the model:
	Model(max_c, max_o, c_lim),
	max_symbol_(max_s),

	current_order_(max_o + 1),
	num_symbols_(max_s),

	totals_(max_s + 2),
	scoreboard_(new bool[max_s]),
	contexts_(max_o + 3)
{
	std::fill(scoreboard_, scoreboard_ + max_symbol_, false);

	// set up null table
	UniversalTable* null_table = new UniversalTable;
	null_table->counts = std::vector<std::uint16_t>(max_symbol_, std::uint16_t(1));  // Set all probabilities to 1.

	// set up internal counts
	null_table->max_symbol_count = 1;
	null_table->max_symbol = max_symbol_;

	// set up start table
	UniversalTable* start_table = new UniversalTable;
	start_table->links = std::vector<UniversalTable*>(max_context_);

	// integrate tables into contexts
	contexts_[0] = null_table;
	contexts_[1] = start_table;

	// build initial 'normal' tables
	for (int i = 2; i <= max_order_; i++) {
		// set up current order table
		contexts_[i] = new UniversalTable;
		// build forward links
		if (i < max_order_) {
			contexts_[i]->links = std::vector<UniversalTable*>(max_context_);
		}
		contexts_[i - 1]->links[0] = contexts_[i];
	}
}

UniversalModel::~UniversalModel()
{
	// clean up each 'normal' table
	delete contexts_[1];

	// clean up null table
	delete contexts_[0];

	// free everything else
	delete[] scoreboard_;
}

void UniversalModel::update_model(int symbol)
{
	if (symbol >= 0) {
		for (int local_order = (current_order_ < 1) ? 1 : current_order_;
			local_order <= max_order_; local_order++) {
			UniversalTable* context = contexts_[local_order];
			auto& count = context->counts[symbol];
			// update count for specific symbol & scale
			count++;
			// store side information for totalize_table
			context->max_symbol_count = std::max(count, context->max_symbol_count);
			context->max_symbol = std::max(symbol + 1, context->max_symbol);
			// if count for that symbol have gone above the maximum count
			// the table has to be resized (scale factor 2)
			if (count == max_symbol_count_) {
				context->rescale_table();
			}
		}
	}

	// reset scoreboard and current order
	current_order_ = max_order_;
	std::fill(scoreboard_, scoreboard_ + max_symbol_, false);
	num_symbols_ = max_symbol_;
}

void UniversalModel::shift_context(int c)
{
	if ((max_order_ < 2) || (c < 0)) return;

	// shift each orders' context
	for (int i = max_order_; i > 1; i--) {
		// this is the new current order context
		UniversalTable* context = contexts_[i - 1]->links[c];

		// check if context exists, build if needed
		if (context == nullptr) {
			// reserve memory for next table_s
			context = new UniversalTable;
			// finished here if this is a max order context
			if (i < max_order_) {
				// build links to higher order tables otherwise
				context->links.resize(max_context_);
			}
			// put context to its right place
			contexts_[i - 1]->links[c] = context;
		}

		// switch context
		contexts_[i] = context;
	}
}

void UniversalModel::flush_model()
{
	contexts_[1]->recursive_flush();
}

void UniversalModel::exclude_symbols_above(int c)
{
	for (c = c + 1; c < max_symbol_; c++) {
		if (!scoreboard_[c]) {
			scoreboard_[c] = true;
			num_symbols_--;
		}
	}
}

int UniversalModel::convert_int_to_symbol(int c, Symbol& s)
{
	// totalize table for the current context
	UniversalTable* context = contexts_[current_order_];
	totalize_table(*context);

	// finding the scale is easy
	s.scale = totals_[0];

	// check if that symbol exists in the current table. send escape otherwise
	if (context->counts[c] > 0) {
		// return high and low count for the current symbol
		s.low_count = totals_[c + 2];
		s.high_count = totals_[c + 1];
		return 0;
	}

	// return high and low count for the escape symbol
	s.low_count = totals_[1];
	s.high_count = totals_[0];
	current_order_--;
	return 1;
}

void UniversalModel::get_symbol_scale(Symbol& s)
{
	// Getting the scale is easy: totalize the table, and then use the accumulated count:
	totalize_table(*contexts_[current_order_]);
	s.scale = totals_[0];
}

int UniversalModel::convert_symbol_to_int(std::uint32_t count, Symbol& s)
{
	// go through the totals table, search the symbol that matches the count
	std::uint32_t c;
	for (c = 1; c < totals_.size(); c++) {
		if (count >= totals_[c]) {
			break;
		}
	}
	// set up the current symbol
	s.low_count = totals_[c]; // It is guaranteed that there exists such a symbol.
	s.high_count = totals_[c - 1]; // This is guaranteed to not go out of bounds since the search started at index 1 of totals.
								   // send escape if escape symbol encountered
	if (c == 1) {
		current_order_--;
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
		int local_symb = num_symbols_;

		// set the last symbol of the totals to zero
		std::int32_t i = context.max_symbol - 1;
		totals_[i + 2] = 0;

		// (re)set current total
		std::uint32_t curr_total = 0;

		// go reverse though the whole counts table and accumulate counts
		// leave space at the beginning of the table for the escape symbol
		for (; i >= 0; i--) {
			// only count probability if the current symbol is not 'scoreboard - excluded'
			if (!scoreboard_[i]) {
				std::uint16_t curr_count = counts[i];
				if (curr_count > 0) {
					// add counts for the current symbol
					curr_total += curr_count;
					// exclude symbol from scoreboard
					scoreboard_[i] = true;
					num_symbols_--;
				}
			}
			totals_[i + 1] = curr_total;
		}
		// here the escape calculation needs to take place
		std::uint32_t esc_prob;
		if (local_symb == num_symbols_) {
			esc_prob = 1;
		}
		else if (num_symbols_ == 0) {
			esc_prob = 0;
		}
		else {
			// esc_prob = 1;
			esc_prob = num_symbols_ * (local_symb - num_symbols_);
			esc_prob /= (local_symb * context.max_symbol_count);
			esc_prob++;
		}
		// include escape probability in totals table
		totals_[0] = totals_[1] + esc_prob;
	}
	else { // if counts are not already set
		   // setup counts for current table
		context.counts.resize(max_symbol_);
		// set totals table -> only escape probability included
		totals_[0] = 1;
		totals_[1] = 0;
	}
}

BinaryModel::BinaryModel(int max_c, int max_o, int c_lim) :
	// Copy settings into the model:
	Model(max_c, max_o, c_lim),
	contexts_(max_o + 3)
{
	// set up null table
	BinaryTable* null_table = new BinaryTable;
	null_table->counts = std::vector<std::uint16_t>(2, std::uint16_t(1));
	null_table->scale = std::uint32_t(2);

	// set up start table
	BinaryTable* start_table = new BinaryTable;
	start_table->links = std::vector<BinaryTable*>(max_context_);

	// integrate tables into contexts
	contexts_[0] = null_table;
	contexts_[1] = start_table;

	// build initial 'normal' tables
	for (int i = 2; i <= max_order_; i++) {
		// set up current order table
		contexts_[i] = new BinaryTable;
		// build forward links
		if (i < max_order_) {
			contexts_[i]->links = std::vector<BinaryTable*>(max_context_);
		}
		contexts_[i - 1]->links[0] = contexts_[i];
	}
}

BinaryModel::~BinaryModel()
{
	// clean up each 'normal' table
	delete contexts_[1];

	// clean up null table
	delete contexts_[0];
}

void BinaryModel::update_model(int symbol) {
	// only contexts, that were actually used to encode
	// the symbol get their counts updated
	if (symbol >= 0 && max_order_ >= 0) {
		BinaryTable* context = contexts_[max_order_];
		// update count for specific symbol & scale
		context->counts[symbol]++;
		context->scale++;
		// if counts for that symbol have gone above the maximum count
		// the table has to be resized (scale factor 2)
		if (context->counts[symbol] >= max_symbol_count_)
			context->rescale_table();
	}
}

void BinaryModel::shift_context(int c)
{
	if ((max_order_ < 2) || (c < 0)) return;

	// shift each orders' context
	for (int i = max_order_; i > 1; i--) {
		// this is the new current order context
		BinaryTable* context = contexts_[i - 1]->links[c];

		// check if context exists, build if needed
		if (context == nullptr) {
			// reserve memory for next table
			context = new BinaryTable;
			// finished here if this is a max order context
			if (i < max_order_) {
				// build links to higher order tables otherwise
				context->links.resize(max_context_);
			}
			// put context to its right place
			contexts_[i - 1]->links[c] = context;
		}

		// switch context
		contexts_[i] = context;
	}
}

void BinaryModel::flush_model()
{
	contexts_[1]->recursive_flush();
}

int BinaryModel::convert_int_to_symbol(int c, Symbol& s)
{
	BinaryTable* context = contexts_[max_order_];

	// check if counts are available
	context->create_counts_if_empty();

	// finding the scale is easy
	s.scale = context->scale;

	// return high and low count for current symbol
	if (c == 0) { // if 0 is to be encoded
		s.low_count = std::uint32_t(0);
		s.high_count = context->counts[0];
	} else { // if 1 is to be encoded
		s.low_count = context->counts[0];
		s.high_count = context->scale;
	}

	return 1;
}

void BinaryModel::get_symbol_scale(Symbol& s)
{
	BinaryTable* context = contexts_[max_order_];

	// check if counts are available
	context->create_counts_if_empty();

	// getting the scale is easy
	s.scale = context->scale;
}

int BinaryModel::convert_symbol_to_int(std::uint32_t count, Symbol& s)
{
	BinaryTable* context = contexts_[max_order_];
	auto counts0 = context->counts[0];

	// set up the current symbol
	if (count < counts0) {
		s.low_count = std::uint32_t(0);
		s.high_count = counts0;
		return 0;
	} else {
		s.low_count = counts0;
		s.high_count = s.scale;
		return 1;
	}
}
