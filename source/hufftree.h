#ifndef HUFFTREE_H
#define HUFFTREE_H

#include <array>
#include <cstdint>

#include "bitreader.h"
#include "huffcodes.h"

class HuffTree {
private:
	std::array<std::uint16_t, 256> l{};
	std::array<std::uint16_t, 256> r{};

public:
	// Constructs a Huffman tree from the given Huffman codes.
	HuffTree(const HuffCodes& codes);

	// Returns next the next code (from the tree and the Huffman data).
	std::uint8_t next_huffcode(BitReader& huffr) const;
};

#endif
