#include "pjg_codec.h"

#include "jpeg_codec.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace {

void get_context_nnb( int pos, int w, int* a, int* b )
{
	if ( pos == 0 ) {
		*a = -1;
		*b = -1;
	}
	else if ( ( pos % w ) == 0 ) {
		*b = pos - w;
		*a = ( pos >= ( w << 1 ) ) ? ( pos - ( w << 1 ) ) : *b;
	}
	else if ( pos < w ) {
		*a = pos - 1;
		*b = ( pos >= 2 ) ? ( pos - 2 ) : *a;
	}
	else {
		*a = pos - 1;
		*b = pos - w;
	}
}

void pjg_aavrg_prepare( const CodecContext& ctx, int* offsets, int* weights, int cmp )
{
	const int w = ctx.cmpnfo[ cmp ].bch;

	offsets[ 0 ] =  0 + ( ( -2 ) * w ); // top-top
	offsets[ 1 ] = -1 + ( ( -1 ) * w ); // top-left
	offsets[ 2 ] =  0 + ( ( -1 ) * w ); // top
	offsets[ 3 ] =  1 + ( ( -1 ) * w ); // top-right
	offsets[ 4 ] = -2 + ( (  0 ) * w ); // left-left
	offsets[ 5 ] = -1 + ( (  0 ) * w ); // left

	weights[ 0 ] = abs_ctx_weights_lum[ 0 ][ 0 ][ 2 ];
	weights[ 1 ] = abs_ctx_weights_lum[ 0 ][ 1 ][ 1 ];
	weights[ 2 ] = abs_ctx_weights_lum[ 0 ][ 1 ][ 2 ];
	weights[ 3 ] = abs_ctx_weights_lum[ 0 ][ 1 ][ 3 ];
	weights[ 4 ] = abs_ctx_weights_lum[ 0 ][ 2 ][ 0 ];
	weights[ 5 ] = abs_ctx_weights_lum[ 0 ][ 2 ][ 1 ];
}

inline int pjg_aavrg_context( const unsigned short* abs_store, const int* offsets, const int* weights, int pos, int p_y, int p_x, int r_x )
{
	constexpr int kW0   = abs_ctx_weights_lum[ 0 ][ 0 ][ 2 ];
	constexpr int kW1   = abs_ctx_weights_lum[ 0 ][ 1 ][ 1 ];
	constexpr int kW2   = abs_ctx_weights_lum[ 0 ][ 1 ][ 2 ];
	constexpr int kW3   = abs_ctx_weights_lum[ 0 ][ 1 ][ 3 ];
	constexpr int kW4   = abs_ctx_weights_lum[ 0 ][ 2 ][ 0 ];
	constexpr int kW5   = abs_ctx_weights_lum[ 0 ][ 2 ][ 1 ];
	constexpr unsigned int kWAll = static_cast<unsigned int>( kW0 + kW1 + kW2 + kW3 + kW4 + kW5 );

	if ( ( p_y >= 2 ) && ( p_x >= 2 ) && ( r_x >= 1 ) ) {
		const unsigned int ctx_avr =
			static_cast<unsigned int>( abs_store[ pos + offsets[ 0 ] ] ) * kW0 +
			static_cast<unsigned int>( abs_store[ pos + offsets[ 1 ] ] ) * kW1 +
			static_cast<unsigned int>( abs_store[ pos + offsets[ 2 ] ] ) * kW2 +
			static_cast<unsigned int>( abs_store[ pos + offsets[ 3 ] ] ) * kW3 +
			static_cast<unsigned int>( abs_store[ pos + offsets[ 4 ] ] ) * kW4 +
			static_cast<unsigned int>( abs_store[ pos + offsets[ 5 ] ] ) * kW5;
		return static_cast<int>( ( ctx_avr + ( kWAll / 2u ) ) / kWAll );
	}

	int ctx_avr = 0;
	int w_ctx   = 0;
	int w_curr  = 0;

	if ( p_y >= 2 ) {
		w_curr = weights[ 0 ]; ctx_avr += abs_store[ pos + offsets[ 0 ] ] * w_curr; w_ctx += w_curr;
		w_curr = weights[ 2 ]; ctx_avr += abs_store[ pos + offsets[ 2 ] ] * w_curr; w_ctx += w_curr;
		if ( p_x >= 2 ) {
			w_curr = weights[ 1 ]; ctx_avr += abs_store[ pos + offsets[ 1 ] ] * w_curr; w_ctx += w_curr;
			w_curr = weights[ 4 ]; ctx_avr += abs_store[ pos + offsets[ 4 ] ] * w_curr; w_ctx += w_curr;
			w_curr = weights[ 5 ]; ctx_avr += abs_store[ pos + offsets[ 5 ] ] * w_curr; w_ctx += w_curr;
		}
		else if ( p_x == 1 ) {
			w_curr = weights[ 1 ]; ctx_avr += abs_store[ pos + offsets[ 1 ] ] * w_curr; w_ctx += w_curr;
			w_curr = weights[ 5 ]; ctx_avr += abs_store[ pos + offsets[ 5 ] ] * w_curr; w_ctx += w_curr;
		}
		if ( r_x >= 1 ) {
			w_curr = weights[ 3 ]; ctx_avr += abs_store[ pos + offsets[ 3 ] ] * w_curr; w_ctx += w_curr;
		}
	}
	else if ( p_y == 1 ) {
		w_curr = weights[ 2 ]; ctx_avr += abs_store[ pos + offsets[ 2 ] ] * w_curr; w_ctx += w_curr;
		if ( p_x >= 2 ) {
			w_curr = weights[ 1 ]; ctx_avr += abs_store[ pos + offsets[ 1 ] ] * w_curr; w_ctx += w_curr;
			w_curr = weights[ 4 ]; ctx_avr += abs_store[ pos + offsets[ 4 ] ] * w_curr; w_ctx += w_curr;
			w_curr = weights[ 5 ]; ctx_avr += abs_store[ pos + offsets[ 5 ] ] * w_curr; w_ctx += w_curr;
		}
		else if ( p_x == 1 ) {
			w_curr = weights[ 1 ]; ctx_avr += abs_store[ pos + offsets[ 1 ] ] * w_curr; w_ctx += w_curr;
			w_curr = weights[ 5 ]; ctx_avr += abs_store[ pos + offsets[ 5 ] ] * w_curr; w_ctx += w_curr;
		}
		if ( r_x >= 1 ) {
			w_curr = weights[ 3 ]; ctx_avr += abs_store[ pos + offsets[ 3 ] ] * w_curr; w_ctx += w_curr;
		}
	}
	else {
		if ( p_x >= 2 ) {
			w_curr = weights[ 4 ]; ctx_avr += abs_store[ pos + offsets[ 4 ] ] * w_curr; w_ctx += w_curr;
			w_curr = weights[ 5 ]; ctx_avr += abs_store[ pos + offsets[ 5 ] ] * w_curr; w_ctx += w_curr;
		}
		else if ( p_x == 1 ) {
			w_curr = weights[ 5 ]; ctx_avr += abs_store[ pos + offsets[ 5 ] ] * w_curr; w_ctx += w_curr;
		}
	}

	return ( w_ctx != 0 ) ? ( ctx_avr + ( w_ctx / 2 ) ) / w_ctx : 0;
}

int pjg_lakh_context( const short* const* coeffs_x, int neighbor_off, const int* pred_cf, int pos )
{
	int pred = 0;
	const int npos = pos - neighbor_off;

	pred -= ( coeffs_x[ 1 ][ pos ] + coeffs_x[ 1 ][ npos ] ) * pred_cf[ 1 ];
	pred -= ( coeffs_x[ 2 ][ pos ] - coeffs_x[ 2 ][ npos ] ) * pred_cf[ 2 ];
	pred -= ( coeffs_x[ 3 ][ pos ] + coeffs_x[ 3 ][ npos ] ) * pred_cf[ 3 ];
	pred -= ( coeffs_x[ 4 ][ pos ] - coeffs_x[ 4 ][ npos ] ) * pred_cf[ 4 ];
	pred -= ( coeffs_x[ 5 ][ pos ] + coeffs_x[ 5 ][ npos ] ) * pred_cf[ 5 ];
	pred -= ( coeffs_x[ 6 ][ pos ] - coeffs_x[ 6 ][ npos ] ) * pred_cf[ 6 ];
	pred -= ( coeffs_x[ 7 ][ pos ] + coeffs_x[ 7 ][ npos ] ) * pred_cf[ 7 ];
	pred = ( ( pred > 0 ) ? ( pred + ( pred_cf[ 0 ] / 2 ) ) : ( pred - ( pred_cf[ 0 ] / 2 ) ) ) / pred_cf[ 0 ];
	pred += coeffs_x[ 0 ][ npos ];

	return pred;
}

