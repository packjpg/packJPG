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

	// Decodes image encoded as pjg to colldata.
	void decode();
private:
	// Undoes DHT segment optimizations.
	void deoptimize_dht(Segment& segment);

	// Undoes DQT segment optimizations.
	void deoptimize_dqt(Segment& segment);

	// Undoes DHT and DQT (header) optimizations.
	void deoptimize_header(std::vector<Segment>& segments);

	// Decodes frequency scan order.
	std::array<std::uint8_t, 64> zstscan();

	// Decodes number of nonzeroes (high).
	void zdst_high(Component& cmpt);

	// Decodes number of nonzeroes (low).
	void zdst_low(Component& cmpt);

	// Decodes DC coefficients.
	void dc(Component& cmpt);

	// Decodes high (7x7) AC coefficients.
	void ac_high(Component& cmpt);

	// Decodes first row/col AC coefficients.
	void ac_low(Component& cmpt);

	// Decodes generic 8-bit data.
	std::vector<std::uint8_t> generic();

	// Decodes one bit.
	std::uint8_t bit();

	std::unique_ptr<ArithmeticDecoder> decoder_;
	PjgContext context_;
};

#endif