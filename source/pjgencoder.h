#ifndef PJGENCODER_H
#define PJGENCODER_H

#include <array>
#include <cstdint>
#include <memory>

#include "aricoder.h"
#include "component.h"
#include "segment.h"

class PjgEncoder {
public:
	PjgEncoder(Writer& encoding_output);

	// Encodes image data as pjg.
	void encode(std::uint8_t padbit, std::vector<Component>& components, std::vector<Segment>& segments, const std::vector<std::uint8_t>& rst_err, const std::vector<std::uint8_t>& grbgdata);
private:
	// Encodes frequency scanorder.
	void zstscan(const std::array<std::uint8_t, 64>& zero_sorted_scan);

	// Encodes zero-distribution-lists (number of non zeroes) for higher ACs.
	void zdst_high(const Component& component);

	// Encodes zero-distribution-lists (number of non zeroes) for lower ACs.
	void zdst_low(const Component& component);

	// Encodes DC coefficients.
	void dc(const Component& component);

	// Encodes higher (7x7) AC coefficients.
	void ac_high(Component& component);

	// Encodes first row/col (lower) AC coefficients.
	void ac_low(Component& component);


	void encode_residual(BinaryModel& model, int coeff_abs, int coeff_bitlen, int val_context);

	// Encodes all of the segments as generic 8-bit data
	void generic(const std::vector<Segment>& segments);

	// Encodes a vector of generic (8bit) data.
	void generic(const std::vector<std::uint8_t>& data);

	// Encodes one bit.
	void bit(std::uint8_t bit);

	// Get zero-sorted frequency scan vector.
	std::array<std::uint8_t, 64> get_zerosort_scan(const Component& component) const;

	std::unique_ptr<ArithmeticEncoder> encoder_;
};

#endif
