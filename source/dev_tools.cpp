#include "dev_tools.h"

#include "jpeg_codec.h"
#include "path_utils.h"
#include "pjg_codec.h"

#include <array>
#include <cstdio>
#include <vector>

namespace {

std::string resolve_dump_filename( const std::string& base, const char* ext, bool overwrite )
{
	return overwrite ? make_filename( base, ext ) : make_unique_filename( base, ext );
}

FILE* open_dump_file( const std::string& fn, const char* mode )
{
	if ( !ensure_parent_directory( fn ) ) {
		return nullptr;
	}
	return std::fopen( fn.c_str(), mode );
}

bool close_dump_file( CodecContext& ctx, FILE* fp, const std::string& fn )
{
	const bool had_err = ( std::ferror( fp ) != 0 );
	const int rc = std::fclose( fp );
	if ( had_err || ( rc != 0 ) ) {
		ctx.set_errormessage( kFwrErrMsg, fn.c_str() );
		ctx.errorlevel = 2;
		return false;
	}
	return true;
}

bool dump_file( CodecContext& ctx, const std::string& base, const char* ext, const void* data, std::size_t bpv, std::size_t size, bool overwrite )
{
	const std::string fn = resolve_dump_filename( base, ext, overwrite );
	FILE* fp = open_dump_file( fn, "wb" );
	if ( fp == nullptr ) {
		ctx.set_errormessage( kFwrErrMsg, fn.c_str() );
		ctx.errorlevel = 2;
		return false;
	}
	if ( ( size > 0 ) && ( std::fwrite( data, bpv, size, fp ) != size ) ) {
		std::fclose( fp );
		ctx.set_errormessage( kFwrErrMsg, fn.c_str() );
		ctx.errorlevel = 2;
		return false;
	}
	return close_dump_file( ctx, fp, fn );
}

bool dump_hdr( CodecContext& ctx, const std::string& base, bool overwrite )
{
	return dump_file( ctx, base, "hdr", ctx.hdrdata.data(), 1, ctx.hdrdata.size(), overwrite );
}

bool dump_huf( CodecContext& ctx, const std::string& base, bool overwrite )
{
	return dump_file( ctx, base, "huf", ctx.huffdata.data(), 1, ctx.huffdata.size(), overwrite );
}

bool dump_coll( CodecContext& ctx, const std::string& base, int collmode, bool overwrite )
{
	constexpr std::array<const char*, 4> ext{ "coll0", "coll1", "coll2", "coll3" };

	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		const std::string fn = resolve_dump_filename( base, ext[ cmp ], overwrite );
		FILE* fp = open_dump_file( fn, "wb" );
		if ( fp == nullptr ) {
			ctx.set_errormessage( kFwrErrMsg, fn.c_str() );
			ctx.errorlevel = 2;
			return false;
		}

		const auto& ci = ctx.cmpnfo[ cmp ];
		switch ( collmode ) {
			case 0:
				for ( int bpos = 0; bpos < 64; bpos++ )
					std::fwrite( ctx.colldata[ cmp ][ bpos ].data(), sizeof( short ), static_cast<std::size_t>( ci.bc ), fp );
				break;

			case 1:
				for ( int dpos = 0; dpos < ci.bc; dpos++ )
					for ( int bpos = 0; bpos < 64; bpos++ )
						std::fwrite( &ctx.colldata[ cmp ][ bpos ][ dpos ], sizeof( short ), 1, fp );
				break;

			case 2: {
				int dpos = 0;
				for ( int i = 0; i < 64; ) {
					const int bpos = zigzag[ i++ ];
					std::fwrite( &ctx.colldata[ cmp ][ bpos ][ dpos ], sizeof( short ), static_cast<std::size_t>( ci.bch ), fp );
					if ( ( i % 8 ) == 0 ) {
						dpos += ci.bch;
						if ( dpos >= ci.bc ) dpos = 0;
						else i -= 8;
					}
				}
				break;
			}

			case 3:
				for ( int i = 0; i < ( ci.bcv * 8 ); i++ ) {
					for ( int j = 0; j < ( ci.bch * 8 ); j++ ) {
						const int bpos = zigzag[ ( ( i % 8 ) * 8 ) + ( j % 8 ) ];
						const int dpos = ( ( i / 8 ) * ci.bch ) + ( j / 8 );
						std::fwrite( &ctx.colldata[ cmp ][ bpos ][ dpos ], sizeof( short ), 1, fp );
					}
				}
				break;

			case 4: {
				int dpos = 0;
				for ( int i = 0; i < 64; ) {
					const int bpos = even_zigzag[ i++ ];
					std::fwrite( &ctx.colldata[ cmp ][ bpos ][ dpos ], sizeof( short ), static_cast<std::size_t>( ci.bch ), fp );
					if ( ( i % 8 ) == 0 ) {
						dpos += ci.bch;
						if ( dpos >= ci.bc ) dpos = 0;
						else i -= 8;
					}
				}
				break;
			}

			case 5:
				for ( int i = 0; i < ( ci.bcv * 8 ); i++ ) {
					for ( int j = 0; j < ( ci.bch * 8 ); j++ ) {
						const int bpos = even_zigzag[ ( ( i % 8 ) * 8 ) + ( j % 8 ) ];
						const int dpos = ( ( i / 8 ) * ci.bch ) + ( j / 8 );
						std::fwrite( &ctx.colldata[ cmp ][ bpos ][ dpos ], sizeof( short ), 1, fp );
					}
				}
				break;
		}

		if ( !close_dump_file( ctx, fp, fn ) ) {
			return false;
		}
	}

