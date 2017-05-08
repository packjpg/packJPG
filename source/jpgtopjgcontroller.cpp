#include "jpgtopjgontroller.h"

#include <cstdint>
#include <string>

#include "jpgreader.h"
#include "jpgdecoder.h"
#include "pjgencoder.h"

JpgToPjgController::JpgToPjgController(Reader& jpg_input, Writer& pjg_output)
	: jpg_input_(jpg_input), pjg_output_(pjg_output) {}

JpgToPjgController::~JpgToPjgController() {}

void JpgToPjgController::execute() {
	auto reader = std::make_unique<JpgReader>(jpg_input_);
	reader->read();
	auto frame_info = reader->get_frame_info();
	auto segments = reader->get_segments();
	const auto& huffman_data = reader->get_huffman_data();
	const auto& rst_err = reader->get_rst_err();
	const auto& garbage_data = reader->get_garbage_data();

	auto jpeg_decoder = std::make_unique<JpgDecoder>(*frame_info, segments, huffman_data);
	jpeg_decoder->decode();

	this->check_value_range(frame_info->components);

	const auto padbit = jpeg_decoder->get_padbit();

	for (auto& component : frame_info->components) {
		component.adapt_icos();
		component.predict_dc();
		component.calc_zdst_lists();
	}

	auto pjg_encoder = std::make_unique<PjgEncoder>(pjg_output_);
	pjg_encoder->encode(padbit,
	                    frame_info->components,
	                    segments,
	                    rst_err,
	                    garbage_data);

}

void JpgToPjgController::check_value_range(const std::vector<Component>& components) const {
	for (const auto& component : components) {
		for (std::size_t freq = 0; freq < component.colldata.size(); freq++) {
			const auto& coefficients = component.colldata[freq];
			const auto absmax = component.max_v(freq);
			for (auto value : coefficients) {
				if (std::abs(value) > absmax) {
					throw std::range_error("value out of range error: cmp id: " + std::to_string(component.jid)
						+ ", frq " + std::to_string(freq)
						+ ", val " + std::to_string(value)
						+ ", max " + std::to_string(absmax));
				}
			}
		}
	}
}
