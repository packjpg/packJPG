#ifndef JPGREADER_H
#define JPGREADER_H

#include <cstdint>
#include <memory>
#include <vector>

#include "frameinfo.h"
#include "reader.h"
#include "segment.h"

class JpgReader {
public:
	JpgReader(Reader& reader);
	// Read in header and image data.
	void read();

	std::vector<Segment> get_segments();
	std::unique_ptr<FrameInfo> get_frame_info();
	std::vector<std::uint8_t> get_huffman_data();
	std::vector<std::uint8_t> get_garbage_data();
	std::vector<std::uint8_t> get_rst_err();

private:

	void read_sos();
	std::vector<Segment> parse_segments();
	std::vector<std::uint8_t> read_garbage_data();

	Reader& reader_;
	
	int scans_processed_ = 0; // Count of scans.

	std::unique_ptr<FrameInfo> frame_info_;
	std::vector<Segment> segments_;
	std::vector<std::uint8_t> huffman_data_;

	std::vector<std::uint8_t> rst_err_;
	std::vector<std::uint8_t> garbage_data_;
};

#endif