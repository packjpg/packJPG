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
	pjg_decoder->decode();

	auto frame_info = pjg_decoder->get_frame_info();
	const auto segments = pjg_decoder->get_segments();

	if (debug_.options_.fcoll_dump) {
		debug_.dump_coll(frame_info->components, debug_.options_.collmode);
	}

	if (debug_.options_.zdst_dump) {
		debug_.dump_zdst(frame_info->components);
	}

	if (debug_.options_.txt_info) {
		debug_.dump_info(*frame_info, segments);
	}

	if (debug_.options_.dist_info) {
		debug_.dump_dist(frame_info->components);
	}

	for (auto& component : frame_info->components) {
		component.adapt_icos();
		component.unpredict_dc();
	}

	if (debug_.options_.coll_dump) {
		debug_.dump_coll(frame_info->components, debug_.options_.collmode);
	}

	if (debug_.options_.pgm_dump) {
		debug_.dump_pgm(frame_info->components);
	}

	auto jpeg_encoder = std::make_unique<JpgEncoder>(*frame_info, segments, pjg_decoder->get_padbit());
	jpeg_encoder->encode();

	if (debug_.options_.split_dump) {
		debug_.dump_header(segments);
		debug_.dump_huffman(jpeg_encoder->get_huffman_data());
	}

	auto jpeg_writer = std::make_unique<JpgWriter>(jpg_output_,
	                                               segments,
	                                               jpeg_encoder->get_huffman_data(),
	                                               pjg_decoder->get_garbage_data(),
	                                               jpeg_encoder->get_restart_marker_pos(),
	                                               pjg_decoder->get_rst_err(),
	                                               jpeg_encoder->get_scan_pos());
	jpeg_writer->write();
}
