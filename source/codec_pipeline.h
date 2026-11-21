#ifndef CODEC_PIPELINE_H
#define CODEC_PIPELINE_H

#include "codec_context.h"

#include <ctime>
#include <stdexcept>
#include <string>
#include <vector>

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

	std::clock_t begin = 0;
	if ( reporter != nullptr ) {
		reporter->on_begin( stage );
		begin = std::clock();
	}

	bool success = false;
	try {
		success = fn();
	} catch ( const std::bad_alloc& ) {
		ctx.errormessage = kMemErrMsg;
		ctx.errorlevel = 2;
	} catch ( const std::runtime_error& e ) {
		ctx.errormessage = e.what();
		ctx.errorlevel = 2;
	}

	if ( ( ctx.errorlevel > 0 ) && ( ctx.error_stage == StageId::None ) ) {
		ctx.error_stage = stage;
	}
	if ( !success ) {
		ctx.error_stage = stage;
	}

	if ( reporter != nullptr ) {
		const std::clock_t end = std::clock();
		const int total = static_cast<int>( ( static_cast<double>( ( end - begin ) * 1000 ) ) / CLOCKS_PER_SEC );
		reporter->on_end( success, total );
	}
}

void run_compress_pipeline( CodecContext& ctx, const StageReporter* reporter = nullptr );
void run_decompress_pipeline( CodecContext& ctx, const StageReporter* reporter = nullptr );
void process_standard_file( CodecContext& ctx, const StageReporter* reporter = nullptr );

namespace packjpg {

// Reentrant, thread-safe in-memory conversion (JPG -> PJG or PJG -> JPG).
bool convert_memory(
	const std::uint8_t* in_data,
	std::size_t in_size,
	std::vector<std::uint8_t>& out_data,
	std::string* error_msg = nullptr,
	bool discard_meta = false,
	int err_tol = 1 );

} // namespace packjpg

#endif