void pjg_get_zerosort_scan( const CodecContext& ctx, unsigned char* sv, int cmp )
{
	unsigned int zdist[ 64 ];
	const int bc = ctx.cmpnfo[ cmp ].bc;

	for ( int i = 0; i < 64; i++ ) {
		sv[ i ]    = static_cast<unsigned char>( i );
		zdist[ i ] = 0;
	}

	for ( int bpos = 0; bpos < 64; bpos++ ) {
		const short* band = ctx.colldata[ cmp ][ bpos ].data();
		for ( int dpos = 0; dpos < bc; dpos++ )
			if ( band[ dpos ] == 0 ) zdist[ bpos ]++;
	}

	bool done = false;
	while ( !done ) {
		done = true;
		for ( int i = 2; i < 64; i++ ) {
			if ( zdist[ i ] < zdist[ i - 1 ] ) {
				std::swap( zdist[ i ], zdist[ i - 1 ] );
				std::swap( sv[ i ], sv[ i - 1 ] );
				done = false;
			}
		}
	}
}

bool pjg_optimize_header( CodecContext& ctx )
{
	unsigned char type = 0x00;
	unsigned int  len  = 0;
	unsigned int  hpos = 0;

	while ( hpos < ctx.hdrdata.size() ) {
		if ( !ctx.read_header_segment( hpos, type, len ) ) return false;
		if ( type == 0xC4 ) {
			const unsigned int fpos = hpos + len;
			hpos += 4;
			while ( hpos < fpos ) {
				if ( hpos + 17 > fpos ) {
					ctx.errormessage = "size mismatch in dht marker";
					ctx.errorlevel = 2;
					return false;
				}
				hpos++;
				for ( int i = 0; i < 4; i++ ) {
					if ( hpos + std_huff_lengths[ i ] > fpos )
						continue;
					unsigned int spos = 0;
					for ( ; spos < std_huff_lengths[ i ]; spos++ ) {
						if ( ctx.hdrdata[ hpos + spos ] != std_huff_tables[ i ][ spos ] )
							break;
					}
					if ( spos != std_huff_lengths[ i ] )
						continue;

					ctx.hdrdata[ hpos + 0 ] = static_cast<unsigned char>( std_huff_lengths[ i ] - 16 - i );
					ctx.hdrdata[ hpos + 1 ] = static_cast<unsigned char>( i );
					for ( spos = 2; spos < std_huff_lengths[ i ]; spos++ )
						ctx.hdrdata[ hpos + spos ] = 0x00;
					break;
				}

				unsigned int skip = 16;
				for ( int i = 0; i < 16; i++ )
					skip += static_cast<unsigned int>( ctx.hdrdata[ hpos + i ] );
				if ( hpos + skip > fpos ) {
					ctx.errormessage = "size mismatch in dht marker";
					ctx.errorlevel = 2;
					return false;
				}
				hpos += skip;
			}
		}
		else if ( type == 0xDB ) {
			const unsigned int fpos = hpos + len;
			hpos += 4;
			while ( hpos < fpos ) {
				const int prec = static_cast<int>( lbits_u8( ctx.hdrdata[ hpos ], 4 ) );
				hpos++;
				if ( prec == 1 ) {
					if ( hpos + 128 > fpos ) {
						ctx.errormessage = "size mismatch in dqt marker";
						ctx.errorlevel = 2;
						return false;
					}
					hpos += 128;
					continue;
				}
				if ( hpos + 64 > fpos ) {
					ctx.errormessage = "size mismatch in dqt marker";
					ctx.errorlevel = 2;
					return false;
				}
				for ( unsigned int spos = 63; spos > 0; spos-- )
					ctx.hdrdata[ hpos + spos ] -= ctx.hdrdata[ hpos + spos - 1 ];
				hpos += 64;
			}
		}
		else {
			hpos += len;
		}
	}

	return true;
}

bool pjg_unoptimize_header( CodecContext& ctx )
{
	unsigned char type = 0x00;
	unsigned int  len  = 0;
	unsigned int  hpos = 0;

	while ( hpos < ctx.hdrdata.size() ) {
		if ( !ctx.read_header_segment( hpos, type, len ) ) return false;

		if ( type == 0xC4 ) {
			const unsigned int fpos = hpos + len;
			hpos += 4;
			while ( hpos < fpos ) {
				if ( hpos + 17 > fpos ) {
					ctx.errormessage = "size mismatch in dht marker";
					ctx.errorlevel = 2;
					return false;
				}
				hpos++;
				if ( ctx.hdrdata[ hpos ] > 2 ) {
					const int i = ctx.hdrdata[ hpos + 1 ];
					if ( ( i < 0 ) || ( i >= 4 ) || ( hpos + std_huff_lengths[ i ] > fpos ) ) {
						ctx.errormessage = "size mismatch in dht marker";
						ctx.errorlevel = 2;
						return false;
					}
					for ( unsigned int spos = 0; spos < std_huff_lengths[ i ]; spos++ ) {
						ctx.hdrdata[ hpos + spos ] = std_huff_tables[ i ][ spos ];
					}
				}

				unsigned int skip = 16;
				for ( int i = 0; i < 16; i++ )
					skip += static_cast<unsigned int>( ctx.hdrdata[ hpos + i ] );
				if ( hpos + skip > fpos ) {
					ctx.errormessage = "size mismatch in dht marker";
					ctx.errorlevel = 2;
					return false;
				}
				hpos += skip;
			}
		}
		else if ( type == 0xDB ) {
			const unsigned int fpos = hpos + len;
			hpos += 4;
			while ( hpos < fpos ) {
				const int prec = static_cast<int>( lbits_u8( ctx.hdrdata[ hpos ], 4 ) );
				hpos++;
				if ( prec == 1 ) {
					if ( hpos + 128 > fpos ) {
						ctx.errormessage = "size mismatch in dqt marker";
						ctx.errorlevel = 2;
						return false;
					}
					hpos += 128;
					continue;
				}
				if ( hpos + 64 > fpos ) {
					ctx.errormessage = "size mismatch in dqt marker";
					ctx.errorlevel = 2;
					return false;
				}
				for ( unsigned int spos = 1; spos < 64; spos++ )
					ctx.hdrdata[ hpos + spos ] += ctx.hdrdata[ hpos + spos - 1 ];
				hpos += 64;
			}
		}
		else {
			hpos += len;
		}
	}

	return true;
}

bool pjg_encode_zstscan( CodecContext& ctx, ArithmeticEncoder* enc, int cmp )
{
	unsigned char freqlist[ 64 ];

	pjg_get_zerosort_scan( ctx, ctx.zsrtscan[ cmp ], cmp );

	for ( int i = 0; i < 64; i++ )
		freqlist[ i ] = stdscan[ i ];

	auto model = init_model_s( 64, 64, 1 );

	for ( int i = 1; i < 64; i++ ) {
		model->exclude_symbols( 64 - i );

		int tpos = 0;
		int c = i;
		for ( ; c < 64; c++ ) {
			for ( tpos++; freqlist[ tpos ] == 0; tpos++ );
			if ( freqlist[ tpos ] != ctx.zsrtscan[ cmp ][ c ] ) break;
		}
		if ( c == 64 ) {
			encode_ari( enc, model, 0 );
			break;
		}

		int cpos = 1;
		for ( tpos = 0; freqlist[ tpos ] != ctx.zsrtscan[ cmp ][ i ]; tpos++ )
			if ( freqlist[ tpos ] != 0 ) cpos++;
		freqlist[ tpos ] = 0;

		encode_ari( enc, model, cpos );
		model->shift_context( cpos );
	}

	ctx.freqscan[ cmp ] = ctx.zsrtscan[ cmp ];
	return true;
}

bool pjg_encode_zdst_high( const CodecContext& ctx, ArithmeticEncoder* enc, int cmp )
{
	auto model = init_model_s( 49 + 1, 25 + 1, 1 );
	const unsigned char* zdstls = ctx.zdstdata[ cmp ].data();
	const int w  = ctx.cmpnfo[ cmp ].bch;
	const int bc = ctx.cmpnfo[ cmp ].bc;

	for ( int dpos = 0; dpos < bc; dpos++ ) {
		int a = 0;
		int b = 0;
		get_context_nnb( dpos, w, &a, &b );
		a = ( a >= 0 ) ? zdstls[ a ] : 0;
		b = ( b >= 0 ) ? zdstls[ b ] : 0;
		model->shift_context( ( a + b + 2 ) / 4 );
		encode_ari( enc, model, zdstls[ dpos ] );
	}

	return true;
}