	return true;
}

bool dump_zdst( CodecContext& ctx, const std::string& base, bool overwrite )
{
	constexpr std::array<const char*, 4> ext{ "zdst0", "zdst1", "zdst2", "zdst3" };

	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		if ( !dump_file( ctx, base, ext[ cmp ], ctx.zdstdata[ cmp ].data(), 1, static_cast<std::size_t>( ctx.cmpnfo[ cmp ].bc ), overwrite ) )
			return false;
	}
	return true;
}

bool dump_info( CodecContext& ctx, const std::string& base, bool overwrite )
{
	const std::string fn = resolve_dump_filename( base, "nfo", overwrite );
	FILE* fp = open_dump_file( fn, "w" );
	if ( fp == nullptr ) {
		ctx.set_errormessage( kFwrErrMsg, fn.c_str() );
		ctx.errorlevel = 2;
		return false;
	}

	std::fprintf( fp, "<Infofile for JPEG image %s>\n\n\n", ctx.jpgfilename.c_str() );
	std::fprintf( fp, "coding process: %s\n", ( ctx.jpegtype == 1 ) ? "sequential" : "progressive" );
	std::fprintf( fp, "imageheight: %i / imagewidth: %i\n", ctx.imgheight, ctx.imgwidth );
	std::fprintf( fp, "component count: %i\n", ctx.cmpc );
	std::fprintf( fp, "mcu count: %i/%i/%i (all/v/h)\n\n", ctx.mcuc, ctx.mcuv, ctx.mcuh );

	std::fprintf( fp, "\nfile header structure:\n" );
	std::fprintf( fp, " type  length   hpos\n" );
	unsigned int hpos = 0;
	unsigned int len  = 0;
	for ( hpos = 0; hpos + 4 <= ctx.hdrdata.size(); hpos += len ) {
		const unsigned char type = ctx.hdrdata[ hpos + 1 ];
		len = static_cast<unsigned int>( 2 + b_short( ctx.hdrdata[ hpos + 2 ], ctx.hdrdata[ hpos + 3 ] ) );
		if ( ( len < 4 ) || ( hpos + len > ctx.hdrdata.size() ) ) break;
		std::fprintf( fp, " FF%2X  %6i %6i\n", static_cast<int>( type ), static_cast<int>( len ), static_cast<int>( hpos ) );
	}
	std::fprintf( fp, " _END       0 %6i\n\n", static_cast<int>( hpos ) );

	std::fprintf( fp, "\ncompression settings:\n" );
	std::fprintf( fp, " no of segments    ->  %3i[0] %3i[1] %3i[2] %3i[3]\n",
		ctx.segm_cnt[ 0 ], ctx.segm_cnt[ 1 ], ctx.segm_cnt[ 2 ], ctx.segm_cnt[ 3 ] );
	std::fprintf( fp, " noise threshold   ->  %3i[0] %3i[1] %3i[2] %3i[3]\n\n",
		ctx.nois_trs[ 0 ], ctx.nois_trs[ 1 ], ctx.nois_trs[ 2 ], ctx.nois_trs[ 3 ] );

	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		const auto& ci = ctx.cmpnfo[ cmp ];
		std::fprintf( fp, "\ncomponent number %i ->\n", cmp );
		std::fprintf( fp, "sample factors: %i/%i (v/h)\n", ci.sfv, ci.sfh );
		std::fprintf( fp, "blocks per mcu: %i\n", ci.mbs );
		std::fprintf( fp, "block count (mcu): %i/%i/%i (all/v/h)\n", ci.bc, ci.bcv, ci.bch );
		std::fprintf( fp, "block count (sng): %i/%i/%i (all/v/h)\n", ci.nc, ci.ncv, ci.nch );
		std::fprintf( fp, "quantiser table ->" );
		for ( int i = 0; i < 64; i++ ) {
			const int bpos = zigzag[ i ];
			if ( ( i % 8 ) == 0 ) std::fprintf( fp, "\n" );
			std::fprintf( fp, "%4i, ", ctx.quant( cmp, bpos ) );
		}
		std::fprintf( fp, "\nmaximum values ->" );
		for ( int i = 0; i < 64; i++ ) {
			const int bpos = zigzag[ i ];
			if ( ( i % 8 ) == 0 ) std::fprintf( fp, "\n" );
			std::fprintf( fp, "%4i, ", ctx.max_v( cmp, bpos ) );
		}
		std::fprintf( fp, "\n\n" );
	}

	return close_dump_file( ctx, fp, fn );
}

