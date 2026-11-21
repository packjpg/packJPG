#include "packjpglib.h"

#include "codec_pipeline.h"
#include "path_utils.h"

#include <cstdio>
#include <ctime>
#include <memory>
#include <stdexcept>

namespace {

struct LibrarySession {
	CodecContext ctx;
	int lib_in_type  = -1;
	int lib_out_type = -1;
};

LibrarySession& current_session()
{
	thread_local LibrarySession session;
	return session;
}

} // namespace


/* -----------------------------------------------
	DLL export converter function
	----------------------------------------------- */
EXPORT bool pjglib_convert_stream2stream( char* msg )
{
	return pjglib_convert_stream2mem( nullptr, nullptr, msg );
}


/* -----------------------------------------------
	DLL export converter function
	----------------------------------------------- */
EXPORT bool pjglib_convert_file2file( char* in, char* out, char* msg )
{
	pjglib_init_streams( static_cast<void*>( in ), 0, 0, static_cast<void*>( out ), 0 );
	return pjglib_convert_stream2mem( nullptr, nullptr, msg );
}


/* -----------------------------------------------
	DLL export converter function
	----------------------------------------------- */
EXPORT bool pjglib_convert_stream2mem( unsigned char** out_file, unsigned int* out_size, char* msg )
{
	auto& sess = current_session();
	auto& ctx  = sess.ctx;

	ctx.auto_set = true;
	ctx.reset_buffers();
	ctx.action = A_COMPRESS;

	const std::clock_t begin = std::clock();

	if ( ctx.errorlevel < ctx.err_tol && ( ctx.str_in == nullptr || ctx.str_out == nullptr ) ) {
		ctx.errormessage = "streams not initialized";
		ctx.errorlevel = 2;
	}

	process_standard_file( ctx );

	if ( ctx.errorlevel < ctx.err_tol ) {
		if ( ( sess.lib_out_type == 1 ) && ( out_file != nullptr ) && ( out_size != nullptr ) ) {
			*out_size = static_cast<unsigned int>( ctx.str_out->num_bytes_written() );
			*out_file = ctx.str_out->get_c_data();
		}
		else if ( ( ctx.str_out != nullptr ) && !ctx.str_out->flush() ) {
			ctx.errormessage = "write error, possibly drive is full";
			ctx.errorlevel = 2;
		}
	}
	else if ( ctx.str_out != nullptr ) {
		ctx.str_out->reset();
	}

	ctx.str_in.reset();
	ctx.str_out.reset();

	const std::clock_t end = std::clock();

	if ( ctx.errorlevel >= ctx.err_tol ) {
		if ( sess.lib_out_type == 0 ) {
			if ( ctx.filetype == F_JPG ) {
				if ( file_exists( ctx.pjgfilename ) ) std::remove( ctx.pjgfilename.c_str() );
			}
			else if ( ctx.filetype == F_PJG ) {
				if ( file_exists( ctx.jpgfilename ) ) std::remove( ctx.jpgfilename.c_str() );
			}
		}
		if ( msg != nullptr ) {
			std::snprintf( msg, kMsgSize, "%s", ctx.errormessage.c_str() );
		}
		return false;
	}

	const int total = static_cast<int>( ( static_cast<double>( ( end - begin ) * 1000 ) ) / CLOCKS_PER_SEC );
	const float cr  = ( ctx.jpgfilesize > 0 ) ? static_cast<float>( 100.0 * ctx.pjgfilesize / ctx.jpgfilesize ) : 0.0f;

	if ( msg != nullptr ) {
		switch ( ctx.filetype ) {
			case F_JPG:
				std::snprintf( msg, kMsgSize, "Compressed to %s (%.2f%%) in %ims",
					ctx.pjgfilename.c_str(), cr, ( total >= 0 ) ? total : -1 );
				break;
			case F_PJG:
				std::snprintf( msg, kMsgSize, "Decompressed to %s (%.2f%%) in %ims",
					ctx.jpgfilename.c_str(), cr, ( total >= 0 ) ? total : -1 );
				break;
			case F_UNK:
			default:
				std::snprintf( msg, kMsgSize, "Unknown filetype" );
				break;
		}
	}

	return true;
}


/* -----------------------------------------------
	DLL export init input (file/mem/FILE*)
	----------------------------------------------- */
