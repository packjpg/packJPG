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

	std::vector<Segment> get_segments();
	std::vector<std::uint8_t> get_garbage_data();
	std::unique_ptr<FrameInfo> get_frame_info();
	std::uint8_t get_padbit();
	std::vector<std::uint8_t> get_rst_err();
private:
	// Decodes frequency scan order.
	std::array<std::uint8_t, 64> decode_zero_sorted_scan();

	// Decodes number of nonzeroes (high).
	void zdst_high(Component& component);

	// Decodes number of nonzeroes (low).
	void zdst_low(Component& component);

	// Decodes DC coefficients.
	void dc(Component& component);

	// Decodes high (7x7) AC coefficients.
	void ac_high(Component& component);

	// Decodes first row/col AC coefficients.
	void ac_low(Component& component);

	// Decodes generic 8-bit data.
	std::vector<std::uint8_t> generic();

	// Decodes one bit.
	std::uint8_t bit();

	std::unique_ptr<FrameInfo> frame_info_;
	std::vector<Segment> segments_;

	std::uint8_t padbit_ = -1;

	std::vector<std::uint8_t> rst_err_;
	std::vector<std::uint8_t> garbage_data_;

	std::unique_ptr<ArithmeticDecoder> decoder_;
};

#endif