bool dump_dist( CodecContext& ctx, const std::string& base, bool overwrite )
{
	const std::string fn = resolve_dump_filename( base, "dist", overwrite );
	FILE* fp = open_dump_file( fn, "wb" );
	if ( fp == nullptr ) {
		ctx.set_errormessage( kFwrErrMsg, fn.c_str() );
		ctx.errorlevel = 2;
		return false;
	}

	std::array<unsigned int, 1025> dist{};
	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		for ( int bpos = 0; bpos < 64; bpos++ ) {
			dist.fill( 0 );
			for ( int dpos = 0; dpos < ctx.cmpnfo[ cmp ].bc; dpos++ ) {
				const int idx = clamped( 0, 1024, abs_val( ctx.colldata[ cmp ][ bpos ][ dpos ] ) );
				dist[ static_cast<std::size_t>( idx ) ]++;
			}
			if ( std::fwrite( dist.data(), sizeof( unsigned int ), dist.size(), fp ) != dist.size() ) {
				std::fclose( fp );
				ctx.set_errormessage( kFwrErrMsg, fn.c_str() );
				ctx.errorlevel = 2;
				return false;
			}
		}
	}

	return close_dump_file( ctx, fp, fn );
}

bool dump_pgm( CodecContext& ctx, const std::string& base, bool overwrite )
{
	constexpr std::array<const char*, 4> ext{ "cmp0.pgm", "cmp1.pgm", "cmp2.pgm", "cmp3.pgm" };

	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		const std::string fn = resolve_dump_filename( base, ext[ cmp ], overwrite );
		FILE* fp = open_dump_file( fn, "wb" );
		if ( fp == nullptr ) {
			ctx.set_errormessage( kFwrErrMsg, fn.c_str() );
			ctx.errorlevel = 2;
			return false;
		}

		const auto& ci = ctx.cmpnfo[ cmp ];
		std::vector<unsigned char> imgdata( static_cast<std::size_t>( ci.bc ) * 64, 0 );

		for ( int dpos = 0; dpos < ci.bc; dpos++ ) {
			const int dcpos = ( ( ( dpos / ci.bch ) * ci.bch ) << 6 ) + ( ( dpos % ci.bch ) << 3 );
			for ( int y = 0; y < 8; y++ ) {
				const int ypos = dcpos + ( y * ( ci.bch << 3 ) );
				for ( int x = 0; x < 8; x++ ) {
					const int xpos = ypos + x;
					int pix_v = idct_2d_fst_8x8( ctx, cmp, dpos, x, y );
					pix_v = dct_rescale( pix_v ) + 128;
					imgdata[ static_cast<std::size_t>( xpos ) ] = static_cast<unsigned char>( clamped( 0, 255, pix_v ) );
				}
			}
		}

		std::fprintf( fp, "P5\n" );
		std::fprintf( fp, "# created by %s v%i.%i%s (%s) by %s\n",
			kAppTitle, kAppVersion / 10, kAppVersion % 10, kSubVersion, kVersionDate, kAuthor );
		std::fprintf( fp, "%i %i\n", ci.bch * 8, ci.bcv * 8 );
		std::fprintf( fp, "255\n" );
		if ( !imgdata.empty() &&
		     ( std::fwrite( imgdata.data(), sizeof( unsigned char ), imgdata.size(), fp ) != imgdata.size() ) ) {
			std::fclose( fp );
			ctx.set_errormessage( kFwrErrMsg, fn.c_str() );
			ctx.errorlevel = 2;
			return false;
		}
		if ( !close_dump_file( ctx, fp, fn ) ) {
			return false;
		}
	}

	return true;
}

} // namespace


