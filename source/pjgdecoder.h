#ifndef PJGDECODER_H
#define PJGDECODER_H

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

#include "aricoder.h"
#include "component.h"
#include "frameinfo.h"
#include "reader.h"
#include "segment.h"
#include "eobdata.h"

class PjgDecoder {
public:
	PjgDecoder(Reader& decoding_stream);

	// Decodes image encoded as pjg to colldata.
	std::tuple<FrameInfo,
		std::vector<Segment>,
		std::vector<Component>,
		std::vector<std::uint8_t>,
		std::vector<std::uint8_t>,
		std::uint8_t> decode();
private:
	// Decodes frequency (zero-sorted) scan order.
	std::array<std::uint8_t, 64> decode_zero_sorted_scan();

	// Decode zero-distribution-lists (number of nonzeroes) for higher (7x7) ACs.
	std::vector<std::uint8_t> decode_zdst_high(const Component& component);

	// Decode zero-distribution-lists (number of nonzeroes) for lower ACs.
	std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>> decode_zdst_low(const Component& component, const std::vector<std::uint8_t>& zero_dist_context, const EOBData& eob_data);

	// Decodes DC coefficients.
	void decode_dc(Component& component, const std::vector<std::uint8_t>& zero_dist_list);

	// Decodes high (7x7) AC coefficients.
	EOBData decode_ac_high(Component& component, const std::array<std::uint8_t, 64>& zero_sorted_scan, const std::vector<std::uint8_t>& zero_dist_list);

	// Decodes first row/col AC coefficients.
	void decode_ac_low(Component& component, std::vector<std::uint8_t>& zdstxlow, std::vector<std::uint8_t>& zdstylow);

	// Decodes generic 8-bit data.
	std::vector<std::uint8_t> decode_generic();

	// Decodes one bit.
	std::uint8_t decode_bit();

	int decode_residual(BinaryModel& residual_model, int starting_bit, int context, int initial_residual = 1);

	std::uint8_t padbit_ = std::numeric_limits<std::uint8_t>::max();

	std::vector<std::uint8_t> rst_err_;
	std::vector<std::uint8_t> garbage_data_;

	std::unique_ptr<ArithmeticDecoder> decoder_;
};

#endif