#ifndef EOBDATA_H
#define EOBDATA_H

#include <cstdint>
#include <vector>

class EOBData {
public:
	std::vector<std::uint8_t> eob_x;
	std::vector<std::uint8_t> eob_y;
};

#endif