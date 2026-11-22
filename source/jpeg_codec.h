#ifndef JPEG_CODEC_H
#define JPEG_CODEC_H

#include "codec_context.h"

bool read_jpeg( CodecContext& ctx );
bool merge_jpeg( CodecContext& ctx );
bool decode_jpeg( CodecContext& ctx );
bool recode_jpeg( CodecContext& ctx );

bool jpg_setup_imginfo( CodecContext& ctx, bool from_pjg );
inline bool jpg_setup_imginfo( CodecContext& ctx ) { return jpg_setup_imginfo( ctx, ctx.filetype == F_PJG ); }
bool jpg_parse_jfif( CodecContext& ctx, unsigned char type, unsigned int len, const unsigned char* segment, bool from_pjg );
inline bool jpg_parse_jfif( CodecContext& ctx, unsigned char type, unsigned int len, const unsigned char* segment )
{
	return jpg_parse_jfif( ctx, type, len, segment, ctx.filetype == F_PJG );
}
bool jpg_rebuild_header( CodecContext& ctx );

inline bool jpg_valid_sos_length( unsigned int len, unsigned int ns ) noexcept
{
	return ( ns >= 1u ) && ( ns <= 4u ) && ( len == 8u + 2u * ns );
}

inline bool jpg_parse_dht_counts( const unsigned char* clen, int& sym_count ) noexcept
{
	if ( clen[ 0 ] > 2u ) {
		return false;
	}
	int total = 0;
	int kraft = 0;
	for ( int i = 0; i < 16; ++i ) {
		const int cnt = static_cast<int>( clen[ i ] );
		total += cnt;
		kraft = ( kraft << 1 ) + cnt;
		if ( kraft > ( 1 << ( i + 1 ) ) ) {
			return false;
		}
	}
	if ( total > 256 ) {
		return false;
	}
	sym_count = total;
	return true;
}

int  jpg_next_huffcode( BitReader* huffr, const HuffTree* ctree );
void jpg_build_huffcodes( const unsigned char* clen, const unsigned char* cval, HuffCodes* hc, HuffTree* ht );

#endif
