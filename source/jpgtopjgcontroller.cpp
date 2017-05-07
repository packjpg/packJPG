#include "jpgtopjgontroller.h"

#include <cstdint>

#include "jpgreader.h"
#include "jpgdecoder.h"
#include "pjgencoder.h"

JpgToPjgController::JpgToPjgController(Reader& jpg_input, Writer& pjg_output)
	: jpg_input_(jpg_input), pjg_output_(pjg_output) {}

JpgToPjgController::~JpgToPjgController() {}

void JpgToPjgController::execute() {
	auto reader = std::make_unique<JpgReader>(jpg_input_);
	try {
		reader->read();
	} catch (const std::runtime_error&) {
		throw;
	}

	auto frame_info = reader->get_frame_info();
	auto segments = reader->get_segments();
	const auto& huffman_data = reader->get_huffman_data();
	const auto& rst_err = reader->get_rst_err();
	const auto& garbage_data = reader->get_garbage_data();

	auto jpeg_decoder = std::make_unique<JpgDecoder>(*frame_info, segments, huffman_data);
	try {
		jpeg_decoder->decode();
		jpeg_decoder->check_value_range(frame_info->components);
	} catch (const std::runtime_error&) {
		throw;
	}

	const auto padbit = jpeg_decoder->get_padbit();

	for (auto& component : frame_info->components) {
		component.adapt_icos();
		component.predict_dc();
		component.calc_zdst_lists();
	}

	try {
		auto pjg_encoder = std::make_unique<PjgEncoder>(pjg_output_);
		pjg_encoder->encode(padbit,
		                    frame_info->components,
		                    segments,
		                    rst_err,
		                    garbage_data);
	} catch (const std::runtime_error&) {
		throw;
	}
}
