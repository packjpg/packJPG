#ifndef CODEC_PIPELINE_H
#define CODEC_PIPELINE_H

#include "codec_context.h"

#include <chrono>
#include <exception>
#include <new>
#include <string>
#include <vector>

#if defined(BUILD_DLL)
	#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__)
		#define PJG_PUBLIC_API __declspec( dllexport )
	#elif defined(__GNUC__) && ( __GNUC__ >= 4 )
		#define PJG_PUBLIC_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define PJG_PUBLIC_API
	#endif
#elif defined(USE_PACKJPG_DLL) && !defined(BUILD_LIB) && ( defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) )
	#define PJG_PUBLIC_API __declspec( dllimport )
#else
	#define PJG_PUBLIC_API
#endif

struct StageReporter {
	FILE* msgout    = nullptr;
	int   verbosity = 0;

	void on_begin( StageId stage ) const;
	void on_end( bool success, int elapsed_ms ) const;
};

template <typename Fn>
void execute_stage( CodecContext& ctx, StageId stage, Fn&& fn, const StageReporter* reporter = nullptr )
{
	if ( ctx.errorlevel >= ctx.err_tol ) {
		return;
	}

	std::chrono::steady_clock::time_point begin{};
	if ( reporter != nullptr ) {
		reporter->on_begin( stage );
		begin = std::chrono::steady_clock::now();
	}

	bool success = false;
	try {
		success = fn();
	} catch ( const std::bad_alloc& ) {
		ctx.errormessage = kMemErrMsg;
		ctx.errorlevel = 2;
	} catch ( const std::exception& e ) {
		ctx.errormessage = e.what();
		ctx.errorlevel = 2;
	} catch ( ... ) {
		ctx.errormessage = "unexpected internal exception";
		ctx.errorlevel = 2;
	}

	if ( ( ctx.errorlevel > 0 ) && ( ctx.error_stage == StageId::None ) ) {
		ctx.error_stage = stage;
	}
	if ( !success ) {
		ctx.error_stage = stage;
		if ( ctx.errorlevel < ctx.err_tol ) {
			ctx.errorlevel = 2;
		}
	}

	if ( reporter != nullptr ) {
		const auto end = std::chrono::steady_clock::now();
		const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>( end - begin ).count();
		const int total = ( ms > 2147483647LL ) ? 2147483647 : static_cast<int>( ms );
		reporter->on_end( success, total );
	}
}

void run_compress_pipeline( CodecContext& ctx, const StageReporter* reporter = nullptr );
void run_decompress_pipeline( CodecContext& ctx, const StageReporter* reporter = nullptr );
void process_standard_file( CodecContext& ctx, const StageReporter* reporter = nullptr );

namespace packjpg {

// Reentrant, thread-safe in-memory conversion (JPG -> PJG or PJG -> JPG).
PJG_PUBLIC_API bool convert_memory(
	const std::uint8_t* in_data,
	std::size_t in_size,
	std::vector<std::uint8_t>& out_data,
	std::string* error_msg = nullptr,
	bool discard_meta = false,
	int err_tol = 1 );

} // namespace packjpg

#endif

