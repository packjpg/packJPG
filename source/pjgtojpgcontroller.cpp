#include "pjgtojpgcontroller.h"

#include <cstdint>

#include "jpgencoder.h"
#include "jpgwriter.h"
#include "pjgdecoder.h"

PjgToJpgController::PjgToJpgController(Reader& pjg_input, Writer& jpg_output) :
	pjg_input_(pjg_input),
	jpg_output_(jpg_output)
	{}

PjgToJpgController::~PjgToJpgController() {
}

void PjgToJpgController::execute() {
	//pjgfilesize = pjg_input_->get_size();
	std::unique_ptr<FrameInfo> frame_info;
	std::vector<Segment> segments;
	std::vector<std::uint8_t> garbage_data;
	std::vector<std::uint8_t> rst_err;
	std::uint8_t padbit;
	try {
		auto pjg_decoder = std::make_unique<PjgDecoder>(pjg_input_);
		pjg_decoder->decode();

		frame_info = pjg_decoder->get_frame_info();
		segments = pjg_decoder->get_segments();
		padbit = pjg_decoder->get_padbit();
		rst_err = pjg_decoder->get_rst_err();
		garbage_data = pjg_decoder->get_garbage_data();
	} catch (const std::exception&) {
		throw;
	}

	for (auto& component : frame_info->components) {
		component.adapt_icos();
		component.unpredict_dc();
	}

	auto jpeg_encoder = std::make_unique<JpgEncoder>(*frame_info, segments, padbit);

	try {
		jpeg_encoder->recode();
	} catch (const std::exception&) {
		throw;
	}

	auto jpeg_writer = std::make_unique<JpgWriter>(jpg_output_,
	                                               segments,
	                                               jpeg_encoder->get_huffman_data(),
	                                               garbage_data,
	                                               jpeg_encoder->get_restart_marker_pos(),
	                                               rst_err,
	                                               jpeg_encoder->get_scan_pos());
	try {
		jpeg_writer->write();
	} catch(const std::runtime_error&) {
		throw;
	}

	// get filesize
	//jpgfilesize = jpg_output_->num_bytes_written();

}