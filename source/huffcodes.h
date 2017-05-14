#ifndef HUFFCODES_H
#define HUFFCODES_H

#include <array>
#include <cstdint>
#include <vector>

class HuffCodes {
public:
	std::array<std::uint16_t, 256> cval{};
	std::array<std::uint16_t, 256> clen{};
	std::uint16_t max_eobrun = 0;
	
	HuffCodes(const std::vector<std::uint8_t>& counts, const std::vector<std::uint8_t>& values);
};

#endif
