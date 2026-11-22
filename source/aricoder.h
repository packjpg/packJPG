#ifndef ARICODER_H
#define ARICODER_H

#include "bitops.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

// defines for coder
constexpr uint32_t CODER_USE_BITS = 31; // Must never be above 31.
constexpr uint32_t CODER_LIMIT100 = uint32_t(1) << CODER_USE_BITS;
constexpr uint32_t CODER_LIMIT025 = CODER_LIMIT100 / 4;
constexpr uint32_t CODER_LIMIT050 = (CODER_LIMIT100 / 4) * 2;
constexpr uint32_t CODER_LIMIT075 = (CODER_LIMIT100 / 4) * 3;
constexpr uint32_t CODER_MAXSCALE = CODER_LIMIT025 - 1;
constexpr uint32_t ESCAPE_SYMBOL = CODER_LIMIT025;

inline int ari_popcount_u64( std::uint64_t x ) noexcept
{
#if defined(__GNUC__) || defined(__clang__)
	return __builtin_popcountll( x );
#else
	int count = 0;
	while ( x != 0 ) {
		x &= ( x - 1 );
		++count;
	}
	return count;
#endif
}

// symbol struct, used in arithmetic coding
struct symbol {
	uint32_t low_count;
	uint32_t high_count;
	uint32_t scale;
};

class ArithmeticBitWriter {
public:
	static constexpr std::size_t kFlushThreshold = 65536;

	explicit ArithmeticBitWriter(Writer* sink = nullptr) : sink_(sink) {
		data_.reserve(sink != nullptr ? kFlushThreshold : 256);
	}

	template <std::uint8_t bit>
	inline void write_bit() {
		curr_byte_ = static_cast<std::uint8_t>((curr_byte_ << 1) | bit);
		curr_bit_++;
		if (curr_bit_ == 8) {
			emit_byte(curr_byte_);
			curr_bit_ = 0;
		}
	}

	void write_n_zero_bits(std::uint64_t n);
	void write_n_one_bits(std::uint64_t n);
	void pad();
	void flush_to_sink();

	const std::vector<std::uint8_t>& get_data() const noexcept {
		return data_;
	}

private:
	void flush_buffer();

	inline void emit_byte(std::uint8_t b) {
		data_.push_back(b);
		if ((sink_ != nullptr) && (data_.size() >= kFlushThreshold)) {
			flush_buffer();
		}
	}

	Writer* sink_ = nullptr;
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
	~ArithmeticEncoder() noexcept;

	inline void encode( symbol* s )
	{
		uint32_t clow_local = clow;
		uint32_t chigh_local = chigh;
		cstep = (chigh_local - clow_local + 1) / s->scale;
		chigh_local = clow_local + (cstep * s->high_count) - 1;
		clow_local = clow_local + (cstep * s->low_count);

		while ( clow_local >= CODER_LIMIT050 || chigh_local < CODER_LIMIT050 ) {
			if ( chigh_local < CODER_LIMIT050 ) {
				bitwriter_.write_bit<0>();
				if ( nrbits > 0 ) {
					bitwriter_.write_n_one_bits( nrbits );
					nrbits = 0;
				}
			}
			else {
				bitwriter_.write_bit<1>();
				clow_local &= CODER_LIMIT050 - 1;
				chigh_local &= CODER_LIMIT050 - 1;
				if ( nrbits > 0 ) {
					bitwriter_.write_n_zero_bits( nrbits );
					nrbits = 0;
				}
			}
			clow_local <<= 1;
			chigh_local = ( chigh_local << 1 ) + 1;
		}

		while ( ( clow_local >= CODER_LIMIT025 ) && ( chigh_local < CODER_LIMIT075 ) ) {
			nrbits++;
			clow_local &= CODER_LIMIT025 - 1;
			chigh_local ^= CODER_LIMIT025 + CODER_LIMIT050;
			clow_local <<= 1;
			chigh_local = ( chigh_local << 1 ) + 1;
		}

		clow = clow_local;
		chigh = chigh_local;
	}

	void finalize();
	void abandon() noexcept { finalized = true; }

private:
	bool finalized = false;
	ArithmeticBitWriter bitwriter_;

