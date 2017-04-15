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
	PjgDecoder(const std::unique_ptr<iostream>& decoding_stream);
	void decode();
private:
	// Undoes DHT segment optimizations.
	void deoptimize_dht(Segment& segment);
	// Undoes DQT segment optimizations.
	void deoptimize_dqt(Segment& segment);
	// Undoes DHT and DQT (header) optimizations.
	void deoptimize_header(std::vector<Segment>& segments);

	std::array<std::uint8_t, 64> zstscan();
	void zdst_high(Component& cmpt);
	void zdst_low(Component& cmpt);
	void dc(Component& cmpt);
	void ac_high(Component& cmpt);
	void ac_low(Component& cmpt);
	std::vector<std::uint8_t> generic();
	std::uint8_t bit();

	std::unique_ptr<ArithmeticDecoder> decoder_;
	PjgContext context_;
};

#endif