bool dump_errfile( CodecContext& ctx, const std::string& input_name, bool overwrite )
{
	if ( ctx.errorlevel == 0 ) return true;

	const std::string fn = resolve_dump_filename( input_name, ( ctx.errorlevel == 1 ) ? "wrn.nfo" : "err.nfo", overwrite );
	FILE* fp = open_dump_file( fn, "w" );
	if ( fp == nullptr ) {
		ctx.set_errormessage( kFwrErrMsg, fn.c_str() );
		ctx.errorlevel = 2;
		return false;
	}

	std::fprintf( fp, "--> error (level %i) in file \"%s\" <--\n\n", ctx.errorlevel, input_name.c_str() );
	std::fprintf( fp, " %s -> %s:\n", stage_status_text( ctx.error_stage ),
		( ctx.errorlevel == 1 ) ? "warning" : "error" );
	std::fprintf( fp, " %s\n", ctx.errormessage.c_str() );
	return close_dump_file( ctx, fp, fn );
}

void run_dev_action( CodecContext& ctx, const std::string& input_name, int collmode, bool overwrite, const StageReporter* reporter )
{
	if ( ctx.filetype == F_JPG ) {
		switch ( ctx.action ) {
			case A_SPLIT_DUMP:
				execute_stage( ctx, StageId::ReadJpeg, [&]() { return read_jpeg( ctx ); },                       reporter );
				execute_stage( ctx, StageId::DumpHdr,  [&]() { return dump_hdr( ctx, input_name, overwrite ); }, reporter );
				execute_stage( ctx, StageId::DumpHuf,  [&]() { return dump_huf( ctx, input_name, overwrite ); }, reporter );
				break;
			case A_COLL_DUMP:
				execute_stage( ctx, StageId::ReadJpeg,   [&]() { return read_jpeg( ctx ); },                                  reporter );
				execute_stage( ctx, StageId::DecodeJpeg, [&]() { return decode_jpeg( ctx ); },                                reporter );
				execute_stage( ctx, StageId::DumpColl,   [&]() { return dump_coll( ctx, input_name, collmode, overwrite ); }, reporter );
				break;
			case A_FCOLL_DUMP:
				execute_stage( ctx, StageId::ReadJpeg,        [&]() { return read_jpeg( ctx ); },                                  reporter );
				execute_stage( ctx, StageId::DecodeJpeg,      [&]() { return decode_jpeg( ctx ); },                                reporter );
				execute_stage( ctx, StageId::CheckValueRange, [&]() { return check_value_range( ctx ); },                          reporter );
				execute_stage( ctx, StageId::AdaptIcos,       [&]() { return adapt_icos( ctx ); },                                 reporter );
				execute_stage( ctx, StageId::PredictDc,       [&]() { return predict_dc( ctx ); },                                 reporter );
				execute_stage( ctx, StageId::DumpColl,        [&]() { return dump_coll( ctx, input_name, collmode, overwrite ); }, reporter );
				break;
			case A_ZDST_DUMP:
				execute_stage( ctx, StageId::ReadJpeg,        [&]() { return read_jpeg( ctx ); },                        reporter );
				execute_stage( ctx, StageId::DecodeJpeg,      [&]() { return decode_jpeg( ctx ); },                      reporter );
				execute_stage( ctx, StageId::CheckValueRange, [&]() { return check_value_range( ctx ); },                reporter );
				execute_stage( ctx, StageId::AdaptIcos,       [&]() { return adapt_icos( ctx ); },                       reporter );
				execute_stage( ctx, StageId::PredictDc,       [&]() { return predict_dc( ctx ); },                       reporter );
				execute_stage( ctx, StageId::CalcZdstLists,   [&]() { return calc_zdst_lists( ctx ); },                  reporter );
				execute_stage( ctx, StageId::DumpZdst,        [&]() { return dump_zdst( ctx, input_name, overwrite ); }, reporter );
				break;
			case A_TXT_INFO:
				execute_stage( ctx, StageId::ReadJpeg, [&]() { return read_jpeg( ctx ); },                        reporter );
				execute_stage( ctx, StageId::DumpInfo, [&]() { return dump_info( ctx, input_name, overwrite ); }, reporter );
				break;
			case A_DIST_INFO:
				execute_stage( ctx, StageId::ReadJpeg,        [&]() { return read_jpeg( ctx ); },                        reporter );
				execute_stage( ctx, StageId::DecodeJpeg,      [&]() { return decode_jpeg( ctx ); },                      reporter );
				execute_stage( ctx, StageId::CheckValueRange, [&]() { return check_value_range( ctx ); },                reporter );
				execute_stage( ctx, StageId::AdaptIcos,       [&]() { return adapt_icos( ctx ); },                       reporter );
				execute_stage( ctx, StageId::PredictDc,       [&]() { return predict_dc( ctx ); },                       reporter );
				execute_stage( ctx, StageId::DumpDist,        [&]() { return dump_dist( ctx, input_name, overwrite ); }, reporter );
				break;
			case A_PGM_DUMP:
				execute_stage( ctx, StageId::ReadJpeg,   [&]() { return read_jpeg( ctx ); },                       reporter );
				execute_stage( ctx, StageId::DecodeJpeg, [&]() { return decode_jpeg( ctx ); },                     reporter );
				execute_stage( ctx, StageId::AdaptIcos,  [&]() { return adapt_icos( ctx ); },                      reporter );
				execute_stage( ctx, StageId::DumpPgm,    [&]() { return dump_pgm( ctx, input_name, overwrite ); }, reporter );
				break;
			default:
				break;
		}
	}
	else if ( ctx.filetype == F_PJG ) {
		switch ( ctx.action ) {
			case A_SPLIT_DUMP:
				execute_stage( ctx, StageId::UnpackPjg,   [&]() { return unpack_pjg( ctx ); },                      reporter );
				execute_stage( ctx, StageId::AdaptIcos,   [&]() { return adapt_icos( ctx ); },                      reporter );
				execute_stage( ctx, StageId::UnpredictDc, [&]() { return unpredict_dc( ctx ); },                    reporter );
				execute_stage( ctx, StageId::RecodeJpeg,  [&]() { return recode_jpeg( ctx ); },                     reporter );
				execute_stage( ctx, StageId::DumpHdr,     [&]() { return dump_hdr( ctx, input_name, overwrite ); }, reporter );
				execute_stage( ctx, StageId::DumpHuf,     [&]() { return dump_huf( ctx, input_name, overwrite ); }, reporter );
				break;
			case A_COLL_DUMP:
				execute_stage( ctx, StageId::UnpackPjg,   [&]() { return unpack_pjg( ctx ); },                                reporter );
				execute_stage( ctx, StageId::AdaptIcos,   [&]() { return adapt_icos( ctx ); },                                reporter );
				execute_stage( ctx, StageId::UnpredictDc, [&]() { return unpredict_dc( ctx ); },                              reporter );
				execute_stage( ctx, StageId::DumpColl,    [&]() { return dump_coll( ctx, input_name, collmode, overwrite ); }, reporter );
				break;
			case A_FCOLL_DUMP:
				execute_stage( ctx, StageId::UnpackPjg, [&]() { return unpack_pjg( ctx ); },                                reporter );
				execute_stage( ctx, StageId::DumpColl,  [&]() { return dump_coll( ctx, input_name, collmode, overwrite ); }, reporter );
				break;
			case A_ZDST_DUMP:
				execute_stage( ctx, StageId::UnpackPjg, [&]() { return unpack_pjg( ctx ); },                       reporter );
				execute_stage( ctx, StageId::DumpZdst,  [&]() { return dump_zdst( ctx, input_name, overwrite ); }, reporter );
				break;
			case A_TXT_INFO:
				execute_stage( ctx, StageId::UnpackPjg, [&]() { return unpack_pjg( ctx ); },                       reporter );
				execute_stage( ctx, StageId::DumpInfo,  [&]() { return dump_info( ctx, input_name, overwrite ); }, reporter );
				break;
			case A_DIST_INFO:
				execute_stage( ctx, StageId::UnpackPjg, [&]() { return unpack_pjg( ctx ); },                       reporter );
				execute_stage( ctx, StageId::DumpDist,  [&]() { return dump_dist( ctx, input_name, overwrite ); }, reporter );
				break;
			case A_PGM_DUMP:
				execute_stage( ctx, StageId::UnpackPjg,   [&]() { return unpack_pjg( ctx ); },                      reporter );
				execute_stage( ctx, StageId::AdaptIcos,   [&]() { return adapt_icos( ctx ); },                      reporter );
				execute_stage( ctx, StageId::UnpredictDc, [&]() { return unpredict_dc( ctx ); },                    reporter );
				execute_stage( ctx, StageId::DumpPgm,     [&]() { return dump_pgm( ctx, input_name, overwrite ); }, reporter );
				break;
			default:
				break;
		}
	}
}

