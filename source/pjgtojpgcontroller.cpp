#include "pjgtojpgcontroller.h"

#include "jpgencoder.h"
#include "jpgwriter.h"
#include "pjgdecoder.h"

PjgToJpgController::PjgToJpgController(Reader& pjg_input, Writer& jpg_output)
	: pjg_input_(pjg_input), jpg_output_(jpg_output) {}

PjgToJpgController::PjgToJpgController(Reader& pjg_input, Writer& jpg_output, ImageDebug debug)
	: pjg_input_(pjg_input), jpg_output_(jpg_output), debug_(debug) {}

void PjgToJpgController::execute() {
	auto pjg_decoder = std::make_unique<PjgDecoder>(pjg_input_);
	auto [frame_info, segments, components, garbage_data, rst_err, padbit] = pjg_decoder->decode();

	if (debug_.options_.fcoll_dump) {
		debug_.dump_coll(components, debug_.options_.collmode);
	}

	if (debug_.options_.zdst_dump) {
		debug_.dump_zdst(components);
	}

	if (debug_.options_.txt_info) {
		debug_.dump_info(frame_info, components, segments);
	}

	if (debug_.options_.dist_info) {
		debug_.dump_dist(components);
	}

	for (auto& component : components) {
		component.adapt_icos();
		component.unpredict_dc();
	}

	if (debug_.options_.coll_dump) {
		debug_.dump_coll(components, debug_.options_.collmode);
	}

	if (debug_.options_.pgm_dump) {
		debug_.dump_pgm(components);
	}

	auto jpeg_encoder = std::make_unique<JpgEncoder>(frame_info, segments, padbit);
	const auto [huffman_data, restart_marker_pos, scan_pos] = jpeg_encoder->encode(components);

	if (debug_.options_.split_dump) {
		debug_.dump_header(segments);
		debug_.dump_huffman(huffman_data);
	}

	auto jpeg_writer = std::make_unique<JpgWriter>(jpg_output_);
	jpeg_writer->write(segments,
	                   huffman_data,
	                   garbage_data,
	                   restart_marker_pos,
	                   rst_err,
	                   scan_pos);
}
