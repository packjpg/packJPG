#ifndef SEGMENT_H
#define SEGMENT_H
#include <cstdint>
#include <vector>

enum class Marker {
	kSOF0 = 0xC0, // Baseline DCT (Huffman coding).
	kSOF1 = 0xC1, // Extended sequential DCT (Huffman coding).
	kSOF2 = 0xC2, // Progressive DCT (Huffman coding).
	kSOF3 = 0xC3, // Lossless (sequential) (Huffman coding).

	kDHT = 0xC4, // Define Huffman table(s).

	kSOF5 = 0xC5, // Differential sequential DCT (Huffman coding).
	kSOF6 = 0xC6, // Differential progressive DCT (Huffman coding).
	kSOF7 = 0xC7, // Differential lossless (sequential) (Huffman coding).

	kJPG = 0xC8, // Reserved for JPEG extensions.

	kSOF9 = 0xC9, // Extended sequential DCT (arithmetic coding).
	kSOF10 = 0xCA, // Progressive DCT (arithmetic coding).
	kSOF11 = 0xCB, // Lossless (sequential) (arithmetic coding).

	kDAC = 0xCC, // Define arithmetic coding conditioning(s).

	kSOF13 = 0xCD, // Differential sequential DCT (arithmetic coding).
	kSOF14 = 0xCE, // Differential progressive DCT (arithmetic coding).
	kSOF15 = 0xCF, // Differential lossless (sequential) (arithmetic coding).

	/*
	Restart RSTn with count n.
	*/

	kRST0 = 0xD0,
	kRST1 = 0xD1,
	kRST2 = 0xD2,
	kRST3 = 0xD3,
	kRST4 = 0xD4,
	kRST5 = 0xD5,
	kRST6 = 0xD6,
	kRST7 = 0xD7,

	kSOI = 0xD8, // Start of image.
	kEOI = 0xD9, // End of image.
	kSOS = 0xDA, // Start of scan.
	kDQT = 0xDB, // Define quantization table(s).
	kDNL = 0xDC, // Define number of lines.
	kDRI = 0xDD, // Define restart interval.
	kDHP = 0xDE, // Define hierarchical progression.
	kEXP = 0xDF, // Expand reference component(s).

	/*
	APPn are reserved for application segments:
	*/
	kAPP0 = 0xE0,
	kAPP1 = 0xE1,
	kAPP2 = 0xE2,
	kAPP3 = 0xE3,
	kAPP4 = 0xE4,
	kAPP5 = 0xE5,
	kAPP6 = 0xE6,
	kAPP7 = 0xE7,
	kAPP8 = 0xE8,
	kAPP9 = 0xE9,
	kAPP10 = 0xEA,
	kAPP11 = 0xEB,
	kAPP12 = 0xEC,
	kAPP13 = 0xED,
	kAPP14 = 0xEE,
	kAPP15 = 0xEF,

	kJPG0 = 0xF0, // Reserved for JPEG extensions.
	kJPG13 = 0xFD, // Reserved for JPEG extensions.
	kCOM = 0xFE, // Comment.

	kTEM = 0x01, // For temporary private use in arithmetic coding.

	kINVALID
};

class Segment {
public:
	/* Reads the information for the segment in the provided header data that starts at
	the given index.
	*/
	Segment(const std::vector<std::uint8_t>& header, std::size_t offset);
	/* Returns the jpeg marker of the segment. Returns an INVALID  type if the segment is invalid.
	*/
	Marker get_type() const;
	/* Returns the number of bytes in the segment, defined as 2 plus the payload size.
	Returns -1 if the segment is invalid.
	*/
	std::size_t get_size() const;
	/* Returns the index of the start of the segment data in the header data.
	The start of the segment is the index of the 0xFF that precedes the segment marker type.
	Returns -1 if the segment is invalid.
	*/
	std::size_t get_header_offset() const;

	/*
	Returns a copy of the segment's data, including the marker (and length field, where applicable).
	*/
	std::vector<std::uint8_t> get_data() const;

	/*
	Attempts to set the segment's data as the given input. This will fail if the size
	of the input vector is different than the current size of the segment or if the marker
	(i.e. the first two bytes) is different.
	*/
	void set_data(std::vector<std::uint8_t>& data);

	/*
	Returns an in-order vector of the segments contained in the header data,
	starting at the given offset in the data.
	*/
	static std::vector<Segment> parse_segments(const std::vector<std::uint8_t>& header_data, std::size_t offset = 0) {
		if (offset > header_data.size()) {
			return std::vector<Segment>(); // Don't try to read in invalid memory.
		}

		std::vector<Segment> segments; // The segments in the header.
		auto header_pos = std::next(std::begin(header_data), offset); // Position in the header.

		// Parse the segments:
		while (std::distance(header_pos, std::end(header_data)) > 0) {
			Segment segment(header_data, std::distance(std::begin(header_data), header_pos));
			segments.push_back(segment);
			std::advance(header_pos, segment.get_size());
			if (segment.get_type() == Marker::kEOI) {
				break; // Last segment encountered, don't read any more.
			}
		}

		return segments;
	}

private:
	/*
	Returns whether the given marker type has a length field associated with it.
	*/
	static bool has_length(Marker type);

	Marker type_ = Marker::kINVALID; // The type of the segment.
	std::size_t header_pos_ = 0; // The index of the header data where the segment starts.
	std::vector<std::uint8_t> data_; // The bytes in the segment, including the segment type and length fields where applicable.
};

#endif
