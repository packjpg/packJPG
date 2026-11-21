#ifndef JPEG_CODEC_H
#define JPEG_CODEC_H

#include "codec_context.h"

bool read_jpeg( CodecContext& ctx );
bool merge_jpeg( CodecContext& ctx );
bool decode_jpeg( CodecContext& ctx );
bool recode_jpeg( CodecContext& ctx );

bool jpg_setup_imginfo( CodecContext& ctx );
bool jpg_parse_jfif( CodecContext& ctx, unsigned char type, unsigned int len, const unsigned char* segment );
bool jpg_rebuild_header( CodecContext& ctx );

int  jpg_next_huffcode( BitReader* huffr, const HuffTree* ctree );
void jpg_build_huffcodes( const unsigned char* clen, const unsigned char* cval, HuffCodes* hc, HuffTree* ht );

#endif
