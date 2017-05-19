#include "pjgtojpgcontroller.h"

#include "jpgencoder.h"
#include "jpgwriter.h"
#include "pjgdecoder.h"

PjgToJpgController::PjgToJpgController(Reader& pjg_input, Writer& jpg_output)
	: pjg_input_(pjg_input), jpg_output_(jpg_output) {}

PjgToJpgController::~PjgToJpgController() {}

void PjgToJpgController::execute() {
	auto pjg_decoder = std::make_unique<PjgDecoder>(pjg_input_);
	pjg_decoder->decode();

	auto frame_info = pjg_decoder->get_frame_info();
	const auto segments = pjg_decoder->get_segments();

	auto jpeg_encoder = std::make_unique<JpgEncoder>(*frame_info, segments, pjg_decoder->get_padbit());
	jpeg_encoder->encode();

	auto jpeg_writer = std::make_unique<JpgWriter>(jpg_output_,
	                                               segments,
	                                               jpeg_encoder->get_huffman_data(),
	                                               pjg_decoder->get_garbage_data(),
	                                               jpeg_encoder->get_restart_marker_pos(),
	                                               pjg_decoder->get_rst_err(),
	                                               jpeg_encoder->get_scan_pos());
	jpeg_writer->write();
}