	unsigned int clow = 0;
	unsigned int chigh = CODER_LIMIT100 - 1;
	unsigned int cstep = 0;
	std::uint64_t nrbits = 0;
};


class ArithmeticDecoder {
public:
	explicit ArithmeticDecoder(Reader& reader);
	~ArithmeticDecoder() = default;

	inline unsigned int decode_count( symbol* s )
	{
		if ( s->scale == 0 || chigh < clow || ccode < clow || ccode > chigh ) {
			throw std::runtime_error( "corrupted arithmetic stream" );
		}
		cstep = ( ( chigh - clow ) + 1 ) / s->scale;
		if ( cstep == 0 ) {
			throw std::runtime_error( "corrupted arithmetic stream" );
		}

		const unsigned int count = ( ccode - clow ) / cstep;
		if ( count >= s->scale ) {
			throw std::runtime_error( "corrupted arithmetic stream" );
		}
		return count;
	}

	inline void decode( symbol* s )
	{
		constexpr uint32_t kMask100 = CODER_LIMIT100 - 1u;
		uint32_t ccode_local = ccode;
		uint32_t clow_local  = clow;
		uint32_t chigh_local = clow_local + ( cstep * s->high_count ) - 1u;
		clow_local           = clow_local + ( cstep * s->low_count );

		if ( ( ( clow_local ^ chigh_local ) & CODER_LIMIT050 ) == 0u ) {
#if defined(__GNUC__) || defined(__clang__)
			const int k = __builtin_clz( ( ( clow_local ^ chigh_local ) << 1 ) | 1u );
#else
			int k = 0;
			uint32_t diff = ( ( clow_local ^ chigh_local ) << 1 ) | 1u;
			while ( ( diff & 0x80000000u ) == 0u ) {
				diff <<= 1;
				++k;
			}
#endif
			clow_local  = ( clow_local << k ) & kMask100;
			chigh_local = ( ( chigh_local << k ) | ( ( 1u << k ) - 1u ) ) & kMask100;
			ccode_local = ( ( ccode_local << k ) | read_bits( k ) ) & kMask100;
		}

		while ( ( clow_local >= CODER_LIMIT025 ) && ( chigh_local < CODER_LIMIT075 ) ) {
			clow_local &= CODER_LIMIT025 - 1u;
			chigh_local ^= CODER_LIMIT025 + CODER_LIMIT050;
			ccode_local -= CODER_LIMIT025;
			clow_local <<= 1;
			chigh_local = ( chigh_local << 1 ) + 1u;
			ccode_local = ( ccode_local << 1 ) | read_bits( 1 );
		}

		chigh = chigh_local;
		clow  = clow_local;
		ccode = ccode_local;
	}

private:
	bool refill_buffer();
	void fill_bit_reservoir();

	inline uint32_t read_bits( int k )
	{
		if ( bits_left_ < k ) {
			fill_bit_reservoir();
		}
		bits_left_ -= k;
		return static_cast<uint32_t>( ( bit_buf_ >> bits_left_ ) & ( ( 1u << k ) - 1u ) );
	}

	Reader& reader_;
	std::array<std::uint8_t, 4096> in_buf_{};
	std::size_t buf_pos_ = 0;
	std::size_t buf_len_ = 0;
	std::uint64_t bit_buf_ = 0;
	int bits_left_ = 0;
	std::size_t eof_zero_bytes_ = 0;

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

	inline void update_model( int symbol )
	{
		if ( symbol >= 0 ) {
			for ( int local_order = ( current_order < 1 ) ? 1 : current_order;
				  local_order <= max_order; local_order++ ) {
				const std::int32_t node_idx = contexts_[ static_cast<std::size_t>( local_order ) ];
				if ( nodes_[ static_cast<std::size_t>( node_idx ) ].counts_offset < 0 ) {
					alloc_counts( node_idx );
				}
				SNode& context = nodes_[ static_cast<std::size_t>( node_idx ) ];
				std::uint16_t& count = counts_pool_[ static_cast<std::size_t>( context.counts_offset + symbol ) ];
				count++;
				context.max_count  = std::max( count, context.max_count );
				context.max_symbol = std::max( std::uint16_t( symbol + 1 ), context.max_symbol );
				if ( count == max_count ) {
					rescale_node( context );
				}
			}
		}

		current_order = max_order;
		if ( use_bitmask_ ) {
			sb_mask_ = 0;
		}
		else {
			std::fill( scoreboard_.begin(), scoreboard_.end(), std::uint8_t( 0 ) );
		}
		sb0_count = max_symbol;
	}