bool pjg_encode_zdst_low( const CodecContext& ctx, ArithmeticEncoder* enc, int cmp )
{
	auto model = init_model_s( 8, 8, 2 );
	const unsigned char* zdstls_x = ctx.zdstxlow[ cmp ].data();
	const unsigned char* zdstls_y = ctx.zdstylow[ cmp ].data();
	const unsigned char* ctx_eobx = ctx.eobxhigh[ cmp ].data();
	const unsigned char* ctx_eoby = ctx.eobyhigh[ cmp ].data();
	const unsigned char* ctx_zdst = ctx.zdstdata[ cmp ].data();
	const int bc = ctx.cmpnfo[ cmp ].bc;

	for ( int dpos = 0; dpos < bc; dpos++ ) {
		model->shift_context( ( ctx_zdst[ dpos ] + 3 ) / 7 );
		model->shift_context( ctx_eobx[ dpos ] );
		encode_ari( enc, model, zdstls_x[ dpos ] );
	}
	for ( int dpos = 0; dpos < bc; dpos++ ) {
		model->shift_context( ( ctx_zdst[ dpos ] + 3 ) / 7 );
		model->shift_context( ctx_eoby[ dpos ] );
		encode_ari( enc, model, zdstls_y[ dpos ] );
	}

	return true;
}

bool pjg_encode_dc( const CodecContext& ctx, ArithmeticEncoder* enc, int cmp )
{
	const unsigned char* segm_tab = segm_tables[ ctx.segm_cnt[ cmp ] - 1 ];

	const int max_val = ctx.max_v( cmp, 0 );
	const int max_len = bitlen_1024p( max_val );

	auto mod_len = init_model_s( max_len + 1, ( ctx.segm_cnt[ cmp ] > max_len ) ? ctx.segm_cnt[ cmp ] : max_len + 1, 2 );
	auto mod_res = init_model_b( ( ctx.segm_cnt[ cmp ] < 16 ) ? ( 1 << 4 ) : ctx.segm_cnt[ cmp ], 2 );
	auto mod_sgn = init_model_b( 1, 0 );

	const int bc = ctx.cmpnfo[ cmp ].bc;
	const int w  = ctx.cmpnfo[ cmp ].bch;

	std::vector<unsigned short> absv_store_vec( static_cast<std::size_t>( bc ), 0 );
	unsigned short* absv_store = absv_store_vec.data();

	int c_absc_off[ 6 ];
	int c_weight[ 6 ];
	pjg_aavrg_prepare( ctx, c_absc_off, c_weight, cmp );

	const short* coeffs = ctx.colldata[ cmp ][ 0 ].data();
	const unsigned char* zdstls = ctx.zdstdata[ cmp ].data();

	for ( int dpos = 0, p_y = 0; dpos < bc; ++p_y ) {
		const int row_w = std::min( w, bc - dpos );
		for ( int p_x = 0; p_x < row_w; ++p_x, ++dpos ) {
			const int r_x = w - ( p_x + 1 );

			const int snum    = segm_tab[ zdstls[ dpos ] ];
			const int ctx_avr = pjg_aavrg_context( absv_store, c_absc_off, c_weight, dpos, p_y, p_x, r_x );
			const int ctx_len = bitlen_1024p( ctx_avr );
			shift_model( mod_len, ctx_len, snum );

			if ( coeffs[ dpos ] == 0 ) {
				encode_ari( enc, mod_len, 0 );
			}
			else {
				const int absv = abs_val( coeffs[ dpos ] );
				const int clen = bitlen_1024p( absv );
				const int sgn  = ( coeffs[ dpos ] > 0 ) ? 0 : 1;
				encode_ari( enc, mod_len, clen );
				for ( int bp = clen - 2; bp >= 0; bp-- ) {
					shift_model( mod_res, snum, bp );
					const int bt = bit_n( absv, bp );
					encode_ari( enc, mod_res, bt );
				}
				encode_ari( enc, mod_sgn, sgn );
				absv_store[ dpos ] = static_cast<unsigned short>( absv );
			}
		}
	}

	return true;
}

bool pjg_encode_ac_high( CodecContext& ctx, ArithmeticEncoder* enc, int cmp )
{
	const unsigned char* segm_tab = segm_tables[ ctx.segm_cnt[ cmp ] - 1 ];

	auto mod_len = init_model_s( 11, ( ctx.segm_cnt[ cmp ] > 11 ) ? ctx.segm_cnt[ cmp ] : 11, 2 );
	auto mod_res = init_model_b( ( ctx.segm_cnt[ cmp ] < 16 ) ? ( 1 << 4 ) : ctx.segm_cnt[ cmp ], 2 );
	auto mod_sgn = init_model_b( 9, 1 );

	const int bc = ctx.cmpnfo[ cmp ].bc;
	const int w  = ctx.cmpnfo[ cmp ].bch;

	std::vector<unsigned short> absv_store_vec( static_cast<std::size_t>( bc ), 0 );
	std::vector<unsigned char>  sgn_store_vec( static_cast<std::size_t>( bc ), 0 );
	std::vector<unsigned char>  zdstls_vec = ctx.zdstdata[ cmp ];
	unsigned short* absv_store = absv_store_vec.data();
	unsigned char*  sgn_store  = sgn_store_vec.data();
	unsigned char*  zdstls     = zdstls_vec.data();

	unsigned char* eob_x = ctx.eobxhigh[ cmp ].data();
	unsigned char* eob_y = ctx.eobyhigh[ cmp ].data();
	std::memset( eob_x, 0x00, static_cast<std::size_t>( bc ) );
	std::memset( eob_y, 0x00, static_cast<std::size_t>( bc ) );

	int c_absc_off[ 6 ];
	int c_weight[ 6 ];

	for ( int i = 1; i < 64; i++ ) {
		const int bpos = static_cast<int>( ctx.freqscan[ cmp ][ i ] );
		const int b_x  = unzigzag[ bpos ] % 8;
		const int b_y  = unzigzag[ bpos ] / 8;

		if ( ( b_x == 0 ) || ( b_y == 0 ) )
			continue;

		std::memset( absv_store, 0x00, static_cast<std::size_t>( bc ) * sizeof( unsigned short ) );
		std::memset( sgn_store, 0x00, static_cast<std::size_t>( bc ) );

		pjg_aavrg_prepare( ctx, c_absc_off, c_weight, cmp );

		const short* coeffs = ctx.colldata[ cmp ][ bpos ].data();
		const int max_val = ctx.max_v( cmp, bpos );
		const int max_len = bitlen_1024p( max_val );

		for ( int dpos = 0, p_y = 0; dpos < bc; ++p_y ) {
			const int row_w = std::min( w, bc - dpos );
			for ( int p_x = 0; p_x < row_w; ++p_x, ++dpos ) {
				if ( zdstls[ dpos ] == 0 )
					continue;

				const int r_x = w - ( p_x + 1 );

				const int snum    = segm_tab[ zdstls[ dpos ] ];
				const int ctx_avr = pjg_aavrg_context( absv_store, c_absc_off, c_weight, dpos, p_y, p_x, r_x );
				const int ctx_len = bitlen_1024p( ctx_avr );
				shift_model( mod_len, ctx_len, snum );
				mod_len->exclude_symbols( max_len );

				if ( coeffs[ dpos ] == 0 ) {
					encode_ari( enc, mod_len, 0 );
				}
				else {
					const int absv = abs_val( coeffs[ dpos ] );
					const int clen = bitlen_1024p( absv );
					const int sgn  = ( coeffs[ dpos ] > 0 ) ? 0 : 1;
					encode_ari( enc, mod_len, clen );
					for ( int bp = clen - 2; bp >= 0; bp-- ) {
						shift_model( mod_res, snum, bp );
						const int bt = bit_n( absv, bp );
						encode_ari( enc, mod_res, bt );
					}
					int ctx_sgn = ( p_x > 0 ) ? sgn_store[ dpos - 1 ] : 0;
					if ( p_y > 0 ) ctx_sgn += 3 * sgn_store[ dpos - w ];
					mod_sgn->shift_context( ctx_sgn );
					encode_ari( enc, mod_sgn, sgn );

					absv_store[ dpos ] = static_cast<unsigned short>( absv );
					sgn_store[ dpos ]  = static_cast<unsigned char>( sgn + 1 );
					zdstls[ dpos ]--;
					if ( b_x > eob_x[ dpos ] ) eob_x[ dpos ] = static_cast<unsigned char>( b_x );
					if ( b_y > eob_y[ dpos ] ) eob_y[ dpos ] = static_cast<unsigned char>( b_y );
				}
			}
		}

		mod_len->flush_model();
		mod_res->flush_model();
		mod_sgn->flush_model();
	}

	return true;
}

