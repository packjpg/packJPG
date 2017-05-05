#ifndef SEGMENT_H
#define SEGMENT_H
#include <cstdint>
#include <vector>
#include "reader.h"

enum class Marker {
	SOF0 = 0xC0, // Baseline DCT (Huffman coding).
	SOF1 = 0xC1, // Extended sequential DCT (Huffman coding).
	SOF2 = 0xC2, // Progressive DCT (Huffman coding).
	SOF3 = 0xC3, // Lossless (sequential) (Huffman coding).

	DHT = 0xC4, // Define Huffman table(s).

	SOF5 = 0xC5, // Differential sequential DCT (Huffman coding).
	SOF6 = 0xC6, // Differential progressive DCT (Huffman coding).
	SOF7 = 0xC7, // Differential lossless (sequential) (Huffman coding).

	JPG = 0xC8, // Reserved for JPEG extensions.

	SOF9 = 0xC9, // Extended sequential DCT (arithmetic coding).
	SOF10 = 0xCA, // Progressive DCT (arithmetic coding).
	SOF11 = 0xCB, // Lossless (sequential) (arithmetic coding).

	DAC = 0xCC, // Define arithmetic coding conditioning(s).

	SOF13 = 0xCD, // Differential sequential DCT (arithmetic coding).
	SOF14 = 0xCE, // Differential progressive DCT (arithmetic coding).
	SOF15 = 0xCF, // Differential lossless (sequential) (arithmetic coding).

	/*
	Restart RSTn with count n.
	*/

	RST0 = 0xD0,
	RST1 = 0xD1,
	RST2 = 0xD2,
	RST3 = 0xD3,
	RST4 = 0xD4,
	RST5 = 0xD5,
	RST6 = 0xD6,
	RST7 = 0xD7,

	SOI = 0xD8, // Start of image.
	EOI = 0xD9, // End of image.
	SOS = 0xDA, // Start of scan.
	DQT = 0xDB, // Define quantization table(s).
	DNL = 0xDC, // Define number of lines.
	DRI = 0xDD, // Define restart interval.
	DHP = 0xDE, // Define hierarchical progression.
	EXP = 0xDF, // Expand reference component(s).

	/*
	APPn are reserved for application segments:
	*/
	APP0 = 0xE0,
	APP1 = 0xE1,
	APP2 = 0xE2,
	APP3 = 0xE3,
	APP4 = 0xE4,
	APP5 = 0xE5,
	APP6 = 0xE6,
	APP7 = 0xE7,
	APP8 = 0xE8,
	APP9 = 0xE9,
	APP10 = 0xEA,
	APP11 = 0xEB,
	APP12 = 0xEC,
	APP13 = 0xED,
	APP14 = 0xEE,
	APP15 = 0xEF,

	JPG0 = 0xF0, // Reserved for JPEG extensions.
	JPG13 = 0xFD, // Reserved for JPEG extensions.
	COM = 0xFE, // Comment.

	TEM = 0x01, // For temporary private use in arithmetic coding.

	INVALID
};

class Segment {
public:
	/*
	 * Reads the next segment in from the reader. If the next bytes in the reader aren't a segment,
	 * there are insufficient bytes in the reader for the segment, or the segment is otherwise invalid,
	 * a runtime_error exception is thrown.
	 */
	Segment(Reader& reader);

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

	/*
	Returns a copy of the segment's data, including the marker (and length field, where applicable).
	*/
	std::vector<std::uint8_t> get_data() const;

	/*
	 * Optimizes the segment (if it is a DHT or DQT segment) for PJG compression.
	 */
	void optimize();
	/*
	 * Un-optimizes the segment (if it is a DHT or DQT segment), such that the segment has data == unoptimize(optimize(data)).
	 * Running undo_optimize on a non-optimized segment has undefined (but likely not good) effects.
	 */
	void undo_optimize();

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
			if (segment.get_type() == Marker::EOI) {
				break; // Last segment encountered, don't read any more.
			}
			segments.push_back(segment);
			std::advance(header_pos, segment.get_size());
		}

		return segments;
	}

private:
	/*
	Returns whether the given marker type has a length field associated with it.
	*/
	static bool has_length(Marker type);

	void optimize_dqt();
	void optimize_dht();

	// Undoes DHT segment optimizations.
	void undo_dht_optimization();

	// Undoes DQT segment optimizations.
	void undo_dqt_optimization();

	Marker type_ = Marker::INVALID; // The type of the segment.
	std::vector<std::uint8_t> data_; // The bytes in the segment, including the segment type and length fields where applicable.
};

#endif
