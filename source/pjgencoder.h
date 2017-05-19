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
	void encode_zero_sorted_scan(const std::array<std::uint8_t, 64>& zero_sorted_scan);

	// Encodes zero-distribution-lists (number of non zeroes) for higher ACs.
	void zdst_high(const Component& component, const std::vector<std::uint8_t>& zero_dist_list);

	// Encodes zero-distribution-lists (number of non zeroes) for lower ACs.
	void zdst_low(const Component& component, const std::vector<std::uint8_t>& zero_dist_context, const std::vector<std::uint8_t>& zdstxlow, const std::vector<std::uint8_t>& zdstylow, const std::vector<std::uint8_t>& eob_x, const std::vector<std::uint8_t>& eob_y);

	// Encodes DC coefficients.
	void dc(const Component& component, const std::vector<std::uint8_t>& zero_dist_list);

	// Encodes higher (7x7) AC coefficients.
	std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>> ac_high(Component& component, std::vector<std::uint8_t>& zero_dist_list, const std::array<std::uint8_t, 64>& zero_sorted_scan);

	// Encodes first row/col (lower) AC coefficients.
	void ac_low(Component& component, std::vector<std::uint8_t>& zdstxlow, std::vector<std::uint8_t>& zdstylow);


	void encode_residual(BinaryModel& model, int coeff_abs, int coeff_bitlen, int val_context);

	// Encodes all of the segments as generic 8-bit data
	void generic(const std::vector<Segment>& segments);

	// Encodes a vector of generic (8bit) data.
	void generic(const std::vector<std::uint8_t>& data);

	// Encodes one bit.
	void bit(std::uint8_t bit);

	// Get zero-sorted frequency scan vector.
	std::array<std::uint8_t, 64> get_zero_sorted_scan(const Component& component) const;

	std::unique_ptr<ArithmeticEncoder> encoder_;
};

#endif