bool pjg_encode_ac_low( CodecContext& ctx, ArithmeticEncoder* enc, int cmp )
{
	const short* coeffs_x[ 8 ];
	int pred_cf[ 8 ];

	auto mod_len = init_model_s( 11, ( ctx.segm_cnt[ cmp ] > 11 ) ? ctx.segm_cnt[ cmp ] : 11, 2 );
	auto mod_res = init_model_b( 1 << 4, 2 );
	auto mod_top = init_model_b( ( ctx.nois_trs[ cmp ] > 4 ) ? ( 1 << ctx.nois_trs[ cmp ] ) : ( 1 << 4 ), 3 );
	auto mod_sgn = init_model_b( 11, 1 );

	const int bc = ctx.cmpnfo[ cmp ].bc;
	const int w  = ctx.cmpnfo[ cmp ].bch;

	for ( int i = 2; i < 16; i++ ) {
		int b_x = ( i % 2 == 0 ) ? ( i / 2 ) : 0;
		int b_y = ( i % 2 == 1 ) ? ( i / 2 ) : 0;
		const int bpos = static_cast<int>( zigzag[ b_x + ( 8 * b_y ) ] );

		const short* coeffs = ctx.colldata[ cmp ][ bpos ].data();
		int neighbor_off = 0;
		unsigned char* zdstls = nullptr;
		const bool check_x = ( b_x == 0 );

		if ( check_x ) {
			for ( ; b_x < 8; b_x++ ) {
				coeffs_x[ b_x ] = ctx.colldata[ cmp ][ zigzag[ b_x + ( 8 * b_y ) ] ].data();
				pred_cf[ b_x ]  = icos_base_8x8[ b_x * 8 ] * ctx.quant( cmp, zigzag[ b_x + ( 8 * b_y ) ] );
			}
			b_x = 0;
			neighbor_off = 1;
			zdstls = ctx.zdstylow[ cmp ].data();
		}
		else {
			for ( ; b_y < 8; b_y++ ) {
				coeffs_x[ b_y ] = ctx.colldata[ cmp ][ zigzag[ b_x + ( 8 * b_y ) ] ].data();
				pred_cf[ b_y ]  = icos_base_8x8[ b_y * 8 ] * ctx.quant( cmp, zigzag[ b_x + ( 8 * b_y ) ] );
			}
			b_y = 0;
			neighbor_off = w;
			zdstls = ctx.zdstxlow[ cmp ].data();
		}

		const int max_valp = ctx.max_v( cmp, bpos );
		const int max_valn = -max_valp;
		const int max_len  = bitlen_1024p( max_valp );
		const int thrs_bp  = ( max_len > ctx.nois_trs[ cmp ] ) ? ( max_len - ctx.nois_trs[ cmp ] ) : 0;

		for ( int dpos = 0, p_y = 0; dpos < bc; ++p_y ) {
			const int row_w = std::min( w, bc - dpos );
			for ( int p_x = 0; p_x < row_w; ++p_x, ++dpos ) {
				if ( zdstls[ dpos ] == 0 )
					continue;

				int ctx_lak = ( ( check_x ? p_x : p_y ) > 0 )
					? pjg_lakh_context( coeffs_x, neighbor_off, pred_cf, dpos )
					: 0;
				ctx_lak = clamped( max_valn, max_valp, ctx_lak );
				const int ctx_len = bitlen_2048n( ctx_lak );

				shift_model( mod_len, ctx_len, zdstls[ dpos ] );
				mod_len->exclude_symbols( max_len );

				if ( coeffs[ dpos ] == 0 ) {
					encode_ari( enc, mod_len, 0 );
				}
				else {
					const int absv = abs_val( coeffs[ dpos ] );
					const int clen = bitlen_2048n( absv );
					const int sgn  = ( coeffs[ dpos ] > 0 ) ? 0 : 1;
					encode_ari( enc, mod_len, clen );
					int bp = clen - 2;
					int ctx_res = ( bp >= thrs_bp ) ? 1 : 0;
					const int ctx_abs = abs_val( ctx_lak );
					const int ctx_sgn = ( ctx_lak == 0 ) ? 0 : ( ( ctx_lak > 0 ) ? 1 : 2 );
					for ( ; bp >= thrs_bp; bp-- ) {
						shift_model( mod_top, ctx_abs >> thrs_bp, ctx_res, clen - thrs_bp );
						const int bt = bit_n( absv, bp );
						encode_ari( enc, mod_top, bt );
						ctx_res = ( ctx_res << 1 ) | ( bt ? 1 : 0 );
					}
					for ( ; bp >= 0; bp-- ) {
						shift_model( mod_res, zdstls[ dpos ], bp );
						const int bt = bit_n( absv, bp );
						encode_ari( enc, mod_res, bt );
					}
					shift_model( mod_sgn, ctx_len, ctx_sgn );
					encode_ari( enc, mod_sgn, sgn );
					zdstls[ dpos ]--;
				}
			}
		}

		mod_len->flush_model();
		mod_res->flush_model();
		mod_top->flush_model();
		mod_sgn->flush_model();
	}

	return true;
}

bool pjg_encode_generic( ArithmeticEncoder* enc, const unsigned char* data, std::size_t len )
{
	auto model = init_model_s( 256 + 1, 256, 1 );
	for ( std::size_t i = 0; i < len; i++ ) {
		encode_ari( enc, model, data[ i ] );
		model->shift_context( data[ i ] );
	}
	encode_ari( enc, model, 256 );
	return true;
}

bool pjg_encode_bit( ArithmeticEncoder* enc, unsigned char bit )
{
	auto model = init_model_b( 1, -1 );
	encode_ari( enc, model, bit );
	return true;
}

bool pjg_decode_zstscan( CodecContext& ctx, ArithmeticDecoder* dec, int cmp )
{
	unsigned char freqlist[ 64 ];

	ctx.zsrtscan[ cmp ][ 0 ] = 0;
	for ( int i = 0; i < 64; i++ )
		freqlist[ i ] = stdscan[ i ];

	auto model = init_model_s( 64, 64, 1 );

	for ( int i = 1; i < 64; i++ ) {
		model->exclude_symbols( 64 - i );

		int cpos = decode_ari( dec, model );
		model->shift_context( cpos );

		if ( cpos == 0 ) {
			for ( int tpos = 0; i < 64; i++ ) {
				while ( ( ++tpos < 64 ) && ( freqlist[ tpos ] == 0 ) );
				if ( tpos >= 64 ) {
					throw std::runtime_error( "corrupted arithmetic stream" );
				}
				ctx.zsrtscan[ cmp ][ i ] = freqlist[ tpos ];
			}
			break;
		}

		int tpos = 0;
		for ( ; tpos < 64; tpos++ ) {
			if ( freqlist[ tpos ] != 0 ) cpos--;
			if ( cpos == 0 ) break;
		}
		if ( tpos >= 64 ) {
			throw std::runtime_error( "corrupted arithmetic stream" );
		}

		ctx.zsrtscan[ cmp ][ i ] = freqlist[ tpos ];
		freqlist[ tpos ] = 0;
	}

	ctx.freqscan[ cmp ] = ctx.zsrtscan[ cmp ];
	return true;
}

bool pjg_decode_zdst_high( CodecContext& ctx, ArithmeticDecoder* dec, int cmp )
{
	auto model = init_model_s( 49 + 1, 25 + 1, 1 );
	unsigned char* zdstls = ctx.zdstdata[ cmp ].data();
	const int w  = ctx.cmpnfo[ cmp ].bch;
	const int bc = ctx.cmpnfo[ cmp ].bc;

	for ( int dpos = 0; dpos < bc; dpos++ ) {
		int a = 0;
		int b = 0;
		get_context_nnb( dpos, w, &a, &b );
		a = ( a >= 0 ) ? zdstls[ a ] : 0;
		b = ( b >= 0 ) ? zdstls[ b ] : 0;
		model->shift_context( ( a + b + 2 ) / 4 );
		zdstls[ dpos ] = static_cast<unsigned char>( decode_ari( dec, model ) );
	}

	return true;
}

