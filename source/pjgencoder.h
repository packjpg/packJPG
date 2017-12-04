#ifndef PJGENCODER_H
#define PJGENCODER_H

#include <array>
#include <cstdint>
#include <memory>

#include "aricoder.h"
#include "component.h"
#include "eobdata.h"
#include "segment.h"
#include "zerodistribution.h"

class PjgEncoder {
public:
	PjgEncoder(Writer& encoding_output);

	// Encodes image data as pjg.
	void encode(std::uint8_t padbit, std::vector<Component>& components, std::vector<Segment>& segments, const std::vector<std::uint8_t>& rst_err, const std::vector<std::uint8_t>& grbgdata);
private:
	// Encodes frequency scanorder.
	void encode_zero_sorted_scan(const std::array<std::uint8_t, 64>& zero_sorted_scan);

	// Encodes zero-distribution-lists (number of non zeroes) for higher ACs.
	void encode_zdst_high(const Component& component, const ZeroDistribution& zero_data);

	// Encodes zero-distribution-lists (number of non zeroes) for lower ACs.
	void encode_zdst_low(const ZeroDistribution& zero_data, const EOBData& eob_data);

	// Encodes DC coefficients.
	void encode_dc(const Component& component, const ZeroDistribution& zero_data);

	// Encodes higher (7x7) AC coefficients, returning eob data.
	EOBData encode_ac_high(const Component& component, const ZeroDistribution& zero_data, const std::array<std::uint8_t, 64>& zero_sorted_scan);

	// Encodes first row/col (lower) AC coefficients.
	void encode_ac_low(const Component& component, ZeroDistribution& zero_data);

	void encode_residual(BinaryModel& model, int coeff_abs, int coeff_bitlen, int val_context);

	// Encodes all of the segments as generic 8-bit data
	void encode_generic(const std::vector<Segment>& segments);

	// Encodes a vector of generic (8bit) data.
	void encode_generic(const std::vector<std::uint8_t>& data);

	// Encodes one bit.
	void encode_bit(std::uint8_t bit);

	// Get zero-sorted frequency scan vector.
	std::array<std::uint8_t, 64> get_zero_sorted_scan(const Component& component) const;

	std::unique_ptr<ArithmeticEncoder> encoder_;
};

#endif
