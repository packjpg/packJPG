#include "packjpglib.h"

#include "codec_pipeline.h"
#include "path_utils.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>

static_assert( kMsgSize == PJGLIB_MSG_SIZE, "PJGLIB_MSG_SIZE must match kMsgSize" );

namespace {

struct LibrarySession {
	CodecContext ctx;
	int         lib_in_type      = -1;
	int         lib_out_type     = -1;
	bool        has_init_attempt = false;
	std::string target_filepath;
	std::string staging_filepath;

	void discard_streams_and_staging() noexcept
	{
		if ( ( ctx.str_out != nullptr ) && ( lib_out_type != 0 ) ) {
			try {
				ctx.str_out->reset();
			} catch ( ... ) {
			}
		}
		ctx.str_in.reset();
		ctx.str_out.reset();
		if ( !staging_filepath.empty() ) {
			remove_file_utf8( staging_filepath );
			staging_filepath.clear();
		}
		target_filepath.clear();
		lib_in_type      = -1;
		lib_out_type     = -1;
		has_init_attempt = false;
	}

	~LibrarySession()
	{
		discard_streams_and_staging();
	}
};

LibrarySession& current_session()
{
	thread_local LibrarySession session;
	return session;
}

bool convert_session_impl(
	unsigned char** out_file,
	unsigned int* out_size,
	char* msg,
	bool require_mem_out )
{
	if ( out_file != nullptr ) {
		*out_file = nullptr;
	}
	if ( out_size != nullptr ) {
		*out_size = 0;
	}

	auto& sess = current_session();
	auto& ctx  = sess.ctx;

	struct SessionCleanupGuard {
		LibrarySession& s;
		bool committed = false;
		~SessionCleanupGuard()
		{
			if ( !committed ) {
				s.discard_streams_and_staging();
			} else {
				s.ctx.str_in.reset();
				s.ctx.str_out.reset();
				s.staging_filepath.clear();
				s.target_filepath.clear();
				s.lib_in_type      = -1;
				s.lib_out_type     = -1;
				s.has_init_attempt = false;
			}
		}
	} guard{ sess };

	try {
		ctx.auto_set = true;
		ctx.reset_buffers();
		ctx.action = A_COMPRESS;

		const auto begin = std::chrono::steady_clock::now();

		if ( !sess.has_init_attempt ) {
			ctx.errormessage = "streams not initialized";
			ctx.errorlevel   = 2;
		}
		else if ( ctx.errorlevel < ctx.err_tol && ( ctx.str_in == nullptr || ctx.str_out == nullptr ) ) {
			ctx.errormessage = "streams not initialized";
			ctx.errorlevel   = 2;
		}
		else if ( ctx.errorlevel < ctx.err_tol && require_mem_out && ( sess.lib_out_type != 1 ) ) {
			ctx.errormessage = "output stream is not initialized as memory";
			ctx.errorlevel   = 2;
		}
		else if ( ctx.errorlevel < ctx.err_tol && ( sess.lib_out_type == 1 ) &&
		          ( out_file == nullptr || out_size == nullptr ) ) {
			ctx.errormessage = "output memory pointers must not be null";
			ctx.errorlevel   = 2;
		}

		process_standard_file( ctx );

		if ( ctx.errorlevel < ctx.err_tol ) {
			if ( sess.lib_out_type == 1 ) {
				const std::size_t written = ctx.str_out->num_bytes_written();
				if ( written > static_cast<std::size_t>( std::numeric_limits<unsigned int>::max() ) ) {
					ctx.errormessage = "output exceeds 32-bit buffer size limit";
					ctx.errorlevel   = 2;
				}
				else {
					unsigned char* buf = ctx.str_out->get_c_data();
					if ( buf == nullptr ) {
						ctx.errormessage = kMemErrMsg;
						ctx.errorlevel   = 2;
					}
					else {
						*out_size = static_cast<unsigned int>( written );
						*out_file = buf;
						guard.committed = true;
					}
				}
			}
			else if ( sess.lib_out_type == 0 ) {
				if ( ( ctx.str_out == nullptr ) || !ctx.str_out->close() ) {
					ctx.errormessage = "write error, possibly drive is full";
					ctx.errorlevel   = 2;
				}
				else {
					ctx.str_out.reset();
					if ( !sess.staging_filepath.empty() ) {
						std::string commit_err;
						if ( !commit_staging_file( sess.staging_filepath, sess.target_filepath, commit_err ) ) {
							ctx.errormessage = commit_err;
							ctx.errorlevel   = 2;
						}
						else {
							sess.staging_filepath.clear();
							guard.committed = true;
						}
					}
					else {
						guard.committed = true;
					}
				}
			}
			else {
				if ( ( ctx.str_out == nullptr ) || !ctx.str_out->flush() || ctx.str_out->error() ) {
					ctx.errormessage = "write error, possibly drive is full";
					ctx.errorlevel   = 2;
				}
				else {
					guard.committed = true;
				}
			}
		}

		const auto end = std::chrono::steady_clock::now();

		if ( ctx.errorlevel >= ctx.err_tol ) {
			if ( msg != nullptr ) {
				std::snprintf( msg, kMsgSize, "%s", ctx.errormessage.c_str() );
			}
			return false;
		}

		const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>( end - begin ).count();
		const int total = ( elapsed_ms > 2147483647LL ) ? 2147483647 : static_cast<int>( elapsed_ms );
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
	} catch ( const std::bad_alloc& ) {
		if ( msg != nullptr ) {
			std::snprintf( msg, kMsgSize, "%s", kMemErrMsg );
		}
		return false;
	} catch ( const std::exception& e ) {
		if ( msg != nullptr ) {
			std::snprintf( msg, kMsgSize, "%s", e.what() );
		}
		return false;
	} catch ( ... ) {
		if ( msg != nullptr ) {
			std::snprintf( msg, kMsgSize, "unexpected internal exception" );
		}
		return false;
	}
}

} // namespace


