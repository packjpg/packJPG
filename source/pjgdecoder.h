#ifndef PJGDECODER_H
#define PJGDECODER_H

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "aricoder.h"
#include "component.h"
#include "frameinfo.h"
#include "reader.h"
#include "segment.h"

class PjgDecoder {
public:
	PjgDecoder(Reader& decoding_stream);

	// Decodes image encoded as pjg to colldata.
	void decode();

	std::vector<Segment> get_segments() const;
	std::vector<std::uint8_t> get_garbage_data() const;
	std::unique_ptr<FrameInfo> get_frame_info();
	std::uint8_t get_padbit() const;
	std::vector<std::uint8_t> get_rst_err() const;
private:
	// Decodes frequency (zero-sorted) scan order.
	std::array<std::uint8_t, 64> decode_zero_sorted_scan();

	// Decode zero-distribution-lists (number of nonzeroes) for higher (7x7) ACs.
	std::vector<std::uint8_t> zdst_high(const Component& component);

	// Decode zero-distribution-lists (number of nonzeroes) for lower ACs.
	std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>> zdst_low(const Component& component, const std::vector<std::uint8_t>& zero_dist_context, const std::vector<std::uint8_t>& eob_x, const std::vector<std::uint8_t>& eob_y);

	// Decodes DC coefficients.
	void decode_dc(Component& component, const std::vector<std::uint8_t>& zero_dist_list);

	// Decodes high (7x7) AC coefficients.
	std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>> ac_high(Component& component, const std::array<std::uint8_t, 64>& zero_sorted_scan, std::vector<std::uint8_t>&& zero_dist_list);

	// Decodes first row/col AC coefficients.
	void PjgDecoder::ac_low(Component& component, std::vector<std::uint8_t>& zdstxlow, std::vector<std::uint8_t>& zdstylow);

	// Decodes generic 8-bit data.
	std::vector<std::uint8_t> generic();

	// Decodes one bit.
	std::uint8_t bit();

	int PjgDecoder::decode_residual(BinaryModel& residual_model, int starting_bit, int context, int initial_residual = 1);

	std::unique_ptr<FrameInfo> frame_info_;
	std::vector<Segment> segments_;

	std::uint8_t padbit_ = -1;

	std::vector<std::uint8_t> rst_err_;
	std::vector<std::uint8_t> garbage_data_;

	std::unique_ptr<ArithmeticDecoder> decoder_;
};

#endif