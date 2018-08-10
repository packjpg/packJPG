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
	std::tuple<std::vector<Segment>, std::vector<std::uint8_t>> read();

	std::vector<Segment> get_segments();
	std::vector<std::uint8_t> get_huffman_data();
	std::vector<std::uint8_t> get_garbage_data();
	std::vector<std::uint8_t> get_rst_err();

private:

	// Reads the compressed image data that follows an SOS segment payload.
	void read_sos();

	// Returns the JPG segments (with the compressed SOS data separate in huffman_data_), sans SOI and EOI segments.
	std::vector<Segment> parse_segments();

	// Returns the bytes that follow the EOI segment.
	std::vector<std::uint8_t> read_garbage_data();

	Reader& reader_;
	
	int scans_processed_ = 0;

	std::vector<Segment> segments_;
	std::vector<std::uint8_t> huffman_data_;

	std::vector<std::uint8_t> rst_err_;
	std::vector<std::uint8_t> garbage_data_;
};

#endif