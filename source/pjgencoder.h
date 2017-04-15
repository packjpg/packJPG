#ifndef PJGENCODER_H
#define PJGENCODER_H

#include <array>
#include <memory>

#include "aricoder.h"
#include "component.h"
#include "pjgcontext.h"
#include "segment.h"

class PjgEncoder {
public:
	PjgEncoder(const std::unique_ptr<iostream>& encoding_output);
	void encode();
private:
	// Optimizes DHT segments for compression.
	void optimize_dht(Segment& segment);
	// Optimizes DQT segments for compression.
	void optimize_dqt(Segment& segment);
	// Optimizes JFIF header for compression.
	void optimize_header(std::vector<Segment>& segments);

	std::array<std::uint8_t, 64> zstscan(const Component& cmp);
	void zdst_high(const Component& cmpt);
	void zdst_low(const Component& cmpt);
	void dc(const Component& cmpt);
	void ac_high(Component& cmpt);
	void ac_low(Component& cmpt);
	void generic(const std::vector<Segment>& segments);
	void generic(const std::vector<std::uint8_t>& data);
	void bit(std::uint8_t bit);

	// Get zero-sorted frequency scan vector.
	std::array<std::uint8_t, 64> get_zerosort_scan(const Component& cmpt);

	std::unique_ptr<ArithmeticEncoder> encoder_;
	PjgContext context_;
};

#endif
