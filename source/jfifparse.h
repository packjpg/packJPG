#ifndef JFIFPARSE_H
#define JFIFPARSE_H

#include <cstdint>
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

	// Helper function that parses DQT segments.
	inline void parse_dqt(std::array<std::array<std::uint16_t, 64>, 4>& qtables, const std::vector<std::uint8_t>& segment) {
		int hpos = 4; // current position in segment, start after segment header
		while (hpos < segment.size()) {
			int lval = bitops::LBITS(segment[hpos], 4);
			int rval = bitops::RBITS(segment[hpos], 4);
			if (lval < 0 || lval >= 2) {
				break;
			}
			if (rval < 0 || rval >= 4) {
				break;
			}
			hpos++;
			if (lval == 0) { // 8 bit precision
				for (int i = 0; i < 64; i++) {
					qtables[rval][i] = static_cast<std::uint16_t>(segment[hpos + i]);
					if (qtables[rval][i] == 0) {
						break;
					}
				}
				hpos += 64;
			}
			else { // 16 bit precision
				for (int i = 0; i < 64; i++) {
					qtables[rval][i] =
						pack(segment[hpos + (2 * i)], segment[hpos + (2 * i) + 1]);
					if (qtables[rval][i] == 0) {
						break;
					}
				}
				hpos += 128;
			}
		}

		if (hpos != segment.size()) {
			// if we get here, something went wrong
			throw std::runtime_error("size mismatch in dqt marker");
		}
	}

	// Helper function that parses DRI segments.
	inline int parse_dri(const std::vector<std::uint8_t>& segment) {
		int hpos = 4; // current position in segment, start after segment header
		return pack(segment[hpos], segment[hpos + 1]);
	}
}

#endif