	inline void shift_context( int c )
	{
		if ( ( max_order < 2 ) || ( c < 0 ) ) return;
		if ( c >= max_context ) {
			throw std::runtime_error( "corrupted arithmetic stream" );
		}

		for ( int i = max_order; i > 1; i-- ) {
			const std::int32_t parent_links = nodes_[ static_cast<std::size_t>( contexts_[ static_cast<std::size_t>( i - 1 ) ] ) ].links_offset;
			std::int32_t child = links_pool_[ static_cast<std::size_t>( parent_links + c ) ];
			if ( child < 0 ) {
				child = alloc_node( i < max_order );
				links_pool_[ static_cast<std::size_t>( parent_links + c ) ] = child;
			}
			contexts_[ static_cast<std::size_t>( i ) ] = child;
		}
	}

	void flush_model();

	inline void exclude_symbols( int c )
	{
		const int start = std::max( 0, c + 1 );
		if ( start >= max_symbol ) {
			return;
		}

		if ( use_bitmask_ ) {
			const std::uint64_t valid_bits = ( max_symbol == 64 )
				? ~std::uint64_t( 0 )
				: ( ( std::uint64_t( 1 ) << max_symbol ) - 1u );
			const std::uint64_t range_mask = valid_bits & ( ~std::uint64_t( 0 ) << start );
			sb_mask_ |= range_mask;
			sb0_count = max_symbol - ari_popcount_u64( sb_mask_ );
		}
		else {
			for ( int sym = start; sym < max_symbol; sym++ ) {
				if ( !scoreboard_[ static_cast<std::size_t>( sym ) ] ) {
					scoreboard_[ static_cast<std::size_t>( sym ) ] = 1;
					sb0_count--;
				}
			}
		}
	}

	inline int convert_int_to_symbol( int c, symbol* s )
	{
		if ( c < 0 || c >= max_symbol || current_order < 0 || current_order > max_order ) {
			throw std::runtime_error( "symbol out of range in arithmetic encoder" );
		}

		const std::int32_t node_idx = contexts_[ static_cast<std::size_t>( current_order ) ];
		totalize_table( node_idx );

		s->scale = totals[ 0 ];

		const SNode& context = nodes_[ static_cast<std::size_t>( node_idx ) ];
		if ( counts_pool_[ static_cast<std::size_t>( context.counts_offset + c ) ] > 0 ) {
			s->low_count  = totals[ static_cast<std::size_t>( c + 2 ) ];
			s->high_count = totals[ static_cast<std::size_t>( c + 1 ) ];
			if ( s->high_count <= s->low_count ) {
				throw std::runtime_error( "excluded symbol in arithmetic encoder" );
			}
			return 0;
		}

		if ( current_order <= 0 || totals[ 0 ] <= totals[ 1 ] ) {
			throw std::runtime_error( "cannot escape null order in arithmetic encoder" );
		}
		s->low_count  = totals[ 1 ];
		s->high_count = totals[ 0 ];
		current_order--;
		return 1;
	}


	inline void get_symbol_scale( symbol* s )
	{
		if ( current_order < 0 || current_order > max_order ) {
			throw std::runtime_error( "corrupted arithmetic stream" );
		}
		totalize_table( contexts_[ static_cast<std::size_t>( current_order ) ] );
		s->scale = totals[ 0 ];
	}