bool pjg_decode_zdst_low( CodecContext& ctx, ArithmeticDecoder* dec, int cmp )
{
	auto model = init_model_s( 8, 8, 2 );
	unsigned char* zdstls_x = ctx.zdstxlow[ cmp ].data();
	unsigned char* zdstls_y = ctx.zdstylow[ cmp ].data();
	const unsigned char* ctx_eobx = ctx.eobxhigh[ cmp ].data();
	const unsigned char* ctx_eoby = ctx.eobyhigh[ cmp ].data();
	const unsigned char* ctx_zdst = ctx.zdstdata[ cmp ].data();
	const int bc = ctx.cmpnfo[ cmp ].bc;

	for ( int dpos = 0; dpos < bc; dpos++ ) {
		model->shift_context( ( ctx_zdst[ dpos ] + 3 ) / 7 );
		model->shift_context( ctx_eobx[ dpos ] );
		zdstls_x[ dpos ] = static_cast<unsigned char>( decode_ari( dec, model ) );
	}
	for ( int dpos = 0; dpos < bc; dpos++ ) {
		model->shift_context( ( ctx_zdst[ dpos ] + 3 ) / 7 );
		model->shift_context( ctx_eoby[ dpos ] );
		zdstls_y[ dpos ] = static_cast<unsigned char>( decode_ari( dec, model ) );
	}

	return true;
}

bool pjg_decode_dc( CodecContext& ctx, ArithmeticDecoder* dec, int cmp )
{
	const unsigned char* segm_tab = segm_tables[ ctx.segm_cnt[ cmp ] - 1 ];

	const int max_val = ctx.max_v( cmp, 0 );
	const int max_len = bitlen_1024p( max_val );

	auto mod_len = init_model_s( max_len + 1, ( ctx.segm_cnt[ cmp ] > max_len ) ? ctx.segm_cnt[ cmp ] : max_len + 1, 2 );
	auto mod_res = init_model_b( ( ctx.segm_cnt[ cmp ] < 16 ) ? ( 1 << 4 ) : ctx.segm_cnt[ cmp ], 2 );
	auto mod_sgn = init_model_b( 1, 0 );

	const int bc = ctx.cmpnfo[ cmp ].bc;
	const int w  = ctx.cmpnfo[ cmp ].bch;

	std::vector<unsigned short> absv_store_vec( static_cast<std::size_t>( bc ), 0 );
	unsigned short* absv_store = absv_store_vec.data();

	int c_absc_off[ 6 ];
	int c_weight[ 6 ];
	pjg_aavrg_prepare( ctx, c_absc_off, c_weight, cmp );

	short* coeffs = ctx.colldata[ cmp ][ 0 ].data();
	const unsigned char* zdstls = ctx.zdstdata[ cmp ].data();

	for ( int dpos = 0, p_y = 0; dpos < bc; ++p_y ) {
		const int row_w = std::min( w, bc - dpos );
		for ( int p_x = 0; p_x < row_w; ++p_x, ++dpos ) {
			const int r_x = w - ( p_x + 1 );

			const int snum    = segm_tab[ zdstls[ dpos ] ];
			const int ctx_avr = pjg_aavrg_context( absv_store, c_absc_off, c_weight, dpos, p_y, p_x, r_x );
			const int ctx_len = bitlen_1024p( ctx_avr );
			shift_model( mod_len, ctx_len, snum );
			const int clen = decode_ari( dec, mod_len );

			if ( clen != 0 ) {
				int absv = 1;
				for ( int bp = clen - 2; bp >= 0; bp-- ) {
					shift_model( mod_res, snum, bp );
					const int bt = decode_ari( dec, mod_res );
					absv = ( absv << 1 ) | ( bt ? 1 : 0 );
				}
				const int sgn = decode_ari( dec, mod_sgn );
				coeffs[ dpos ]     = static_cast<short>( ( sgn == 0 ) ? absv : -absv );
				absv_store[ dpos ] = static_cast<unsigned short>( absv );
			}
		}
	}

	return true;
}

bool pjg_decode_ac_high( CodecContext& ctx, ArithmeticDecoder* dec, int cmp )
{
	const unsigned char* segm_tab = segm_tables[ ctx.segm_cnt[ cmp ] - 1 ];

	auto mod_len = init_model_s( 11, ( ctx.segm_cnt[ cmp ] > 11 ) ? ctx.segm_cnt[ cmp ] : 11, 2 );
	auto mod_res = init_model_b( ( ctx.segm_cnt[ cmp ] < 16 ) ? ( 1 << 4 ) : ctx.segm_cnt[ cmp ], 2 );
	auto mod_sgn = init_model_b( 9, 1 );

	const int bc = ctx.cmpnfo[ cmp ].bc;
	const int w  = ctx.cmpnfo[ cmp ].bch;

	std::vector<unsigned short> absv_store_vec( static_cast<std::size_t>( bc ), 0 );
	std::vector<unsigned char>  sgn_store_vec( static_cast<std::size_t>( bc ), 0 );
	std::vector<unsigned char>  zdstls_vec = ctx.zdstdata[ cmp ];
	unsigned short* absv_store = absv_store_vec.data();
	unsigned char*  sgn_store  = sgn_store_vec.data();
	unsigned char*  zdstls     = zdstls_vec.data();

	unsigned char* eob_x = ctx.eobxhigh[ cmp ].data();
	unsigned char* eob_y = ctx.eobyhigh[ cmp ].data();
	std::memset( eob_x, 0x00, static_cast<std::size_t>( bc ) );
	std::memset( eob_y, 0x00, static_cast<std::size_t>( bc ) );

	int c_absc_off[ 6 ];
	int c_weight[ 6 ];

	for ( int i = 1; i < 64; i++ ) {
		const int bpos = static_cast<int>( ctx.freqscan[ cmp ][ i ] );
		const int b_x  = unzigzag[ bpos ] % 8;
		const int b_y  = unzigzag[ bpos ] / 8;

		if ( ( b_x == 0 ) || ( b_y == 0 ) )
			continue;

		std::memset( absv_store, 0x00, static_cast<std::size_t>( bc ) * sizeof( unsigned short ) );
		std::memset( sgn_store, 0x00, static_cast<std::size_t>( bc ) );

		pjg_aavrg_prepare( ctx, c_absc_off, c_weight, cmp );

		short* coeffs = ctx.colldata[ cmp ][ bpos ].data();
		const int max_val = ctx.max_v( cmp, bpos );
		const int max_len = bitlen_1024p( max_val );

		for ( int dpos = 0, p_y = 0; dpos < bc; ++p_y ) {
			const int row_w = std::min( w, bc - dpos );
			for ( int p_x = 0; p_x < row_w; ++p_x, ++dpos ) {
				if ( zdstls[ dpos ] == 0 )
					continue;

				const int r_x = w - ( p_x + 1 );

				const int snum    = segm_tab[ zdstls[ dpos ] ];
				const int ctx_avr = pjg_aavrg_context( absv_store, c_absc_off, c_weight, dpos, p_y, p_x, r_x );
				const int ctx_len = bitlen_1024p( ctx_avr );
				shift_model( mod_len, ctx_len, snum );
				mod_len->exclude_symbols( max_len );

				const int clen = decode_ari( dec, mod_len );
				if ( clen != 0 ) {
					int absv = 1;
					for ( int bp = clen - 2; bp >= 0; bp-- ) {
						shift_model( mod_res, snum, bp );
						const int bt = decode_ari( dec, mod_res );
						absv = ( absv << 1 ) | ( bt ? 1 : 0 );
					}
					int ctx_sgn = ( p_x > 0 ) ? sgn_store[ dpos - 1 ] : 0;
					if ( p_y > 0 ) ctx_sgn += 3 * sgn_store[ dpos - w ];
					mod_sgn->shift_context( ctx_sgn );
					const int sgn = decode_ari( dec, mod_sgn );

					coeffs[ dpos ]     = static_cast<short>( ( sgn == 0 ) ? absv : -absv );
					absv_store[ dpos ] = static_cast<unsigned short>( absv );
					sgn_store[ dpos ]  = static_cast<unsigned char>( sgn + 1 );
					zdstls[ dpos ]--;
					if ( b_x > eob_x[ dpos ] ) eob_x[ dpos ] = static_cast<unsigned char>( b_x );
					if ( b_y > eob_y[ dpos ] ) eob_y[ dpos ] = static_cast<unsigned char>( b_y );
				}
			}
		}

		mod_len->flush_model();
		mod_res->flush_model();
		mod_sgn->flush_model();
	}

	return true;
}

