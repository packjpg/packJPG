#ifndef JFIFPARSE_H
#define JFIFPARSE_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

#include "bitops.h"
#include "huffcodes.h"
#include "jpegtype.h"
#include "scaninfo.h"
#include "segment.h"

namespace jfif {
	constexpr int pack(std::uint8_t left, std::uint8_t right) {
		return (int(left) << 8) + int(right);
	}

	// Builds Huffman trees and codes.
	inline void parse_dht(const std::vector<std::uint8_t>& segment, std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2>& hcodes) {
		int hpos = 4; // current position in segment, start after segment header
					  // build huffman trees & codes
		while (hpos < segment.size()) {
			int lval = bitops::LBITS(segment[hpos], 4);
			int rval = bitops::RBITS(segment[hpos], 4);
			if (lval < 0 || lval >= 2 || rval < 0 || rval >= 4) {
				break;
			}

			hpos++;
			// build huffman codes & trees
			hcodes[lval][rval] = std::make_unique<HuffCodes>(&(segment[hpos + 0]), &(segment[hpos + 16]));

			int skip = 16;
			for (int i = 0; i < 16; i++) {
				skip += static_cast<int>(segment[hpos + i]);
			}
			hpos += skip;
		}

		if (hpos != segment.size()) {
			// if we get here, something went wrong
			throw std::range_error("size mismatch in dht marker");
		}
	}

	/*
	 * Reads quantization tables from a dqt segment into the supplied map. Any existing quantization table in the map with the same index
	 * as one of the new quantization tables is overwritten. Throws a runtime_error exception if there is a problem parsing the segment.
	 */
	inline void parse_dqt(std::map<int, std::array<std::uint16_t, 64>>& qtables, const std::vector<std::uint8_t>& segment) {
		std::size_t segment_pos = 4; // current position in segment, start after segment header
		while (segment_pos < segment.size()) {
			const std::uint8_t byte = segment[segment_pos];
			const int precision = bitops::LBITS(byte, 4);
			if (precision < 0 || precision > 1) {
				throw std::runtime_error("Invalid quantization table element precision: " + std::to_string(precision));
			}

			const int index = bitops::RBITS(byte, 4);
			if (index < 0 || index > 3) {
				throw std::runtime_error("Invalid quantization table destination identifier: " + std::to_string(index));
			}
			segment_pos++;

			std::array<std::uint16_t, 64> qtable{};
			if (precision == 0) { // 8-bit quantization table element precision.
				for (std::size_t i = 0; i < qtable.size(); i++) {
					qtable[i] = static_cast<std::uint16_t>(segment[segment_pos + i]);
					if (qtable[i] == 0) {
						throw std::runtime_error("Quantization table contains an element with a zero value.");
					}
				}
				segment_pos += 64;
			}
			else { // 16-bit quantization table element precision.
				for (std::size_t i = 0; i < qtable.size(); i++) {
					qtable[i] = pack(segment[segment_pos + (2 * i)], segment[segment_pos + (2 * i) + 1]);
					if (qtable[i] == 0) {
						throw std::runtime_error("Quantization table contains an element with a zero value.");
					}
				}
				segment_pos += 128;
			}
			qtables[index] = qtable;
		}

		if (segment_pos != segment.size()) {
			// if we get here, something went wrong
			throw std::runtime_error("Invalid length in DQT segment.");
		}
	}

	// Helper function that parses DRI segments.
	inline int parse_dri(const std::vector<std::uint8_t>& segment) {
		int hpos = 4; // current position in segment, start after segment header
		return pack(segment[hpos], segment[hpos + 1]);
	}
}

#endif