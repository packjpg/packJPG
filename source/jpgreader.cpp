#include "jpgreader.h"

#include <string>

#include "jfifparse.h"

JpgReader::JpgReader(Reader& reader) : reader_(reader) {}

std::vector<Segment> JpgReader::parse_segments() {
	std::vector<Segment> segments;
	while (!reader_.end_of_reader()) {
		Segment segment(reader_);
		if (segment.get_type() == Marker::EOI) {
			break; // Don't read garbage data (data occurring after EOI) here.
		}

		segments.push_back(segment);

		if (segment.get_type() == Marker::SOS) {
			// Read the compressed data:
			read_sos();
			scans_processed_++;
		}
	}
	return segments;
}

std::vector<std::uint8_t> JpgReader::read_garbage_data() {
	if (reader_.end_of_reader()) {
		return std::vector<std::uint8_t>();
	}

	const auto garbage_amount = reader_.get_size() - reader_.num_bytes_read();
	std::vector<std::uint8_t> garbage(garbage_amount);
	if (reader_.read(garbage, garbage_amount) != garbage_amount) {
		throw std::runtime_error("Unable to read garbage data.");
	}

	return garbage;
}

void JpgReader::read() {
	segments_ = parse_segments();
	garbage_data_ = read_garbage_data();
	frame_info_ = jfif::get_frame_info(segments_);
}


void JpgReader::read_sos() {
	int restart_marker_counter = 0;
	int curr_restart_marker_counter = 0;
	while (!reader_.end_of_reader()) {
		auto byte = reader_.read_byte();

		if (byte != 0xFF) {
			curr_restart_marker_counter = 0;
			while (byte != 0xFF) {
				huffman_data_.emplace_back(byte);
				byte = reader_.read_byte();
			}
		}

		byte = reader_.read_byte();
		if (byte == 0x00) {
			curr_restart_marker_counter = 0;
			// no zeroes needed -> ignore 0x00. write 0xFF
			huffman_data_.emplace_back(0xFF);
		} else if (byte == 0xD0 + (restart_marker_counter % 8)) {
			// restart marker
			restart_marker_counter++;
			curr_restart_marker_counter++;
		} else {
			// Otherwise it's the start of another segment.
			if (curr_restart_marker_counter > 0) {
				// Store the number of wrongly-set restart markers:
				if (rst_err_.empty()) {
					rst_err_.resize(scans_processed_ + 1);
				}
			}
			if (!rst_err_.empty()) {
				rst_err_.resize(scans_processed_ + 1);
				if (curr_restart_marker_counter > 255) {
					throw std::runtime_error("Severe false use of RST markers (" + std::to_string(curr_restart_marker_counter) + ")");
				}
				rst_err_[scans_processed_] = curr_restart_marker_counter;
			}
			reader_.rewind_bytes(2); // Unread the start of the next segment.
			break;
		}
	}
}

std::vector<Segment> JpgReader::get_segments() {
	return segments_;
}

std::unique_ptr<FrameInfo> JpgReader::get_frame_info() {
	return std::move(frame_info_);
}

std::vector<std::uint8_t> JpgReader::get_huffman_data() {
	return huffman_data_;
}

std::vector<std::uint8_t> JpgReader::get_garbage_data() {
	return garbage_data_;
}

std::vector<std::uint8_t> JpgReader::get_rst_err() {
	return rst_err_;
}
