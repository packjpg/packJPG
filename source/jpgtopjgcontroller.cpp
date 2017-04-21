#include "jpgtopjgontroller.h"

#include <cstdint>

#include "jpgreader.h"
#include "jpgdecoder.h"
#include "pjgencoder.h"

JpgToPjgController::JpgToPjgController(Reader& jpg_input, Writer& pjg_output) : jpg_input_(jpg_input), pjg_output_(pjg_output) {

}


JpgToPjgController::~JpgToPjgController() {
}

void JpgToPjgController::execute() {
	//jpgfilesize = jpg_input_->get_size();

	std::unique_ptr<FrameInfo> frame_info;
	std::vector<Segment> segments;
	std::vector<std::uint8_t> huffman_data;
	std::vector<std::uint8_t> garbage_data;
	std::vector<std::uint8_t> rst_err;

	auto reader = std::make_unique<JpgReader>(jpg_input_);
	try {
		reader->read();

		frame_info = reader->get_frame_info();
		segments = reader->get_segments();
		huffman_data = reader->get_huffman_data();
		rst_err = reader->get_rst_err();
		garbage_data = reader->get_garbage_data();
	} catch (const std::exception&) {
		throw;
	}

	std::uint8_t padbit;

	std::unique_ptr<JpgDecoder> jpeg_decoder = std::make_unique<JpgDecoder>();
	try {
		jpeg_decoder->decode(*frame_info, segments, huffman_data);
		padbit = jpeg_decoder->get_padbit();
		jpeg_decoder->check_value_range(frame_info->components);
	} catch (const std::exception&) {
		throw;
	}

	for (auto& component : frame_info->components) {
		component.adapt_icos();
		component.predict_dc();
		component.calc_zdst_lists();
	}

	try {
		auto pjg_encoder = std::make_unique<PjgEncoder>(pjg_output_);
		pjg_encoder->encode(padbit, frame_info->components, segments, rst_err, garbage_data);
	} catch (const std::exception&) {
		throw;
	}
	//pjgfilesize = pjg_output_->num_bytes_written();
}