	inline int convert_symbol_to_int( uint32_t count, symbol* s )
	{
		int c = 1;
		if ( totals_end_ <= 16 ) {
			// For small alphabets (DC/AC bit lengths), symbol 0 (c=2) or escape (c=1)
			// occurs ~85% of the time, making a short sequential scan much friendlier
			// to the branch predictor than binary search.
			while ( ( c < totals_end_ ) && ( count < totals[ static_cast<std::size_t>( c ) ] ) ) {
				++c;
			}
		}
		else {
			int lo = 1;
			int hi = totals_end_;
			while ( lo < hi ) {
				const int mid = ( lo + hi ) >> 1;
				if ( count >= totals[ static_cast<std::size_t>( mid ) ] ) {
					hi = mid;
				}
				else {
					lo = mid + 1;
				}
			}
			c = lo;
		}

		if ( c > totals_end_ || count < totals[ static_cast<std::size_t>( c ) ] ||
			 totals[ static_cast<std::size_t>( c - 1 ) ] <= totals[ static_cast<std::size_t>( c ) ] ) {
			throw std::runtime_error( "corrupted arithmetic stream" );
		}

		s->low_count  = totals[ static_cast<std::size_t>( c ) ];
		s->high_count = totals[ static_cast<std::size_t>( c - 1 ) ];

		if ( c == 1 ) {
			if ( current_order <= 0 ) {
				throw std::runtime_error( "corrupted arithmetic stream" );
			}
			current_order--;
			return ESCAPE_SYMBOL;
		}

		return c - 2;
	}

private:
	struct SNode {
		std::int32_t  counts_offset = -1;
		std::int32_t  links_offset  = -1;
		std::uint16_t max_count     = 0;
		std::uint16_t max_symbol    = 0;
	};

	std::int32_t alloc_node( bool with_links );
	void alloc_counts( std::int32_t node_idx );
	void rescale_node( SNode& node ) noexcept;

	inline void totalize_table( std::int32_t node_idx )
	{
		SNode& context = nodes_[ static_cast<std::size_t>( node_idx ) ];

		if ( context.counts_offset >= 0 ) {
			const std::uint16_t* counts = counts_pool_.data() + context.counts_offset;
			const int local_symb = sb0_count;

			int i = static_cast<int>( context.max_symbol ) - 1;
			totals_end_ = i + 2;
			totals[ static_cast<std::size_t>( totals_end_ ) ] = 0;

			uint32_t curr_total = 0;

			if ( use_bitmask_ ) {
				const std::uint64_t mask = sb_mask_;
				std::uint64_t new_nonzero = 0;
				for ( ; i >= 0; i-- ) {
					const std::uint32_t c = counts[ i ];
					const std::uint32_t allow = static_cast<std::uint32_t>( ( ~mask >> i ) & 1u );
					curr_total += c * allow;
					new_nonzero |= static_cast<std::uint64_t>( c != 0 ) << i;
					totals[ static_cast<std::size_t>( i + 1 ) ] = curr_total;
				}
				sb_mask_ = mask | new_nonzero;
				sb0_count = max_symbol - ari_popcount_u64( sb_mask_ );
			}
			else {
				for ( ; i >= 0; i-- ) {
					if ( !scoreboard_[ static_cast<std::size_t>( i ) ] ) {
						const uint16_t curr_count = counts[ i ];
						if ( curr_count > 0 ) {
							curr_total += curr_count;
							scoreboard_[ static_cast<std::size_t>( i ) ] = 1;
							sb0_count--;
						}
					}
					totals[ static_cast<std::size_t>( i + 1 ) ] = curr_total;
				}
			}

			uint32_t esc_prob;
			if ( local_symb == sb0_count ) {
				esc_prob = 1;
			}
			else if ( sb0_count == 0 ) {
				esc_prob = 0;
			}
			else {
				esc_prob  = static_cast<uint32_t>( sb0_count * ( local_symb - sb0_count ) );
				esc_prob /= static_cast<uint32_t>( local_symb * context.max_count );
				esc_prob++;
			}
			totals[ 0 ] = totals[ 1 ] + esc_prob;
		}
		else {
			alloc_counts( node_idx );
			totals[ 0 ] = 1;
			totals[ 1 ] = 0;
			totals_end_ = 1;
		}
	}

	const int  max_symbol;
	const int  max_context;
	const int  max_order;
	const int  max_count;
	const bool use_bitmask_;

	int current_order;
	int sb0_count;
	int totals_end_ = 1;
	std::uint64_t sb_mask_ = 0;

