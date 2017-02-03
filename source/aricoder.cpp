#include "aricoder.h"

#include "bitops.h"

#include <algorithm>
#include <functional>
#include <stdlib.h>

/* -----------------------------------------------
	constructor for aricoder class
	----------------------------------------------- */

aricoder::aricoder( iostream* stream, StreamMode iomode ) : sptr(stream), mode(iomode)
{
	if ( mode == StreamMode::kRead) { // mode is reading / decoding
		// code buffer has to be filled before starting decoding
		for (int i = 0; i < CODER_USE_BITS; i++ )
			ccode = ( ccode << 1 ) | read_bit();
	} // mode is writing / encoding otherwise
}

/* -----------------------------------------------
	destructor for aricoder class
	----------------------------------------------- */

aricoder::~aricoder()
{
	if ( mode == StreamMode::kWrite) { // mode is writing / encoding
		// due to clow < CODER_LIMIT050, and chigh >= CODER_LIMIT050
		// there are only two possible cases
		if ( clow < CODER_LIMIT025 ) { // case a.) 
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
}

/* -----------------------------------------------
	arithmetic encoder function
	----------------------------------------------- */
	
void aricoder::encode( symbol* s )
{	
	// Make local copies of clow_ and chigh_ for cache performance:
	uint32_t clow_local = clow;
	uint32_t chigh_local = chigh;
	// update steps, low count, high count
	cstep = (chigh_local - clow_local + 1) / s->scale;
	chigh_local = clow_local + (cstep * s->high_count) - 1;
	clow_local = clow_local + (cstep * s->low_count);
	
	// e3 scaling is performed for speed and to avoid underflows
	// if both, low and high are either in the lower half or in the higher half
	// one bit can be safely shifted out
	while ( clow_local >= CODER_LIMIT050  || chigh_local < CODER_LIMIT050  ) {
		if (chigh_local < CODER_LIMIT050 ) {	// this means both, high and low are below, and 0 can be safely shifted out
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
	while ( (clow_local >= CODER_LIMIT025 ) && (chigh_local < CODER_LIMIT075 ) ) {
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

void aricoder::writeNrbitsAsZero() {
	if (nrbits + cbit >= 8) {
		int remainingBits = 8 - cbit;
		nrbits -= remainingBits;
		bbyte <<= remainingBits;
		sptr->write_byte(bbyte);
		cbit = 0;
	}

	constexpr uint8_t zero = 0;
	while (nrbits >= 8) {
		sptr->write_byte(zero);
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

void aricoder::writeNrbitsAsOne() {
	if (nrbits + cbit >= 8) {
		int remainingBits = 8 - cbit;
		nrbits -= remainingBits;
		bbyte <<= remainingBits;
		bbyte |= std::numeric_limits<uint8_t>::max() >> (8 - remainingBits);
		sptr->write_byte(bbyte);
		cbit = 0;
	}

	constexpr uint8_t all_ones = std::numeric_limits<uint8_t>::max();
	while (nrbits >= 8) {
		sptr->write_byte(all_ones);
		nrbits -= 8;
	}

	/*
	No need to check if cbits is 8, since nrbits is strictly less than 8
	and cbit is initially 0 here:
	*/
	bbyte = (bbyte << nrbits) | (std::numeric_limits<uint8_t>::max() >> (8 - nrbits));
	cbit += nrbits;
	nrbits = 0;
}


/* -----------------------------------------------
	arithmetic decoder get count function
	----------------------------------------------- */
	
unsigned int aricoder::decode_count( symbol* s )
{
	// update cstep, which is needed to remove the symbol from the stream later
	cstep = ( ( chigh - clow ) + 1 ) / s->scale;
	
	// return counts, needed to decode the symbol from the statistical model
	return ( ccode - clow ) / cstep;
}

/* -----------------------------------------------
	arithmetic decoder function
	----------------------------------------------- */
	
void aricoder::decode( symbol* s )
{
	// no actual decoding takes place, as this has to happen in the statistical model
	// the symbol has to be removed from the stream, though
	
	// alread have steps updated from decoder_count
	// update low count and high count
	uint32_t ccode_local = ccode;
	uint32_t clow_local = clow;
	uint32_t chigh_local = clow_local + (cstep * s->high_count) - 1;
	clow_local = clow_local + (cstep * s->low_count);
	
	// e3 scaling is performed for speed and to avoid underflows
	// if both, low and high are either in the lower half or in the higher half
	// one bit can be safely shifted out
	while ( (clow_local >= CODER_LIMIT050 ) || (chigh_local < CODER_LIMIT050 ) ) {
		if (clow_local >= CODER_LIMIT050 ) {
			clow_local &= CODER_LIMIT050 - 1;
			chigh_local &= CODER_LIMIT050 - 1;
			ccode_local &= CODER_LIMIT050 - 1;
		} // if the first wasn't the case, it's chigh < CODER_LIMIT050
		clow_local <<= 1;
		chigh_local <<= 1;
		chigh_local++;
		ccode_local <<= 1;
		ccode_local |= read_bit();
		nrbits = 0;
	}
	
	// e3 scaling, to make sure that theres enough space between low and high
	while ( (clow_local >= CODER_LIMIT025 ) && (chigh_local < CODER_LIMIT075 ) ) {
		nrbits++;
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

/* -----------------------------------------------
	bit reader function
	----------------------------------------------- */
	
unsigned char aricoder::read_bit()
{
	// read in new byte if needed
	if ( cbit == 0 ) {
		if ( !sptr->read_byte(&bbyte)) // read next byte if available
			bbyte = 0; // if no more data is left in the stream
		cbit = 8;
	}
	
	// decrement current bit position
	cbit--;	
	// return bit at cbit position
	return BITN( bbyte, cbit );
}


/* -----------------------------------------------
	universal statistical model for arithmetic coding
	
	boundaries of this model:
	max_s (maximum symbol) -> 1 <= max_s <= 1024 (???)
	max_c (maximum context) -> 1 <= max_c <= 1024 (???)
	max_o (maximum order) -> -1 <= max_o <= 4
	c_lim (maximum count) -> 2 <= c_lim <= 4096 (???)
	WARNING: this can be memory intensive, so don't overdo it
	max_s == 256; max_c == 256; max_o == 4 would be way too much
	----------------------------------------------- */
	
model_s::model_s( int max_s, int max_c, int max_o, int c_lim ) :
		// Copy settings into the model:
		max_symbol(max_s),
		max_context(max_c),
		max_order(max_o + 1),
		max_count(c_lim),

		current_order(max_o + 1),
		sb0_count(max_s),

		totals(max_s + 2),
		scoreboard(new bool[max_s]),
		contexts(max_o + 3)
{
	std::fill(scoreboard, scoreboard + max_symbol, false);
	
	// set up null table
	table_s* null_table = new table_s;
	null_table->counts = std::vector<uint16_t>(max_symbol, uint16_t(1));  // Set all probabilities to 1.

	// set up internal counts
	null_table->max_count = 1;
	null_table->max_symbol = max_symbol;
	
	// set up start table
	table_s* start_table = new table_s;
	start_table->links = std::vector<table_s*>(max_context);
	
	// integrate tables into contexts
	contexts[ 0 ] = null_table;
	contexts[ 1 ] = start_table;
	
	// build initial 'normal' tables
	for (int i = 2; i <= max_order; i++ ) {
		// set up current order table
		contexts[i] = new table_s;
		// build forward links
		if ( i < max_order ) {
			contexts[i]->links = std::vector<table_s*>(max_context);
		}
		contexts[ i - 1 ]->links[ 0 ] = contexts[ i ];
	}
}


/* -----------------------------------------------
	model class destructor - recursive cleanup of memory is done here
	----------------------------------------------- */

model_s::~model_s()
{	
	// clean up each 'normal' table
	delete contexts[1];
	
	// clean up null table
	delete contexts[0];
	
	// free everything else
	delete[] scoreboard;
}


/* -----------------------------------------------
	Updates statistics for a specific symbol / resets to highest order.
	Use -1 if you just want to reset without updating statistics.
	----------------------------------------------- */
void model_s::update_model( int symbol )
{		
	// only contexts, that were actually used to encode
	// the symbol get its count updated
	if ( symbol >= 0 ) {
		for (int local_order = ( current_order < 1 ) ? 1 : current_order;
				local_order <= max_order; local_order++ ) {
			table_s* context = contexts[ local_order ];
			auto& count = context->counts[symbol];
			// update count for specific symbol & scale
			count++;
			// store side information for totalize_table
			context->max_count = std::max(count, context->max_count);
			context->max_symbol = std::max(uint16_t(symbol + 1), context->max_symbol);
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


/* -----------------------------------------------
	shift in one context (max no of contexts is max_c)
	----------------------------------------------- */
	
void model_s::shift_context( int c )
{	
	// shifting is not possible if max_order is below 1
	// or context index is negative
	if ( ( max_order < 2 ) || ( c < 0 ) ) return;
	
	// shift each orders' context
	for (int i = max_order; i > 1; i-- ) {
		// this is the new current order context
		table_s* context = contexts[ i - 1 ]->links[ c ];
		
		// check if context exists, build if needed
		if ( context == nullptr ) {
			// reserve memory for next table_s
			context = new table_s;
			// finished here if this is a max order context
			if ( i < max_order ) {
				// build links to higher order tables otherwise
				context->links.resize(max_context);
			}
			// put context to its right place
			contexts[ i - 1 ]->links[ c ] = context;
		}
		
		// switch context
		contexts[ i ] = context;
	}
}


/* -----------------------------------------------
	Flushes the entire model by calling rescale_table on all contexts.
	----------------------------------------------- */
	
void model_s::flush_model()
{
	contexts[1]->recursive_flush();
}


/* -----------------------------------------------
	Excludes every symbol above c.
	----------------------------------------------- */
	
void model_s::exclude_symbols(int c)
{
	// exclusions are back to normal after update_model is used	

	for ( c = c + 1; c < max_symbol; c++ ) {
		if ( !scoreboard[ c ] ) {
			scoreboard[ c ] = true;
			sb0_count--;
		}
	}
}


/* -----------------------------------------------
	converts an int to a symbol, needed only when encoding
	----------------------------------------------- */
	
int model_s::convert_int_to_symbol( int c, symbol *s )
{
	// search the symbol c in the current context table_s,
	// return scale, low- and high counts	

	// totalize table for the current context
	table_s* context = contexts[ current_order ];
	totalize_table( context );
	
	// finding the scale is easy
	s->scale = totals[ 0 ];
	
	// check if that symbol exists in the current table. send escape otherwise
	if ( context->counts[ c ] > 0 ) {
		// return high and low count for the current symbol
		s->low_count  = totals[ c + 2 ];
		s->high_count = totals[ c + 1 ];
		return 0;
	}
	
	// return high and low count for the escape symbol
	s->low_count  = totals[ 1 ];
	s->high_count = totals[ 0 ];
	current_order--;
	return 1;
}


/* -----------------------------------------------
	returns the current context scale needed only when decoding
	----------------------------------------------- */
	
void model_s::get_symbol_scale( symbol *s )
{
	// getting the scale is easy: totalize the table_s, use accumulated count -> done
	totalize_table( contexts[ current_order ] );
	s->scale = totals[ 0 ];
}


/* -----------------------------------------------
	converts a count to an int, called after get_symbol_scale
	----------------------------------------------- */
	
int model_s::convert_symbol_to_int(uint32_t count, symbol *s)
{
	// seek the symbol that matches the count,
	// also, set low- and high count for the symbol - it has to be removed from the stream


	// go through the totals table, search the symbol that matches the count
	int c;
	for (c = 1; c < totals.size(); c++) {
		if (count >= totals[c]) {
			break;
		}
	}
	// set up the current symbol
	s->low_count = totals[c]; // It is guaranteed that there exists such a symbol.
	s->high_count = totals[c - 1]; // This is guaranteed to not go out of bounds since the search started at index 1 of totals.
	// send escape if escape symbol encountered
	if (c == 1) {
		current_order--;
		return ESCAPE_SYMBOL;
	}
	
	// return symbol value
	return c - 2 ; // Since c is not one and is a positive number, this will be nonnegative.
}


/* -----------------------------------------------
	totals are calculated by accumulating counts in the current table_s
	----------------------------------------------- */

void model_s::totalize_table( table_s* context )
{
	// update exclusion is used, so this has to be done each time
	// escape probability calculation also takes place here
	
	// accumulated counts must never exceed CODER_MAXSCALE
	// as CODER_MAXSCALE is big enough, though, (2^29), this shouldn't happen and is not checked
	
	const auto& counts = context->counts;
	
	// check counts
	if (!counts.empty()) {	// if counts are already set
		// locally store current fill/symbol count
		int local_symb = sb0_count;

		// set the last symbol of the totals to zero
		int i = context->max_symbol - 1;
		totals[i + 2] = 0;

		// (re)set current total
		uint32_t curr_total = 0;

		// go reverse though the whole counts table and accumulate counts
		// leave space at the beginning of the table for the escape symbol
		for (; i >= 0; i--) {
			// only count probability if the current symbol is not 'scoreboard - excluded'
			if (!scoreboard[i]) {
				uint16_t curr_count = counts[i];
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
		} else if (sb0_count == 0) {
			esc_prob = 0;
		} else {
			// esc_prob = 1;
			esc_prob  =  sb0_count * ( local_symb - sb0_count );
			esc_prob /= ( local_symb * context->max_count );
			esc_prob++;
		}
		// include escape probability in totals table
		totals[ 0 ] = totals[ 1 ] + esc_prob;
	} else { // if counts are not already set
		// setup counts for current table
		context->counts.resize(max_symbol);
		// set totals table -> only escape probability included
		totals[ 0 ] = 1;
		totals[ 1 ] = 0;
	}	
}

/* -----------------------------------------------
	special version of model_s for binary coding
	
	boundaries of this model:
	... (maximum symbol) -> 2 (0 or 1 )
	max_c (maximum context) -> 1 <= max_c <= 1024 (???)
	max_o (maximum order) -> -1 <= max_o <= 4
	----------------------------------------------- */

model_b::model_b( int max_c, int max_o, int c_lim ) :
		// Copy settings into the model:
		max_context(max_c),
		max_order(max_o + 1),
		max_count(c_lim),

		contexts(max_o + 3)
{
	// set up null table
	table* null_table = new table;
	null_table->counts = std::vector<uint16_t>(2, uint16_t(1));
	null_table->scale = uint32_t(2);
	
	// set up start table
	table* start_table = new table;
	start_table->links = std::vector<table*>(max_context);
		
	// integrate tables into contexts
	contexts[ 0 ] = null_table;
	contexts[ 1 ] = start_table;
	
	// build initial 'normal' tables
	for (int i = 2; i <= max_order; i++ ) {
		// set up current order table
		contexts[i] = new table;
		// build forward links
		if ( i < max_order ) {
			contexts[i]->links = std::vector<table*>(max_context);
		}
		contexts[ i - 1 ]->links[ 0 ] = contexts[ i ];
	}
}


/* -----------------------------------------------
	model class destructor - recursive cleanup of memory is done here
	----------------------------------------------- */
	
model_b::~model_b()
{
	// clean up each 'normal' table
	delete contexts[1];
	
	// clean up null table
	delete contexts[0];
}


/* -----------------------------------------------
	updates statistics for a specific symbol / resets to highest order
	----------------------------------------------- */
	
void model_b::update_model( int symbol )
{
	// use -1 if you just want to reset without updating statistics
	
	table* context = contexts[ max_order ];
	
	// only contexts, that were actually used to encode
	// the symbol get their counts updated
	if ( ( symbol >= 0 ) && ( max_order >= 0 ) ) {
		// update count for specific symbol & scale
		context->counts[ symbol ]++;
		context->scale++;
		// if counts for that symbol have gone above the maximum count
		// the table has to be resized (scale factor 2)
		if ( context->counts[ symbol ] >= max_count )
			context->rescale_table();
	}
}


/* -----------------------------------------------
	shift in one context (max no of contexts is max_c)
	----------------------------------------------- */
	
void model_b::shift_context( int c )
{
	// shifting is not possible if max_order is below 1
	// or context index is negative
	if ( (max_order < 2 ) || ( c < 0 ) ) return;
	
	// shift each orders' context
	for (int i = max_order; i > 1; i-- ) {
		// this is the new current order context
		table* context = contexts[ i - 1 ]->links[ c ];
		
		// check if context exists, build if needed
		if ( context == nullptr ) {
			// reserve memory for next table
			context = new table;		
			// finished here if this is a max order context
			if ( i < max_order) {
				// build links to higher order tables otherwise
				context->links.resize(max_context);
			}
			// put context to its right place
			contexts[ i - 1 ]->links[ c ] = context;
		}
		
		// switch context
		contexts[ i ] = context;
	}
}


/* -----------------------------------------------
	Flushes the entire model by calling rescale_table on all contexts.
	----------------------------------------------- */
	
void model_b::flush_model()
{
	contexts[1]->recursive_flush();
}


/* -----------------------------------------------
	converts an int to a symbol, needed only when encoding
	----------------------------------------------- */
	
int model_b::convert_int_to_symbol( int c, symbol *s )
{
	table* context = contexts[ max_order ];
	
	// check if counts are available
	context->check_counts();
	
	// finding the scale is easy
	s->scale = context->scale;
	
	// return high and low count for current symbol
	if ( c == 0 ) { // if 0 is to be encoded
		s->low_count  = uint32_t(0);
		s->high_count = context->counts[ 0 ];
	}
	else { // if 1 is to be encoded
		s->low_count  = context->counts[ 0 ];
		s->high_count = context->scale;
	}
	
	return 1;
}


/* -----------------------------------------------
	returns the current context scale needed only when decoding
	----------------------------------------------- */
	
void model_b::get_symbol_scale( symbol *s )
{
	table* context = contexts[ max_order ];
	
	// check if counts are available
	context->check_counts();
	
	// getting the scale is easy
	s->scale = context->scale;
}


/* -----------------------------------------------
	converts a count to an int, called after get_symbol_scale
	----------------------------------------------- */
	
int model_b::convert_symbol_to_int(uint32_t count, symbol *s)
{
	table* context = contexts[ max_order ];
	auto counts0 = context->counts[ 0 ];
	
	// set up the current symbol
	if ( count < counts0 ) {
		s->low_count  = uint32_t(0);
		s->high_count = counts0;
		return 0;
	}
	else {
		s->low_count  = counts0;
		s->high_count = s->scale;
		return 1;
	}
}
