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
	PjgEncoder(Writer& encoding_output);

	// Encodes image data as pjg.
	void encode(std::uint8_t padbit, std::vector<Component>& cmpts, std::vector<Segment>& segments, const std::vector<std::uint8_t>& rst_err, const std::vector<std::uint8_t>& grbgdata);
private:
	// Encodes frequency scanorder.
	std::array<std::uint8_t, 64> zstscan(const Component& cmp);

	// Encodes # of non zeroes (high).
	void zdst_high(const Component& cmpt);

	// Encodes # of non zeroes (low).
	void zdst_low(const Component& cmpt);

	// Encodes DC coefficients.
	void dc(const Component& cmpt);

	// Encodes high (7x7) AC coefficients.
	void ac_high(Component& cmpt);

	// Encodes first row/col AC coefficients.
	void ac_low(Component& cmpt);

	// Encodes all of the segments as generic 8-bit data
	void generic(const std::vector<Segment>& segments);

	// Encodes a vector of generic (8bit) data.
	void generic(const std::vector<std::uint8_t>& data);

	// Encodes one bit.
	void bit(std::uint8_t bit);

	// Get zero-sorted frequency scan vector.
	std::array<std::uint8_t, 64> get_zerosort_scan(const Component& cmpt);

	std::unique_ptr<ArithmeticEncoder> encoder_;
	PjgContext context_;
};

#endif
