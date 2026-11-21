#ifndef PJG_CODEC_H
#define PJG_CODEC_H

#include "codec_context.h"

bool adapt_icos( CodecContext& ctx );
bool predict_dc( CodecContext& ctx );
bool unpredict_dc( CodecContext& ctx );
bool check_value_range( CodecContext& ctx );
bool calc_zdst_lists( CodecContext& ctx );
bool pack_pjg( CodecContext& ctx );
bool unpack_pjg( CodecContext& ctx );

int idct_2d_fst_8x8( const CodecContext& ctx, int cmp, int dpos, int ix, int iy );

#endif
