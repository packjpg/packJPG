#ifndef JPGREADER_H
#define JPGREADER_H

#include <cstdint>
#include <vector>

#include "reader.h"
#include "segment.h"

class JpgReader {
public:
	JpgReader(Reader& reader);
	// Read in header and image data.
	std::tuple<std::vector<Segment>,
		std::vector<std::uint8_t>,
		std::vector<std::uint8_t>,
		std::vector<std::uint8_t>> read();

private:

	// Reads the compressed image data that follows an SOS segment payload.
	void read_sos();

	// Returns the JPG segments (with the compressed SOS data separate in huffman_data_), sans SOI and EOI segments.
	std::vector<Segment> parse_segments();

	// Returns the bytes that follow the EOI segment.
	std::vector<std::uint8_t> read_garbage_data();

	Reader& reader_;
	
	int scans_processed_ = 0;

	std::vector<std::uint8_t> huffman_data_;
	std::vector<std::uint8_t> rst_err_;
};

#endif