/* -----------------------------------------------
	DLL export converter function
	----------------------------------------------- */
PJGLIB_API bool pjglib_convert_stream2stream( char* msg )
{
	return convert_session_impl( nullptr, nullptr, msg, false );
}


/* -----------------------------------------------
	DLL export converter function
	----------------------------------------------- */
PJGLIB_API bool pjglib_convert_file2file( const char* in, const char* out, char* msg )
{
	pjglib_init_streams(
		const_cast<void*>( static_cast<const void*>( in ) ), 0, 0,
		const_cast<void*>( static_cast<const void*>( out ) ), 0 );
	return convert_session_impl( nullptr, nullptr, msg, false );
}


/* -----------------------------------------------
	DLL export converter function
	----------------------------------------------- */
PJGLIB_API bool pjglib_convert_stream2mem( unsigned char** out_file, unsigned int* out_size, char* msg )
{
	return convert_session_impl( out_file, out_size, msg, true );
}


/* -----------------------------------------------
	DLL export deallocator for stream2mem buffers
	----------------------------------------------- */
PJGLIB_API void pjglib_free( void* ptr )
{
	std::free( ptr );
}


/* -----------------------------------------------
	DLL export init input (file/mem/FILE*)
	----------------------------------------------- */
PJGLIB_API void pjglib_init_streams( void* in_src, int in_type, int in_size, void* out_dest, int out_type )
{
	try {
		auto& sess = current_session();
		auto& ctx  = sess.ctx;

		unsigned char buffer[ 2 ] = { 0, 0 };

		sess.discard_streams_and_staging();
		sess.has_init_attempt = true;

		ctx.error_stage = StageId::None;
		ctx.errorlevel  = 0;
		ctx.errormessage.clear();
		ctx.filetype    = F_UNK;
		ctx.jpgfilename.clear();
		ctx.pjgfilename.clear();
		ctx.jpgfilesize = 0;
		ctx.pjgfilesize = 0;

		switch ( in_type ) {
			case 0:
				if ( in_src == nullptr ) {
					ctx.errormessage = "null input filename";
					ctx.errorlevel   = 2;
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
					ctx.errorlevel   = 2;
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
					ctx.errorlevel   = 2;
					return;
				}
				break;
			default:
				ctx.set_errormessage( "Invalid input type: %i", in_type );
				ctx.errorlevel = 2;
				return;
		}

		// Validate output parameters and self-clobber BEFORE opening any output file.
		std::string resolved_out_target;
		if ( out_type == 0 ) {
			if ( out_dest == nullptr || static_cast<const char*>( out_dest )[ 0 ] == '\0' ) {
				ctx.errormessage = "null or empty output filename";
				ctx.errorlevel   = 2;
				ctx.str_in.reset();
				return;
			}
			const char* out_cstr = static_cast<const char*>( out_dest );
			std::string target_err;
			if ( !resolve_overwrite_target_utf8( out_cstr, resolved_out_target, target_err ) ) {
				ctx.set_errormessage( "error opening output file %s", out_cstr );
				ctx.errorlevel = 2;
				ctx.str_in.reset();
				return;
			}
			if ( ( in_type == 0 ) &&
			     ( paths_refer_to_same_existing_file( static_cast<const char*>( in_src ), out_cstr ) ||
			       paths_refer_to_same_existing_file( static_cast<const char*>( in_src ), resolved_out_target ) ) ) {
				ctx.errormessage = "input and output refer to the same file";
				ctx.errorlevel   = 2;
				ctx.str_in.reset();
				return;
			}
			if ( !can_overwrite_or_create_target( resolved_out_target, target_err ) ) {
				ctx.set_errormessage( "error opening output file %s", out_cstr );
				ctx.errorlevel = 2;
				ctx.str_in.reset();
				return;
			}
		}
		else if ( out_type == 2 ) {
			if ( out_dest == nullptr ) {
				ctx.errormessage = "StreamWriter: null FILE* stream.";
				ctx.errorlevel   = 2;
				ctx.str_in.reset();
				return;
			}
		}
		else if ( out_type != 1 ) {
			ctx.set_errormessage( "Invalid output type: %i", out_type );
			ctx.errorlevel = 2;
			ctx.str_in.reset();
			return;
		}

		// Verify magic header BEFORE opening FileWriter so unknown/empty inputs never touch disk.
		if ( ctx.str_in->read( buffer, 2 ) != 2 ) {
			ctx.filetype     = F_UNK;
			ctx.errormessage = "filetype of input stream is unknown";
			ctx.errorlevel   = 2;
			ctx.str_in.reset();
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
			ctx.str_in.reset();
			return;
		}

		switch ( out_type ) {
			case 0: {
				const char* display_out = static_cast<const char*>( out_dest );
				if ( is_non_regular_stream_target( resolved_out_target ) ) {
					try {
						ctx.str_out = std::make_unique<FileWriter>( resolved_out_target );
						sess.target_filepath = std::move( resolved_out_target );
						sess.staging_filepath.clear();
					} catch ( const std::runtime_error& ) {
						ctx.set_errormessage( "error opening output file %s", display_out );
						ctx.errorlevel = 2;
						ctx.str_in.reset();
						return;
					}
				}
				else {
					unsigned int existing_mode = 0666u;
					const bool has_mode = query_existing_regular_file_mode( resolved_out_target, existing_mode );
					std::string staging_str;
					FILE* fp = open_exclusive_staging_stream( resolved_out_target, has_mode, existing_mode, staging_str );
					if ( fp == nullptr ) {
						ctx.set_errormessage( "error opening output file %s", display_out );
						ctx.errorlevel = 2;
						ctx.str_in.reset();
						return;
					}
					try {
						ctx.str_out = std::make_unique<FileWriter>( fp, staging_str );
						sess.target_filepath  = std::move( resolved_out_target );
						sess.staging_filepath = std::move( staging_str );
					} catch ( const std::runtime_error& ) {
						remove_file_utf8( staging_str );
						ctx.set_errormessage( "error opening output file %s", display_out );
						ctx.errorlevel = 2;
						ctx.str_in.reset();
						return;
					}
				}
				break;
			}
			case 1:
				ctx.str_out = std::make_unique<MemoryWriter>();
				break;
			case 2:
				try {
					ctx.str_out = std::make_unique<StreamWriter>( static_cast<FILE*>( out_dest ) );
				} catch ( const std::runtime_error& e ) {
					ctx.errormessage = e.what();
					ctx.errorlevel   = 2;
					ctx.str_in.reset();
					return;
				}
				break;
		}

		sess.lib_in_type  = in_type;
		sess.lib_out_type = out_type;
	} catch ( const std::bad_alloc& ) {
		auto& sess = current_session();
		sess.discard_streams_and_staging();
		sess.has_init_attempt = true;
		sess.ctx.errormessage = kMemErrMsg;
		sess.ctx.errorlevel   = 2;
	} catch ( const std::exception& e ) {
		auto& sess = current_session();
		sess.discard_streams_and_staging();
		sess.has_init_attempt = true;
		sess.ctx.errormessage = e.what();
		sess.ctx.errorlevel   = 2;
	} catch ( ... ) {
		auto& sess = current_session();
		sess.discard_streams_and_staging();
		sess.has_init_attempt = true;
		sess.ctx.errormessage = "unexpected internal exception";
		sess.ctx.errorlevel   = 2;
	}
}


/* -----------------------------------------------
	DLL export version information
	----------------------------------------------- */
PJGLIB_API const char* pjglib_version_info( void )
{
	static const std::string info = []() {
		char buf[ 256 ];
		std::snprintf( buf, sizeof( buf ), "--> %s library v%i.%i%s (%s) by %s <--",
			kAppTitle, kAppVersion / 10, kAppVersion % 10, kSubVersion, kVersionDate, kAuthor );
		return std::string( buf );
	}();
	return info.c_str();
}


/* -----------------------------------------------
	DLL export short name
	----------------------------------------------- */
PJGLIB_API const char* pjglib_short_name( void )
{
	static const std::string name = []() {
		char buf[ 64 ];
		std::snprintf( buf, sizeof( buf ), "%s v%i.%i%s",
			kAppTitle, kAppVersion / 10, kAppVersion % 10, kSubVersion );
		return std::string( buf );
	}();
	return name.c_str();
}
