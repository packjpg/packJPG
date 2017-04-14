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
	void encode();
private:
	// Optimizes DHT segments for compression.
	void optimize_dht(Segment& segment);
	// Optimizes DQT segments for compression.
	void optimize_dqt(Segment& segment);
	// Optimizes JFIF header for compression.
	void optimize_header(std::vector<Segment>& segments);

	std::array<std::uint8_t, 64> zstscan(const std::unique_ptr<ArithmeticEncoder>& enc, const Component& cmp);
	void zdst_high(const std::unique_ptr<ArithmeticEncoder>& enc, const Component& cmpt);
	void zdst_low(const std::unique_ptr<ArithmeticEncoder>& enc, const Component& cmpt);
	void dc(const std::unique_ptr<ArithmeticEncoder>& enc, const Component& cmpt);
	void ac_high(const std::unique_ptr<ArithmeticEncoder>& enc, Component& cmpt);
	void ac_low(const std::unique_ptr<ArithmeticEncoder>& enc, Component& cmpt);
	void generic(const std::unique_ptr<ArithmeticEncoder>& enc, const std::vector<Segment>& segments);
	void generic(const std::unique_ptr<ArithmeticEncoder>& enc, const std::vector<std::uint8_t>& data);
	void bit(const std::unique_ptr<ArithmeticEncoder>& enc, std::uint8_t bit);

	// Get zero-sorted frequency scan vector.
	std::array<std::uint8_t, 64> get_zerosort_scan(const Component& cmpt);

	PjgContext context;
};

#endif
