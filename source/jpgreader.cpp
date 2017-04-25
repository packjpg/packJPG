#include "jpgreader.h"

#include <string>

#include "jfifparse.h"
#include "segment.h"

JpgReader::JpgReader(Reader& jpg_input_reader) : jpg_input_reader_(jpg_input_reader) {}

void JpgReader::read() {
	scan_count_ = 0;
	// start headerwriter
	auto header_writer = std::make_unique<MemoryWriter>();

	// start huffman writer
	auto huffman_writer = std::make_unique<MemoryWriter>();

	// alloc memory for segment data first
	std::vector<std::uint8_t> segment(1024);

	Marker type = Marker::kINVALID;
	while (true) {
		if (type == Marker::kSOS) { // if last marker was sos
			try {
				read_sos(*huffman_writer, segment, rst_err_);
			} catch (const std::runtime_error&) {
				throw;
			}
		} else {
			// read in next marker
			if (jpg_input_reader_.read(segment, 2) != 2) {
				throw std::runtime_error("Unable to read segment marker.");
			}
			if (segment[0] != 0xFF) {
				// ugly fix for incorrect marker segment sizes
				throw std::runtime_error("size mismatch in marker segment FF");
			}
		}

		// read segment type
		type = static_cast<Marker>(segment[1]); // TODO: add enum correctness checks?

		// Done parsing the header if the EOI segment is encountered.
		if (type == Marker::kEOI) {
			break;
		}

		// read in next segments' length and check it
		if (jpg_input_reader_.read(segment, 2, 2) != 2) {
			throw std::runtime_error("Unable to read segment length.");
		}
		std::size_t segment_length = 2 + jfif::pack(segment[2], segment[3]); // Length of current marker segment.
		if (segment_length < 4) {
			throw std::runtime_error("Invalid segment length: " + std::to_string(segment_length));
		}

		// realloc segment data if needed
		segment.resize(segment_length);

		// read rest of segment, store back in header writer
		const auto payload_length = segment_length - 4;
		if (jpg_input_reader_.read(segment, payload_length, 4) != payload_length) {
			throw std::runtime_error("Unable to read entire payload of segment.");
		}
		header_writer->write(segment.data(), segment_length);
	}

	// get pointer for header data & size
	segments_ = Segment::parse_segments(header_writer->get_data());
	// get pointer for huffman data & size
	huffman_data_ = huffman_writer->get_data();

	// store garbage after EOI if needed
	std::uint8_t tmp;
	bool garbage_avail = jpg_input_reader_.read_byte(&tmp);
	if (garbage_avail) {

		auto grbgw = std::make_unique<MemoryWriter>();
		grbgw->write_byte(tmp);
		while (true) {
			std::size_t len = jpg_input_reader_.read(segment, segment.capacity());
			if (len == 0) {
				break;
			}
			grbgw->write(segment.data(), len);
		}
		garbage_data_ = grbgw->get_data();
	}

	// parse header for image info
	try {
		frame_info_ = jfif::get_frame_info(segments_);
	} catch (const std::exception&) {
		throw;
	}
}


void JpgReader::read_sos(Writer& huffw, std::vector<std::uint8_t>& segment, std::vector<std::uint8_t>& rst_err) {
	// switch to huffman data reading mode
	int cpos = 0; // rst marker counter
	std::uint32_t crst = 0; // current rst marker counter
	while (true) {
		// read byte from imagedata
		std::uint8_t byte = jpg_input_reader_.read_byte();

		// non-0xFF loop
		if (byte != 0xFF) {
			crst = 0;
			while (byte != 0xFF) {
				huffw.write_byte(byte);
				byte = jpg_input_reader_.read_byte();
			}
		}

		// treatment of 0xFF
		if (byte == 0xFF) {
			byte = jpg_input_reader_.read_byte();
			if (byte == 0x00) {
				crst = 0;
				// no zeroes needed -> ignore 0x00. write 0xFF
				huffw.write_byte(0xFF);
			} else if (byte == 0xD0 + (cpos % 8)) { // restart marker
				// increment rst counters
				cpos++;
				crst++;
			} else { // in all other cases leave it to the header parser routines
				// store number of wrongly set rst markers
				if (crst > 0) {
					if (rst_err.empty()) {
						rst_err.resize(scan_count_ + 1);
					}
				}
				if (!rst_err.empty()) {
					// realloc and set only if needed
					rst_err.resize(scan_count_ + 1);
					if (crst > 255) {
						throw std::runtime_error("Severe false use of RST markers (" + std::to_string(crst) + ")");
						// crst = 255;
					}
					rst_err[scan_count_] = crst;
				}
				// end of current scan
				scan_count_++;
				// on with the header parser routines
				segment[0] = 0xFF;
				segment[1] = byte;
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
