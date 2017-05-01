#ifndef HUFFTREE_H
#define HUFFTREE_H

#include <array>
#include <cstdint>

#include "bitops.h"
#include "bitreader.h"
#include "huffcodes.h"

class HuffTree {
private:
	std::array<std::uint16_t, 256> l{};
	std::array<std::uint16_t, 256> r{};

public:
	// Constructs a Huffman tree from the given Huffman codes.
	HuffTree(const HuffCodes& codes) {
		// initial value for next free place
		int nextfree = 1;

		// work through every code creating links between the nodes (represented through ints)
		for (int i = 0; i < 256; i++) {
			// (re)set current node
			int node = 0;
			// go through each code & store path
			for (int j = codes.clen[i] - 1; j > 0; j--) {
				if (bitops::bitn(codes.cval[i], j) == 1) {
					if (r[node] == 0) {
						r[node] = nextfree++;
					}
					node = r[node];
				} else {
					if (l[node] == 0) {
						l[node] = nextfree++;
					}
					node = l[node];
				}
			}
			// last link is number of targetvalue + 256
			if (codes.clen[i] > 0) {
				if (bitops::bitn(codes.cval[i], 0) == 1) {
					r[node] = i + 256;
				} else {
					l[node] = i + 256;
				}
			}
		}
	}

	// Returns next the next code (from the tree and the Huffman data).
	std::uint8_t next_huffcode(BitReader& huffr) const {
		std::uint16_t node = 0;

		while (node < 256) {
			node = (huffr.read_bit() == 1) ?
				       r[node] : l[node];
			if (node == 0) {
				throw std::runtime_error("Invalid Huffman code.");
			}
		}

		return std::uint8_t(node - 256);
	}
};

#endif