bool pjg_decode_ac_low( CodecContext& ctx, ArithmeticDecoder* dec, int cmp )
{
	const short* coeffs_x[ 8 ];
	int pred_cf[ 8 ];

	auto mod_len = init_model_s( 11, ( ctx.segm_cnt[ cmp ] > 11 ) ? ctx.segm_cnt[ cmp ] : 11, 2 );
	auto mod_res = init_model_b( 1 << 4, 2 );
	auto mod_top = init_model_b( ( ctx.nois_trs[ cmp ] > 4 ) ? ( 1 << ctx.nois_trs[ cmp ] ) : ( 1 << 4 ), 3 );
	auto mod_sgn = init_model_b( 11, 1 );

	const int bc = ctx.cmpnfo[ cmp ].bc;
	const int w  = ctx.cmpnfo[ cmp ].bch;

	for ( int i = 2; i < 16; i++ ) {
		int b_x = ( i % 2 == 0 ) ? ( i / 2 ) : 0;
		int b_y = ( i % 2 == 1 ) ? ( i / 2 ) : 0;
		const int bpos = static_cast<int>( zigzag[ b_x + ( 8 * b_y ) ] );

		short* coeffs = ctx.colldata[ cmp ][ bpos ].data();
		int neighbor_off = 0;
		unsigned char* zdstls = nullptr;
		const bool check_x = ( b_x == 0 );

		if ( check_x ) {
			for ( ; b_x < 8; b_x++ ) {
				coeffs_x[ b_x ] = ctx.colldata[ cmp ][ zigzag[ b_x + ( 8 * b_y ) ] ].data();
				pred_cf[ b_x ]  = icos_base_8x8[ b_x * 8 ] * ctx.quant( cmp, zigzag[ b_x + ( 8 * b_y ) ] );
			}
			b_x = 0;
			neighbor_off = 1;
			zdstls = ctx.zdstylow[ cmp ].data();
		}
		else {
			for ( ; b_y < 8; b_y++ ) {
				coeffs_x[ b_y ] = ctx.colldata[ cmp ][ zigzag[ b_x + ( 8 * b_y ) ] ].data();
				pred_cf[ b_y ]  = icos_base_8x8[ b_y * 8 ] * ctx.quant( cmp, zigzag[ b_x + ( 8 * b_y ) ] );
			}
			b_y = 0;
			neighbor_off = w;
			zdstls = ctx.zdstxlow[ cmp ].data();
		}

		const int max_valp = ctx.max_v( cmp, bpos );
		const int max_valn = -max_valp;
		const int max_len  = bitlen_1024p( max_valp );
		const int thrs_bp  = ( max_len > ctx.nois_trs[ cmp ] ) ? ( max_len - ctx.nois_trs[ cmp ] ) : 0;

		for ( int dpos = 0, p_y = 0; dpos < bc; ++p_y ) {
			const int row_w = std::min( w, bc - dpos );
			for ( int p_x = 0; p_x < row_w; ++p_x, ++dpos ) {
				if ( zdstls[ dpos ] == 0 )
					continue;

				int ctx_lak = ( ( check_x ? p_x : p_y ) > 0 )
					? pjg_lakh_context( coeffs_x, neighbor_off, pred_cf, dpos )
					: 0;
				ctx_lak = clamped( max_valn, max_valp, ctx_lak );
				const int ctx_len = bitlen_2048n( ctx_lak );

				shift_model( mod_len, ctx_len, zdstls[ dpos ] );
				mod_len->exclude_symbols( max_len );

				const int clen = decode_ari( dec, mod_len );
				if ( clen != 0 ) {
					int bp = clen - 2;
					int ctx_res = ( bp >= thrs_bp ) ? 1 : 0;
					const int ctx_abs = abs_val( ctx_lak );
					const int ctx_sgn = ( ctx_lak == 0 ) ? 0 : ( ( ctx_lak > 0 ) ? 1 : 2 );
					for ( ; bp >= thrs_bp; bp-- ) {
						shift_model( mod_top, ctx_abs >> thrs_bp, ctx_res, clen - thrs_bp );
						const int bt = decode_ari( dec, mod_top );
						ctx_res = ( ctx_res << 1 ) | ( bt ? 1 : 0 );
					}
					int absv = ( ctx_res == 0 ) ? 1 : ctx_res;
					for ( ; bp >= 0; bp-- ) {
						shift_model( mod_res, zdstls[ dpos ], bp );
						const int bt = decode_ari( dec, mod_res );
						absv = ( absv << 1 ) | ( bt ? 1 : 0 );
					}
					shift_model( mod_sgn, zdstls[ dpos ], ctx_sgn );
					const int sgn = decode_ari( dec, mod_sgn );
					coeffs[ dpos ] = static_cast<short>( ( sgn == 0 ) ? absv : -absv );
					zdstls[ dpos ]--;
				}
			}
		}

		mod_len->flush_model();
		mod_res->flush_model();
		mod_top->flush_model();
		mod_sgn->flush_model();
	}

	return true;
}

bool pjg_decode_generic( ArithmeticDecoder* dec, std::vector<unsigned char>& out, std::size_t max_bytes )
{
	MemoryWriter bwrt;

	auto model = init_model_s( 256 + 1, 256, 1 );
	while ( true ) {
		const int c = decode_ari( dec, model );
		if ( c == 256 ) break;
		if ( bwrt.num_bytes_written() >= max_bytes ) {
			return false;
		}
		bwrt.write_byte( static_cast<unsigned char>( c ) );
		model->shift_context( c );
	}

	out = bwrt.take_data();
	return true;
}

bool pjg_decode_bit( ArithmeticDecoder* dec, unsigned char* bit )
{
	auto model = init_model_b( 1, -1 );
	*bit = static_cast<unsigned char>( decode_ari( dec, model ) );
	return true;
}

int idct_2d_fst_8x1( const CodecContext& ctx, int cmp, int dpos, int ix )
{
	int idct = 0;
	const int ixy = ix << 3;

	idct += ctx.colldata[ cmp ][  0 ][ dpos ] * ctx.adpt_idct_8x1[ cmp ][ ixy + 0 ];
	idct += ctx.colldata[ cmp ][  1 ][ dpos ] * ctx.adpt_idct_8x1[ cmp ][ ixy + 1 ];
	idct += ctx.colldata[ cmp ][  5 ][ dpos ] * ctx.adpt_idct_8x1[ cmp ][ ixy + 2 ];
	idct += ctx.colldata[ cmp ][  6 ][ dpos ] * ctx.adpt_idct_8x1[ cmp ][ ixy + 3 ];
	idct += ctx.colldata[ cmp ][ 14 ][ dpos ] * ctx.adpt_idct_8x1[ cmp ][ ixy + 4 ];
	idct += ctx.colldata[ cmp ][ 15 ][ dpos ] * ctx.adpt_idct_8x1[ cmp ][ ixy + 5 ];
	idct += ctx.colldata[ cmp ][ 27 ][ dpos ] * ctx.adpt_idct_8x1[ cmp ][ ixy + 6 ];
	idct += ctx.colldata[ cmp ][ 28 ][ dpos ] * ctx.adpt_idct_8x1[ cmp ][ ixy + 7 ];

	return idct;
}

int idct_2d_fst_1x8( const CodecContext& ctx, int cmp, int dpos, int iy )
{
	int idct = 0;
	const int ixy = iy << 3;

	idct += ctx.colldata[ cmp ][  0 ][ dpos ] * ctx.adpt_idct_1x8[ cmp ][ ixy + 0 ];
	idct += ctx.colldata[ cmp ][  2 ][ dpos ] * ctx.adpt_idct_1x8[ cmp ][ ixy + 1 ];
	idct += ctx.colldata[ cmp ][  3 ][ dpos ] * ctx.adpt_idct_1x8[ cmp ][ ixy + 2 ];
	idct += ctx.colldata[ cmp ][  9 ][ dpos ] * ctx.adpt_idct_1x8[ cmp ][ ixy + 3 ];
	idct += ctx.colldata[ cmp ][ 10 ][ dpos ] * ctx.adpt_idct_1x8[ cmp ][ ixy + 4 ];
	idct += ctx.colldata[ cmp ][ 20 ][ dpos ] * ctx.adpt_idct_1x8[ cmp ][ ixy + 5 ];
	idct += ctx.colldata[ cmp ][ 21 ][ dpos ] * ctx.adpt_idct_1x8[ cmp ][ ixy + 6 ];
	idct += ctx.colldata[ cmp ][ 35 ][ dpos ] * ctx.adpt_idct_1x8[ cmp ][ ixy + 7 ];

	return idct;
}

