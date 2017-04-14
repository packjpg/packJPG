#ifndef PJGDECODER_H
#define PJGDECODER_H

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "aricoder.h"
#include "component.h"
#include "pjgcontext.h"
#include "segment.h"

class PjgDecoder {
public:
	void decode();
private:
	// Undoes DHT segment optimizations.
	void deoptimize_dht(Segment& segment);
	// Undoes DQT segment optimizations.
	void deoptimize_dqt(Segment& segment);
	// Undoes DHT and DQT (header) optimizations.
	void deoptimize_header();

	std::array<std::uint8_t, 64> zstscan(const std::unique_ptr<ArithmeticDecoder>& dec);
	void zdst_high(const std::unique_ptr<ArithmeticDecoder>& dec, Component& cmpt);
	void zdst_low(const std::unique_ptr<ArithmeticDecoder>& dec, Component& cmpt);
	void dc(const std::unique_ptr<ArithmeticDecoder>& dec, Component& cmpt);
	void ac_high(const std::unique_ptr<ArithmeticDecoder>& dec, Component& cmpt);
	void ac_low(const std::unique_ptr<ArithmeticDecoder>& dec, Component& cmpt);
	std::vector<std::uint8_t> generic(const std::unique_ptr<ArithmeticDecoder>& dec);
	std::uint8_t bit(const std::unique_ptr<ArithmeticDecoder>& dec);

	PjgContext context;
};

#endif