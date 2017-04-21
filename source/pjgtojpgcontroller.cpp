#include "pjgtojpgcontroller.h"

#include <cstdint>

#include "jpgencoder.h"
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

	std::unique_ptr<JpgEncoder> jpeg_encoder = std::make_unique<JpgEncoder>(segments);

	try {
		jpeg_encoder->recode(*frame_info, padbit);
		jpeg_encoder->merge(jpg_output_, garbage_data, rst_err);
	} catch (const std::exception&) {
		throw;
	}

	// get filesize
	//jpgfilesize = jpg_output_->num_bytes_written();

}