#if defined(USE_PLOCOI)
inline int plocoi( int a, int b, int c )
{
	const int min_v = ( a < b ) ? a : b;
	const int max_v = ( a > b ) ? a : b;
	if ( c >= max_v ) return min_v;
	if ( c <= min_v ) return max_v;
	return a + b - c;
}

int dc_coll_predictor( const CodecContext& ctx, int cmp, int dpos )
{
	const short* coefs = ctx.colldata[ cmp ][ 0 ].data();
	const int w = ctx.cmpnfo[ cmp ].bch;
	int a = 0;
	int b = 0;
	int c = 0;

	if ( dpos < w ) {
		a = coefs[ dpos - 1 ];
	}
	else if ( ( dpos % w ) == 0 ) {
		b = coefs[ dpos - w ];
	}
	else {
		a = coefs[ dpos - 1 ];
		b = coefs[ dpos - w ];
		c = coefs[ dpos - 1 - w ];
	}

	return plocoi( a, b, c );
}
#else
int dc_1ddct_predictor( CodecContext& ctx, int cmp, int dpos )
{
	const int w  = ctx.cmpnfo[ cmp ].bch;
	const int px = dpos % w;
	const int py = dpos / w;

	const short swap = ctx.colldata[ cmp ][ 0 ][ dpos ];
	ctx.colldata[ cmp ][ 0 ][ dpos ] = 0;

	int pred = 0;
	if ( ( px > 0 ) && ( py > 0 ) ) {
		const int pa = idct_2d_fst_8x1( ctx, cmp, dpos - 1, 7 );
		const int pb = idct_2d_fst_1x8( ctx, cmp, dpos - w, 7 );
		const int xa = idct_2d_fst_8x1( ctx, cmp, dpos, 0 );
		const int xb = idct_2d_fst_1x8( ctx, cmp, dpos, 0 );
		pred = ( ( pa - xa ) + ( pb - xb ) ) * ( 8 / 2 );
	}
	else if ( px > 0 ) {
		const int pa = idct_2d_fst_8x1( ctx, cmp, dpos - 1, 7 );
		const int xa = idct_2d_fst_8x1( ctx, cmp, dpos, 0 );
		pred = ( pa - xa ) * 8;
	}
	else if ( py > 0 ) {
		const int pb = idct_2d_fst_1x8( ctx, cmp, dpos - w, 7 );
		const int xb = idct_2d_fst_1x8( ctx, cmp, dpos, 0 );
		pred = ( pb - xb ) * 8;
	}

	ctx.colldata[ cmp ][ 0 ][ dpos ] = swap;

	pred = clamped( -( 1024 * DCT_RSC_FACTOR ), ( 1016 * DCT_RSC_FACTOR ), pred );
	pred = pred / ctx.quant( cmp, 0 );
	pred = dct_rescale( pred );
	return pred;
}
#endif

} // namespace


/* -----------------------------------------------
	adapt ICOS tables for quantizer tables
	----------------------------------------------- */
bool adapt_icos( CodecContext& ctx )
{
	unsigned short quant[ 64 ];
	if ( ( ctx.action == A_PGM_DUMP ) && !ctx.adpt_idct_8x8 ) {
		ctx.adpt_idct_8x8 = std::make_unique<std::array<std::array<int, 8 * 8 * 8 * 8>, 4>>();
	}

	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		for ( int ipos = 0; ipos < 64; ipos++ ) {
			quant[ ipos ] = ctx.quant( cmp, zigzag[ ipos ] );
			if ( quant[ ipos ] >= 2048 )
				quant[ ipos ] = 0;
		}
		if ( ctx.action == A_PGM_DUMP ) {
			for ( int ipos = 0; ipos < 64 * 64; ipos++ )
				( *ctx.adpt_idct_8x8 )[ cmp ][ ipos ] = icos_idct_8x8[ ipos ] * quant[ ipos % 64 ];
		}
		for ( int ipos = 0; ipos < 8 * 8; ipos++ )
			ctx.adpt_idct_1x8[ cmp ][ ipos ] = icos_idct_1x8[ ipos ] * quant[ ( ipos % 8 ) * 8 ];
		for ( int ipos = 0; ipos < 8 * 8; ipos++ )
			ctx.adpt_idct_8x1[ cmp ][ ipos ] = icos_idct_1x8[ ipos ] * quant[ ipos % 8 ];
	}

	return true;
}


/* -----------------------------------------------
	filter DC coefficients
	----------------------------------------------- */
bool predict_dc( CodecContext& ctx )
{
	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		const int absmaxp = ctx.max_v( cmp, 0 );
		const int absmaxn = -absmaxp;
		const int corr_f  = ( 2 * absmaxp ) + 1;

		for ( int dpos = ctx.cmpnfo[ cmp ].bc - 1; dpos > 0; dpos-- ) {
			short* coef = &ctx.colldata[ cmp ][ 0 ][ dpos ];
#if defined(USE_PLOCOI)
			int val = static_cast<int>( *coef ) - dc_coll_predictor( ctx, cmp, dpos );
#else
			int val = static_cast<int>( *coef ) - dc_1ddct_predictor( ctx, cmp, dpos );
#endif
			if ( val > absmaxp ) val -= corr_f;
			else if ( val < absmaxn ) val += corr_f;
			*coef = static_cast<short>( val );
		}
	}

	return true;
}


/* -----------------------------------------------
	unpredict DC coefficients
	----------------------------------------------- */
bool unpredict_dc( CodecContext& ctx )
{
	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		const int absmaxp = ctx.max_v( cmp, 0 );
		const int absmaxn = -absmaxp;
		const int corr_f  = ( 2 * absmaxp ) + 1;

		for ( int dpos = 1; dpos < ctx.cmpnfo[ cmp ].bc; dpos++ ) {
			short* coef = &ctx.colldata[ cmp ][ 0 ][ dpos ];
#if defined(USE_PLOCOI)
			int val = static_cast<int>( *coef ) + dc_coll_predictor( ctx, cmp, dpos );
#else
			int val = static_cast<int>( *coef ) + dc_1ddct_predictor( ctx, cmp, dpos );
#endif
			if ( val > absmaxp ) val -= corr_f;
			else if ( val < absmaxn ) val += corr_f;
			*coef = static_cast<short>( val );
		}
	}

	return true;
}


/* -----------------------------------------------
	checks range of values, error if out of bounds
	----------------------------------------------- */
bool check_value_range( CodecContext& ctx )
{
	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		for ( int bpos = 0; bpos < 64; bpos++ ) {
			const int absmax = ctx.max_v( cmp, bpos );
			const short* band = ctx.colldata[ cmp ][ bpos ].data();
			for ( int dpos = 0; dpos < ctx.cmpnfo[ cmp ].bc; dpos++ ) {
				if ( ( band[ dpos ] > absmax ) || ( band[ dpos ] < -absmax ) ) {
					ctx.set_errormessage( "value out of range error: cmp%i, frq%i, val %i, max %i",
						cmp, bpos, static_cast<int>( band[ dpos ] ), absmax );
					ctx.errorlevel = 2;
					return false;
				}
			}
		}
	}

	return true;
}


/* -----------------------------------------------
	calculate zero distribution lists
	----------------------------------------------- */
bool calc_zdst_lists( CodecContext& ctx )
{
	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		const int bc = ctx.cmpnfo[ cmp ].bc;
		std::fill( ctx.zdstdata[ cmp ].begin(), ctx.zdstdata[ cmp ].end(), 0 );
		std::fill( ctx.zdstxlow[ cmp ].begin(), ctx.zdstxlow[ cmp ].end(), 0 );
		std::fill( ctx.zdstylow[ cmp ].begin(), ctx.zdstylow[ cmp ].end(), 0 );

		for ( int bpos = 1; bpos < 64; bpos++ ) {
			const int b_x = unzigzag[ bpos ] % 8;
			const int b_y = unzigzag[ bpos ] / 8;
			const short* band = ctx.colldata[ cmp ][ bpos ].data();
			if ( b_x == 0 ) {
				for ( int dpos = 0; dpos < bc; dpos++ )
					if ( band[ dpos ] != 0 ) ctx.zdstylow[ cmp ][ dpos ]++;
			}
			else if ( b_y == 0 ) {
				for ( int dpos = 0; dpos < bc; dpos++ )
					if ( band[ dpos ] != 0 ) ctx.zdstxlow[ cmp ][ dpos ]++;
			}
			else {
				for ( int dpos = 0; dpos < bc; dpos++ )
					if ( band[ dpos ] != 0 ) ctx.zdstdata[ cmp ][ dpos ]++;
			}
		}
	}

	return true;
}


