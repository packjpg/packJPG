#include "codec_pipeline.h"

#include "jpeg_codec.h"
#include "pjg_codec.h"

#include <cstring>

void StageReporter::on_begin( StageId stage ) const
{
	if ( ( msgout != nullptr ) && ( verbosity == 2 ) ) {
		const char* text = stage_status_text( stage );
		std::fprintf( msgout, "\n%s ", text );
		for ( std::size_t i = std::strlen( text ); i <= 30; i++ ) {
			std::fprintf( msgout, " " );
		}
	}
}

void StageReporter::on_end( bool success, int elapsed_ms ) const
{
	if ( ( msgout != nullptr ) && ( verbosity == 2 ) ) {
		if ( success ) {
			std::fprintf( msgout, "%6ims", ( elapsed_ms >= 0 ) ? elapsed_ms : -1 );
		}
		else {
			std::fprintf( msgout, "%8s", "ERROR" );
		}
	}
}

void run_compress_pipeline( CodecContext& ctx, const StageReporter* reporter )
{
	execute_stage( ctx, StageId::ReadJpeg,        [&]() { return read_jpeg( ctx ); },        reporter );
	execute_stage( ctx, StageId::DecodeJpeg,      [&]() { return decode_jpeg( ctx ); },      reporter );
	execute_stage( ctx, StageId::CheckValueRange, [&]() { return check_value_range( ctx ); }, reporter );
	execute_stage( ctx, StageId::AdaptIcos,       [&]() { return adapt_icos( ctx ); },       reporter );
	execute_stage( ctx, StageId::PredictDc,       [&]() { return predict_dc( ctx ); },       reporter );
	execute_stage( ctx, StageId::CalcZdstLists,   [&]() { return calc_zdst_lists( ctx ); },  reporter );
	execute_stage( ctx, StageId::PackPjg,         [&]() { return pack_pjg( ctx ); },         reporter );
}

void run_decompress_pipeline( CodecContext& ctx, const StageReporter* reporter )
{
	execute_stage( ctx, StageId::UnpackPjg,   [&]() { return unpack_pjg( ctx ); },   reporter );
	execute_stage( ctx, StageId::AdaptIcos,   [&]() { return adapt_icos( ctx ); },   reporter );
	execute_stage( ctx, StageId::UnpredictDc, [&]() { return unpredict_dc( ctx ); }, reporter );
	execute_stage( ctx, StageId::RecodeJpeg,  [&]() { return recode_jpeg( ctx ); },  reporter );
	execute_stage( ctx, StageId::MergeJpeg,   [&]() { return merge_jpeg( ctx ); },   reporter );
}

void process_standard_file( CodecContext& ctx, const StageReporter* reporter )
{
	if ( ctx.filetype == F_JPG ) {
		run_compress_pipeline( ctx, reporter );
	}
	else if ( ctx.filetype == F_PJG ) {
		run_decompress_pipeline( ctx, reporter );
	}
	ctx.reset_buffers();
}

namespace packjpg {

bool convert_memory(
	const std::uint8_t* in_data,
	std::size_t in_size,
	std::vector<std::uint8_t>& out_data,
	std::string* error_msg,
	bool discard_meta,
	int err_tol )
{
	out_data.clear();
	if ( ( in_data == nullptr ) || ( in_size < 2 ) ) {
		if ( error_msg != nullptr ) {
			*error_msg = "input buffer too small";
		}
		return false;
	}

	CodecContext ctx;
	ctx.disc_meta = discard_meta;
	ctx.err_tol   = err_tol;
	ctx.auto_set  = true;
	ctx.action    = A_COMPRESS;

	ctx.str_in = std::make_unique<MemoryReader>( in_data, in_size );
	auto mem_out = std::make_unique<MemoryWriter>();
	MemoryWriter* mem_out_ptr = mem_out.get();
	ctx.str_out = std::move( mem_out );

	unsigned char magic[ 2 ] = { 0, 0 };
	ctx.str_in->read( magic, 2 );
	if ( ( magic[ 0 ] == 0xFF ) && ( magic[ 1 ] == 0xD8 ) ) {
		ctx.filetype    = F_JPG;
		ctx.jpgfilename = "JPG in memory";
		ctx.pjgfilename = "PJG in memory";
	}
	else if ( ( magic[ 0 ] == kPjgMagic[ 0 ] ) && ( magic[ 1 ] == kPjgMagic[ 1 ] ) ) {
		ctx.filetype    = F_PJG;
		ctx.pjgfilename = "PJG in memory";
		ctx.jpgfilename = "JPG in memory";
	}
	else {
		if ( error_msg != nullptr ) {
			*error_msg = "filetype of input stream is unknown";
		}
		return false;
	}

	ctx.reset_buffers();
	process_standard_file( ctx );

	if ( ctx.errorlevel >= ctx.err_tol ) {
		if ( error_msg != nullptr ) {
			*error_msg = ctx.errormessage;
		}
		return false;
	}

	out_data = mem_out_ptr->take_data();
	if ( error_msg != nullptr ) {
		error_msg->clear();
	}
	return true;
}

} // namespace packjpg
