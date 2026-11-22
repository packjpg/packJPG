#include "aricoder.h"

#include "bitops.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <stdexcept>


void ArithmeticBitWriter::flush_buffer() {
	if ( ( sink_ != nullptr ) && !data_.empty() ) {
		sink_->write( data_.data(), data_.size() );
		data_.clear();
	}
}

void ArithmeticBitWriter::flush_to_sink() {
	flush_buffer();
}

void ArithmeticBitWriter::write_n_zero_bits(std::uint64_t n) {
	if (n == 0) {
		return;
	}
	const std::uint64_t rem = 8u - curr_bit_;
	if (n >= rem) {
		n -= rem;
		curr_byte_ = static_cast<std::uint8_t>(curr_byte_ << rem);
		emit_byte(curr_byte_);
		curr_bit_ = 0;
	}

	while (n >= 8) {
		emit_byte(0);
		n -= 8;
	}

	curr_byte_ = static_cast<std::uint8_t>(curr_byte_ << n);
	curr_bit_ += static_cast<std::size_t>(n);
}

void ArithmeticBitWriter::write_n_one_bits(std::uint64_t n) {
	if (n == 0) {
		return;
	}
	constexpr std::uint8_t all_ones = std::numeric_limits<std::uint8_t>::max();
	const std::uint64_t rem = 8u - curr_bit_;
	if (n >= rem) {
		n -= rem;
		curr_byte_ = static_cast<std::uint8_t>((curr_byte_ << rem) | (all_ones >> (8u - rem)));
		emit_byte(curr_byte_);
		curr_bit_ = 0;
	}

	while (n >= 8) {
		emit_byte(all_ones);
		n -= 8;
	}

	if (n > 0) {
		curr_byte_ = static_cast<std::uint8_t>((curr_byte_ << n) | (all_ones >> (8u - n)));
		curr_bit_ += static_cast<std::size_t>(n);
	}
}

void ArithmeticBitWriter::pad() {
	while (curr_bit_ > 0) {
		write_bit<0>();
	}
}

ArithmeticDecoder::ArithmeticDecoder(Reader& reader) : reader_(reader) {
	ccode = read_bits( static_cast<int>( CODER_USE_BITS ) );
}

bool ArithmeticDecoder::refill_buffer() {
	buf_pos_ = 0;
	buf_len_ = reader_.read( in_buf_.data(), in_buf_.size() );
	return buf_len_ > 0;
}

void ArithmeticDecoder::fill_bit_reservoir() {
	// Allow up to 512 bytes (4,096 bits) of synthesized zero bytes past EOF to satisfy
	// trailing E3 underflow runs omitted by PJG v2.5 finalize() when clow >= 0.25,
	// while bounding post-EOF E3 fixed-point synthesis on truncated streams.
	while ( bits_left_ <= 56 ) {
		std::uint8_t next_b = 0;
		if ( ( buf_pos_ < buf_len_ ) || refill_buffer() ) {
			next_b = in_buf_[ buf_pos_++ ];
		}
		else if ( ++eof_zero_bytes_ > 512u ) {
			throw std::runtime_error( "unexpected end of arithmetic stream" );
		}
		bit_buf_ = ( bit_buf_ << 8 ) | next_b;
		bits_left_ += 8;
	}
}

ArithmeticEncoder::ArithmeticEncoder(Writer& writer)
	: bitwriter_(&writer) {}

ArithmeticEncoder::~ArithmeticEncoder() noexcept {
	if (!finalized && std::uncaught_exceptions() == 0) {
		try {
			this->finalize();
		} catch (...) {
		}
	}
}

void ArithmeticEncoder::finalize() {
	if (finalized) {
		return;
	}

	if (clow < CODER_LIMIT025) {
		bitwriter_.write_bit<0>();
		bitwriter_.write_bit<1>();
		bitwriter_.write_n_one_bits(nrbits);
		nrbits = 0;
	} else {
		bitwriter_.write_bit<1>();
	}

	bitwriter_.pad();
	bitwriter_.flush_to_sink();

	finalized = true;
}


/* -----------------------------------------------
	universal statistical model for arithmetic coding
	----------------------------------------------- */