/* -----------------------------------------------
	packs all parts to compressed pjg
	----------------------------------------------- */
bool pack_pjg( CodecContext& ctx )
{
	ctx.str_out->write( kPjgMagic, 2 );

	if ( !ctx.auto_set ) {
		ctx.str_out->write_byte( 0x00 );
		ctx.str_out->write( ctx.nois_trs.data(), 4 );
		ctx.str_out->write( ctx.segm_cnt.data(), 4 );
	}

	ctx.str_out->write_byte( kAppVersion );

	ArithmeticEncoder encoder_obj( *ctx.str_out );
	ArithmeticEncoder* encoder = &encoder_obj;

	if ( ctx.disc_meta ) {
		if ( !jpg_rebuild_header( ctx ) ) return false;
	}
	if ( !pjg_optimize_header( ctx ) ) return false;
	if ( ctx.padbit == -1 ) ctx.padbit = 1;

	if ( !pjg_encode_generic( encoder, ctx.hdrdata.data(), ctx.hdrdata.size() ) ) return false;
	if ( !pjg_encode_bit( encoder, static_cast<unsigned char>( ctx.padbit ) ) ) return false;
	if ( !pjg_encode_bit( encoder, ctx.has_rst_err ? 1 : 0 ) ) return false;
	if ( ctx.has_rst_err ) {
		if ( ctx.rst_err.size() < static_cast<std::size_t>( ctx.scnc ) )
			ctx.rst_err.resize( static_cast<std::size_t>( ctx.scnc ), 0 );
		if ( !pjg_encode_generic( encoder, ctx.rst_err.data(), static_cast<std::size_t>( ctx.scnc ) ) )
			return false;
	}

	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		if ( !pjg_encode_zstscan( ctx, encoder, cmp ) ) return false;
		if ( !pjg_encode_zdst_high( ctx, encoder, cmp ) ) return false;
		if ( !pjg_encode_ac_high( ctx, encoder, cmp ) ) return false;
		if ( !pjg_encode_zdst_low( ctx, encoder, cmp ) ) return false;
		if ( !pjg_encode_ac_low( ctx, encoder, cmp ) ) return false;
		if ( !pjg_encode_dc( ctx, encoder, cmp ) ) return false;
	}

	if ( !pjg_encode_bit( encoder, ctx.grbgdata.empty() ? 0 : 1 ) ) return false;
	if ( !ctx.grbgdata.empty() ) {
		if ( !pjg_encode_generic( encoder, ctx.grbgdata.data(), ctx.grbgdata.size() ) )
			return false;
	}

	encoder_obj.finalize();

	if ( ctx.str_out->error() ) {
		ctx.errormessage = "write error, possibly drive is full";
		ctx.errorlevel = 2;
		return false;
	}

	ctx.pjgfilesize = static_cast<int>( ctx.str_out->num_bytes_written() );
	return true;
}


/* -----------------------------------------------
	unpacks compressed pjg to colldata
	----------------------------------------------- */
bool unpack_pjg( CodecContext& ctx )
{
	unsigned char hcode = 0;
	unsigned char cb    = 0;

	while ( true ) {
		if ( !ctx.str_in->read_byte( &hcode ) ) {
			ctx.errormessage = "unexpected end of data encountered";
			ctx.errorlevel = 2;
			return false;
		}
		if ( hcode == 0x00 ) {
			if ( ( ctx.str_in->read( ctx.nois_trs.data(), 4 ) != 4 ) ||
				 ( ctx.str_in->read( ctx.segm_cnt.data(), 4 ) != 4 ) ) {
				ctx.errormessage = "unexpected end of data encountered";
				ctx.errorlevel = 2;
				return false;
			}
			for ( int cmp = 0; cmp < 4; cmp++ ) {
				if ( ( ctx.nois_trs[ cmp ] > 10 ) || ( ctx.segm_cnt[ cmp ] < 1 ) || ( ctx.segm_cnt[ cmp ] > 49 ) ) {
					ctx.errormessage = "invalid compression settings in header";
					ctx.errorlevel = 2;
					return false;
				}
			}
			ctx.auto_set = false;
		}
		else if ( hcode >= 0x14 ) {
			if ( hcode != kAppVersion ) {
				ctx.set_errormessage( "incompatible file, use %s v%i.%i",
					kAppName, hcode / 10, hcode % 10 );
				ctx.errorlevel = 2;
				return false;
			}
			break;
		}
		else {
			ctx.set_errormessage( "unknown header code, use newer version of %s", kAppName );
			ctx.errorlevel = 2;
			return false;
		}
	}

	const std::size_t in_bytes = ( ctx.str_in != nullptr ) ? ctx.str_in->get_size() : 0;
	const std::size_t max_generic = std::min<std::size_t>(
		32u << 20,
		std::max<std::size_t>( 262144u, ( in_bytes <= ( ( 32u << 20 ) / 32u ) ) ? ( in_bytes * 32u ) : ( 32u << 20 ) ) );

	ArithmeticDecoder decoder_obj( *ctx.str_in );
	ArithmeticDecoder* decoder = &decoder_obj;

	if ( !pjg_decode_generic( decoder, ctx.hdrdata, max_generic ) ) {
		ctx.errormessage = "corrupted or oversized header data";
		ctx.errorlevel = 2;
		return false;
	}
	if ( !pjg_decode_bit( decoder, &cb ) ) return false;
	ctx.padbit = cb;

	if ( !pjg_decode_bit( decoder, &cb ) ) return false;
	ctx.has_rst_err = ( cb == 1 );
	if ( ctx.has_rst_err ) {
		if ( !pjg_decode_generic( decoder, ctx.rst_err, 65536u ) ) {
			ctx.errormessage = "corrupted restart error data";
			ctx.errorlevel = 2;
			return false;
		}
	}
	else {
		ctx.rst_err.clear();
	}

	if ( !pjg_unoptimize_header( ctx ) ) return false;
	if ( ctx.disc_meta ) {
		if ( !jpg_rebuild_header( ctx ) ) return false;
	}
	if ( !jpg_setup_imginfo( ctx ) ) return false;

	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		if ( !pjg_decode_zstscan( ctx, decoder, cmp ) ) return false;
		if ( !pjg_decode_zdst_high( ctx, decoder, cmp ) ) return false;
		if ( !pjg_decode_ac_high( ctx, decoder, cmp ) ) return false;
		if ( !pjg_decode_zdst_low( ctx, decoder, cmp ) ) return false;
		if ( !pjg_decode_ac_low( ctx, decoder, cmp ) ) return false;
		if ( !pjg_decode_dc( ctx, decoder, cmp ) ) return false;
	}

	if ( !pjg_decode_bit( decoder, &cb ) ) return false;
	if ( cb == 0 ) {
		ctx.grbgdata.clear();
	}
	else if ( !pjg_decode_generic( decoder, ctx.grbgdata, max_generic ) ) {
		ctx.errormessage = "corrupted or oversized trailing data";
		ctx.errorlevel = 2;
		return false;
	}

	ctx.pjgfilesize = static_cast<int>( ctx.str_in->get_size() );
	return true;
}


/* -----------------------------------------------
	inverse DCT transform using precalc tables (fast)
	----------------------------------------------- */
int idct_2d_fst_8x8( const CodecContext& ctx, int cmp, int dpos, int ix, int iy )
{
	if ( !ctx.adpt_idct_8x8 ) {
		return 0;
	}
	int idct = 0;
	const int ixy = ( ( iy << 3 ) + ix ) << 6;
	const auto& tbl = ( *ctx.adpt_idct_8x8 )[ cmp ];

	for ( int i = 0; i < 64; i++ ) {
		idct += ctx.colldata[ cmp ][ zigzag[ i ] ][ dpos ] * tbl[ ixy + i ];
	}

	return idct;
}
