#include "jpgwriter.h"

JpgWriter::JpgWriter(Writer& writer, const std::vector<Segment>& segments,
	const std::vector<std::uint8_t>& huffman_data,
	const std::vector<std::uint8_t>& garbage_data,
	const std::vector<std::size_t>& restart_marker_pos,
	const std::vector<std::uint8_t>& rst_err,
	const std::vector<std::size_t>& scan_pos) :
		writer_(writer),
		segments_(segments),
		huffman_data_(huffman_data),
		garbage_data_(garbage_data),
		restart_marker_pos_(restart_marker_pos),
		scan_pos_(scan_pos),
		rst_err_(rst_err) {}

void JpgWriter::write_scan_huffman_data(int& restart_pos, int scans_processed) {
	int corrected_pos = 0; // in-scan corrected rst marker position

	// write & expand huffman coded image data
	// ipos is the current position in image data.
	for (auto ipos = scan_pos_[scans_processed]; ipos < scan_pos_[scans_processed + 1]; ipos++) {
		// write current byte
		writer_.write_byte(huffman_data_[ipos]);
		// check current byte, stuff if needed
		if (huffman_data_[ipos] == 0xFF) {
			writer_.write_byte(std::uint8_t(0)); // 0xFF stuff value
		}
		// insert restart markers if needed
		if (!restart_marker_pos_.empty()) {
			if (ipos == restart_marker_pos_[restart_pos]) {
				const std::uint8_t restart_marker = 0xD0 + (corrected_pos % 8);
				constexpr std::uint8_t mrk = 0xFF; // marker start
				writer_.write_byte(mrk);
				writer_.write_byte(restart_marker);
				restart_pos++;
				corrected_pos++;
			}
		}
	}
	// insert false rst markers at end if needed
	if (!rst_err_.empty()) {
		while (rst_err_[scans_processed] > 0) {
			const std::uint8_t rst = 0xD0 + (corrected_pos % 8); // Restart marker
			constexpr std::uint8_t mrk = 0xFF; // marker start
			writer_.write_byte(mrk);
			writer_.write_byte(rst);
			corrected_pos++;
			rst_err_[scans_processed]--;
		}
	}
}

void JpgWriter::write() {
	int restart_pos = 0; // current restart marker position
	int scans_processed = 0; // number of current scan	

	constexpr std::array<std::uint8_t, 2> SOI{0xFF, 0xD8};
	writer_.write(SOI);

	// JPEG writing loop
	for (const auto& segment : segments_) {
		writer_.write(segment.get_data());

		if (segment.get_type() == Marker::kSOS) {
			write_scan_huffman_data(restart_pos, scans_processed);
			scans_processed++;
		}
	}

	constexpr std::array<std::uint8_t, 2> EOI{0xFF, 0xD9};
	writer_.write(EOI);

	if (!garbage_data_.empty()) {
		writer_.write(garbage_data_);
	}

	if (writer_.error()) {
		throw std::runtime_error("write error, possibly drive is full");
	}
}