	std::vector<uint32_t> totals;
	std::vector<std::uint8_t> scoreboard_;
	std::vector<SNode> nodes_;
	std::vector<std::uint16_t> counts_pool_;
	std::vector<std::int32_t> links_pool_;
	std::array<std::int32_t, 8> contexts_{};
};


/* -----------------------------------------------
	binary statistical model for arithmetic coding
	----------------------------------------------- */

class model_b
{
public:
	model_b( int max_c, int max_o, int c_lim );

	inline void update_model( int symbol )
	{
		if ( ( symbol >= 0 ) && ( max_order >= 0 ) ) {
			BNode& ctx_node = nodes_[ static_cast<std::size_t>( contexts_[ static_cast<std::size_t>( max_order ) ] ) ];
			const std::uint16_t updated = ( symbol == 0 ) ? ++ctx_node.counts0 : ++ctx_node.counts1;
			ctx_node.scale++;
			if ( updated >= max_count ) {
				ctx_node.rescale();
			}
		}
	}

	inline void shift_context( int c )
	{
		if ( ( max_order < 2 ) || ( c < 0 ) ) return;
		if ( c >= max_context ) {
			throw std::runtime_error( "corrupted arithmetic stream" );
		}

		for ( int i = max_order; i > 1; i-- ) {
			const std::int32_t parent_links = nodes_[ static_cast<std::size_t>( contexts_[ static_cast<std::size_t>( i - 1 ) ] ) ].links_offset;
			std::int32_t child = links_pool_[ static_cast<std::size_t>( parent_links + c ) ];
			if ( child < 0 ) {
				child = alloc_node( i < max_order );
				links_pool_[ static_cast<std::size_t>( parent_links + c ) ] = child;
			}
			contexts_[ static_cast<std::size_t>( i ) ] = child;
		}
	}

	void flush_model();

	inline int convert_int_to_symbol( int c, symbol* s )
	{
		BNode& ctx_node = nodes_[ static_cast<std::size_t>( contexts_[ static_cast<std::size_t>( max_order ) ] ) ];
		ctx_node.check_counts();
		s->scale = ctx_node.scale;
		if ( c == 0 ) {
			s->low_count  = 0u;
			s->high_count = ctx_node.counts0;
		}
		else {
			s->low_count  = ctx_node.counts0;
			s->high_count = ctx_node.scale;
		}
		return 1;
	}

	inline void get_symbol_scale( symbol* s )
	{
		BNode& ctx_node = nodes_[ static_cast<std::size_t>( contexts_[ static_cast<std::size_t>( max_order ) ] ) ];
		ctx_node.check_counts();
		s->scale = ctx_node.scale;
	}

	inline int convert_symbol_to_int( uint32_t count, symbol* s )
	{
		const BNode& ctx_node = nodes_[ static_cast<std::size_t>( contexts_[ static_cast<std::size_t>( max_order ) ] ) ];
		const std::uint32_t counts0 = ctx_node.counts0;
		if ( count < counts0 ) {
			s->low_count  = 0u;
			s->high_count = counts0;
			return 0;
		}
		else {
			s->low_count  = counts0;
			s->high_count = s->scale;
			return 1;
		}
	}

private:
	struct BNode {
		std::int32_t  links_offset = -1;
		std::uint32_t scale        = 0;
		std::uint16_t counts0      = 0;
		std::uint16_t counts1      = 0;

		inline void check_counts() noexcept {
			if ( scale == 0 ) {
				counts0 = 1;
				counts1 = 1;
				scale   = 2;
			}
		}

		inline void rescale() noexcept {
			if ( scale != 0 ) {
				counts0 = std::max<std::uint16_t>( 1, static_cast<std::uint16_t>( counts0 >> 1 ) );
				counts1 = std::max<std::uint16_t>( 1, static_cast<std::uint16_t>( counts1 >> 1 ) );
				scale   = static_cast<std::uint32_t>( counts0 ) + static_cast<std::uint32_t>( counts1 );
			}
		}
	};

	std::int32_t alloc_node( bool with_links );

	const int max_context;
	const int max_order;
	const int max_count;

	std::vector<BNode> nodes_;
	std::vector<std::int32_t> links_pool_;
	std::array<std::int32_t, 8> contexts_{};
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

	do {
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
