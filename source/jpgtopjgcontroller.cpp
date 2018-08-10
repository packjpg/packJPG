#include "jpgtopjgontroller.h"

#include <string>

#include "jpgreader.h"
#include "jpgdecoder.h"
#include "pjgencoder.h"
#include "segmentparser.h"

JpgToPjgController::JpgToPjgController(Reader& jpg_input, Writer& pjg_output)
	: jpg_input_(jpg_input), pjg_output_(pjg_output) {}

JpgToPjgController::JpgToPjgController(Reader& jpg_input, Writer& pjg_output, ImageDebug debug)
	: jpg_input_(jpg_input), pjg_output_(pjg_output), debug_(debug) {}

void JpgToPjgController::execute() {
	auto reader = std::make_unique<JpgReader>(jpg_input_);
	auto [segments, huffman_data] = reader->read();
	auto [frame_info, components] = SegmentParser::get_frame_info(segments);

	if (debug_.options_.split_dump) {
		debug_.dump_header(segments);
		debug_.dump_huffman(huffman_data);
	}


	if (debug_.options_.txt_info) {
		debug_.dump_info(frame_info, components, segments);
	}

	auto jpeg_decoder = std::make_unique<JpgDecoder>(frame_info, segments, huffman_data);
	jpeg_decoder->decode(components);

	if (debug_.options_.coll_dump) {
		debug_.dump_coll(components, debug_.options_.collmode);
	}

	this->check_value_range(components);

	for (auto& component : components) {
		component.adapt_icos();
	}

	if (debug_.options_.pgm_dump) {
		debug_.dump_pgm(components);
	}

	for (auto& component : components) {
		component.predict_dc();
	}

	if (debug_.options_.fcoll_dump) {
		debug_.dump_coll(components, debug_.options_.collmode);
	}

	if (debug_.options_.dist_info) {
		debug_.dump_dist(components);
	}

	if (debug_.options_.zdst_dump) {
		debug_.dump_zdst(components);
	}

	auto pjg_encoder = std::make_unique<PjgEncoder>(pjg_output_);
	pjg_encoder->encode(jpeg_decoder->get_padbit(),
	                    components,
	                    segments,
	                    reader->get_rst_err(),
	                    reader->get_garbage_data());

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
