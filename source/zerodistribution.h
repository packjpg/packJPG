#ifndef ZERODISTRIBUTION_H
#define ZERODISTRIBUTION_H

#include <cstdint>
#include <vector>

class ZeroDistribution {
public:
	std::vector<std::uint8_t> zero_dist_list;
	std::vector<std::uint8_t> zdstxlow;
	std::vector<std::uint8_t> zdstylow;

	ZeroDistribution(std::size_t size_zdist) :
		zero_dist_list(size_zdist),
		zdstxlow(size_zdist),
		zdstylow(size_zdist) {}
};

#endif