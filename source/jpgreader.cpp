#include "jpgreader.h"

#include <string>

#include "jfifparse.h"
#include "segment.h"

JpgReader::JpgReader(Reader& reader) : reader_(reader) {}

std::vector<Segment> JpgReader::parse_segments() {
	std::vector<Segment> segments;
	while (!reader_.end_of_reader()) {
		Segment segment(reader_);
		if (segment.get_type() == Marker::kEOI) {
			break;
		}

		segments.push_back(segment);

		if (segment.get_type() == Marker::kSOS) {
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
	try {
		segments_ = parse_segments();
		garbage_data_ = read_garbage_data();
		frame_info_ = jfif::get_frame_info(segments_);
	} catch (const std::runtime_error&) {
		throw;
	}
	huffman_data_ = huffman_writer_->get_data();
}


void JpgReader::read_sos() {
	// switch to huffman data reading mode
	int cpos = 0; // rst marker counter
	std::uint32_t crst = 0; // current rst marker counter
	while (true) {
		// read byte from imagedata
		std::uint8_t byte = reader_.read_byte();

		// non-0xFF loop
		if (byte != 0xFF) {
			crst = 0;
			while (byte != 0xFF) {
				huffman_writer_->write_byte(byte);
				byte = reader_.read_byte();
			}
		}

		// treatment of 0xFF
		if (byte == 0xFF) {
			byte = reader_.read_byte();
			if (byte == 0x00) {
				crst = 0;
				// no zeroes needed -> ignore 0x00. write 0xFF
				huffman_writer_->write_byte(0xFF);
			} else if (byte == 0xD0 + (cpos % 8)) { // restart marker
				// increment rst counters
				cpos++;
				crst++;
			} else { // in all other cases leave it to the header parser routines
				// store number of wrongly set rst markers
				if (crst > 0) {
					if (rst_err_.empty()) {
						rst_err_.resize(scans_processed_ + 1);
					}
				}
				if (!rst_err_.empty()) {
					// realloc and set only if needed
					rst_err_.resize(scans_processed_ + 1);
					if (crst > 255) {
						throw std::runtime_error("Severe false use of RST markers (" + std::to_string(crst) + ")");
					}
					rst_err_[scans_processed_] = crst;
				}
				reader_.rewind_bytes(2); // Start of the next segment: unread these.
				break;
			}
		} else {
			// otherwise this means end-of-file, so break out
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