EXPORT void pjglib_init_streams( void* in_src, int in_type, int in_size, void* out_dest, int out_type )
{
	auto& sess = current_session();
	auto& ctx  = sess.ctx;

	unsigned char buffer[ 2 ] = { 0, 0 };

	ctx.error_stage = StageId::None;
	ctx.errorlevel  = 0;
	ctx.jpgfilesize = 0;
	ctx.pjgfilesize = 0;
	ctx.str_in.reset();
	ctx.str_out.reset();

	switch ( in_type ) {
		case 0:
			if ( in_src == nullptr ) {
				ctx.errormessage = "null input filename";
				ctx.errorlevel = 2;
				return;
			}
			try {
				ctx.str_in = std::make_unique<FileReader>( static_cast<const char*>( in_src ) );
			} catch ( const std::runtime_error& ) {
				ctx.set_errormessage( "error opening input file %s", static_cast<const char*>( in_src ) );
				ctx.errorlevel = 2;
				return;
			}
			break;
		case 1:
			if ( ( in_size < 0 ) || ( ( in_src == nullptr ) && ( in_size > 0 ) ) ) {
				ctx.errormessage = "invalid input memory buffer";
				ctx.errorlevel = 2;
				return;
			}
			ctx.str_in = std::make_unique<MemoryReader>(
				static_cast<const unsigned char*>( in_src ),
				static_cast<std::size_t>( in_size ) );
			break;
		case 2:
			try {
				ctx.str_in = std::make_unique<StreamReader>( static_cast<FILE*>( in_src ) );
			} catch ( const std::runtime_error& e ) {
				ctx.errormessage = e.what();
				ctx.errorlevel = 2;
				return;
			}
			break;
		default:
			ctx.set_errormessage( "Invalid input type: %i", in_type );
			ctx.errorlevel = 2;
			return;
	}

	switch ( out_type ) {
		case 0:
			if ( out_dest == nullptr ) {
				ctx.errormessage = "null output filename";
				ctx.errorlevel = 2;
				return;
			}
			try {
				ctx.str_out = std::make_unique<FileWriter>( static_cast<const char*>( out_dest ) );
			} catch ( const std::runtime_error& ) {
				ctx.set_errormessage( "error opening output file %s", static_cast<const char*>( out_dest ) );
				ctx.errorlevel = 2;
				return;
			}
			break;
		case 1:
			ctx.str_out = std::make_unique<MemoryWriter>();
			break;
		case 2:
			try {
				ctx.str_out = std::make_unique<StreamWriter>( static_cast<FILE*>( out_dest ) );
			} catch ( const std::runtime_error& e ) {
				ctx.errormessage = e.what();
				ctx.errorlevel = 2;
				return;
			}
			break;
		default:
			ctx.set_errormessage( "Invalid output type: %i", out_type );
			ctx.errorlevel = 2;
			return;
	}

	ctx.jpgfilename.clear();
	ctx.pjgfilename.clear();

	if ( ctx.str_in->read( buffer, 2 ) != 2 ) {
		ctx.filetype     = F_UNK;
		ctx.errormessage = "filetype of input stream is unknown";
		ctx.errorlevel   = 2;
		return;
	}

	if ( ( buffer[ 0 ] == 0xFF ) && ( buffer[ 1 ] == 0xD8 ) ) {
		ctx.filetype    = F_JPG;
		ctx.jpgfilename = ( in_type == 0 )  ? static_cast<const char*>( in_src )   : "JPG in memory";
		ctx.pjgfilename = ( out_type == 0 ) ? static_cast<const char*>( out_dest ) : "PJG in memory";
	}
	else if ( ( buffer[ 0 ] == kPjgMagic[ 0 ] ) && ( buffer[ 1 ] == kPjgMagic[ 1 ] ) ) {
		ctx.filetype    = F_PJG;
		ctx.pjgfilename = ( in_type == 0 )  ? static_cast<const char*>( in_src )   : "PJG in memory";
		ctx.jpgfilename = ( out_type == 0 ) ? static_cast<const char*>( out_dest ) : "JPG in memory";
	}
	else {
		ctx.filetype     = F_UNK;
		ctx.errormessage = "filetype of input stream is unknown";
		ctx.errorlevel   = 2;
		return;
	}

	sess.lib_in_type  = in_type;
	sess.lib_out_type = out_type;
}


/* -----------------------------------------------
	DLL export version information
	----------------------------------------------- */
EXPORT const char* pjglib_version_info( void )
{
	return "--> packJPG library v2.5k (01/22/2016) by Matthias Stirner / Se <--";
}


/* -----------------------------------------------
	DLL export short name
	----------------------------------------------- */
EXPORT const char* pjglib_short_name( void )
{
	return "packJPG v2.5k";
}