model_s::model_s( int max_s, int max_c, int max_o, int c_lim ) :
	max_symbol(max_s),
	max_context(max_c),
	max_order(max_o + 1),
	max_count(c_lim),
	use_bitmask_(max_s <= 64),
	current_order(max_o + 1),
	sb0_count(max_s),
	totals(static_cast<std::size_t>(std::max(0, max_s) + 2), 0),
	scoreboard_(use_bitmask_ ? 0 : static_cast<std::size_t>(std::max(0, max_s)), std::uint8_t(0))
{
	if ( max_order < 0 || max_order >= static_cast<int>( contexts_.size() ) ||
	     max_symbol <= 0 || max_symbol > 65535 ||
	     max_context < 0 || max_count <= 1 || max_count > 65535 ) {
		throw std::runtime_error( "invalid model_s configuration" );
	}

	nodes_.reserve( 64 );
	counts_pool_.reserve( static_cast<std::size_t>( max_symbol ) * 16 );
	links_pool_.reserve( static_cast<std::size_t>( std::max( 1, max_context ) ) * 16 );

	// Node 0: null table (uniform probability 1 for all symbols)
	const std::int32_t null_idx = alloc_node( false );
	nodes_[ static_cast<std::size_t>( null_idx ) ].counts_offset = static_cast<std::int32_t>( counts_pool_.size() );
	counts_pool_.resize( counts_pool_.size() + static_cast<std::size_t>( max_symbol ), std::uint16_t( 1 ) );
	nodes_[ static_cast<std::size_t>( null_idx ) ].max_count  = 1;
	nodes_[ static_cast<std::size_t>( null_idx ) ].max_symbol = static_cast<std::uint16_t>( max_symbol );

	// Node 1: start table
	const std::int32_t start_idx = alloc_node( max_context > 0 );

	contexts_[ 0 ] = null_idx;
	contexts_[ 1 ] = start_idx;

	// Build initial chain along context 0
	for ( int i = 2; i <= max_order; i++ ) {
		const std::int32_t created = alloc_node( i < max_order );
		contexts_[ static_cast<std::size_t>( i ) ] = created;
		const std::int32_t parent_links = nodes_[ static_cast<std::size_t>( contexts_[ static_cast<std::size_t>( i - 1 ) ] ) ].links_offset;
		links_pool_[ static_cast<std::size_t>( parent_links ) ] = created;
	}
}

std::int32_t model_s::alloc_node( bool with_links )
{
	const auto idx = static_cast<std::int32_t>( nodes_.size() );
	SNode node{};
	if ( with_links && ( max_context > 0 ) ) {
		node.links_offset = static_cast<std::int32_t>( links_pool_.size() );
		links_pool_.resize( links_pool_.size() + static_cast<std::size_t>( max_context ), -1 );
	}
	nodes_.push_back( node );
	return idx;
}

void model_s::alloc_counts( std::int32_t node_idx )
{
	const auto new_off = static_cast<std::int32_t>( counts_pool_.size() );
	counts_pool_.resize( counts_pool_.size() + static_cast<std::size_t>( max_symbol ), 0 );
	nodes_[ static_cast<std::size_t>( node_idx ) ].counts_offset = new_off;
}

void model_s::rescale_node( SNode& node ) noexcept
{
	if ( node.counts_offset < 0 ) {
		return;
	}

	std::uint16_t* counts = counts_pool_.data() + node.counts_offset;
	const int lst_symbol = node.max_symbol;
	for ( int i = 0; i < lst_symbol; i++ ) {
		counts[ i ] >>= 1;
	}

	node.max_count >>= 1;

	int i = lst_symbol - 1;
	for ( ; i >= 0; i-- ) {
		if ( counts[ i ] > 0 ) {
			break;
		}
	}
	node.max_symbol = static_cast<std::uint16_t>( i + 1 );
}

void model_s::flush_model()
{
	for ( std::size_t idx = 1; idx < nodes_.size(); ++idx ) {
		rescale_node( nodes_[ idx ] );
	}
}


/* -----------------------------------------------
	special version of model_s for binary coding
	----------------------------------------------- */

model_b::model_b( int max_c, int max_o, int c_lim ) :
	max_context(max_c),
	max_order(max_o + 1),
	max_count(c_lim)
{
	if ( max_order < 0 || max_order >= static_cast<int>( contexts_.size() ) ||
	     max_context < 0 || max_count <= 1 || max_count > 65535 ) {
		throw std::runtime_error( "invalid model_b configuration" );
	}


	nodes_.reserve( 64 );
	links_pool_.reserve( static_cast<std::size_t>( std::max( 1, max_context ) ) * 8 );

	// Node 0: null table
	const std::int32_t null_idx = alloc_node( false );
	nodes_[ static_cast<std::size_t>( null_idx ) ].counts0 = 1;
	nodes_[ static_cast<std::size_t>( null_idx ) ].counts1 = 1;
	nodes_[ static_cast<std::size_t>( null_idx ) ].scale   = 2;

	// Node 1: start table
	const std::int32_t start_idx = alloc_node( max_context > 0 );

	contexts_[ 0 ] = null_idx;
	contexts_[ 1 ] = start_idx;

	for ( int i = 2; i <= max_order; i++ ) {
		const std::int32_t created = alloc_node( i < max_order );
		contexts_[ static_cast<std::size_t>( i ) ] = created;
		const std::int32_t parent_links = nodes_[ static_cast<std::size_t>( contexts_[ static_cast<std::size_t>( i - 1 ) ] ) ].links_offset;
		links_pool_[ static_cast<std::size_t>( parent_links ) ] = created;
	}
}

std::int32_t model_b::alloc_node( bool with_links )
{
	const auto idx = static_cast<std::int32_t>( nodes_.size() );
	BNode node{};
	if ( with_links && ( max_context > 0 ) ) {
		node.links_offset = static_cast<std::int32_t>( links_pool_.size() );
		links_pool_.resize( links_pool_.size() + static_cast<std::size_t>( max_context ), -1 );
	}
	nodes_.push_back( node );
	return idx;
}

void model_b::flush_model()
{
	for ( std::size_t idx = 1; idx < nodes_.size(); ++idx ) {
		nodes_[ idx ].rescale();
	}
}
