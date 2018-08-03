#include "bitreader.h"

#include "bitops.h"

#include <algorithm>
#include <string>
#include <stdexcept>

BitReader::BitReader(const std::vector<std::uint8_t>& bits) :
	data_(bits),
	curr_byte_(std::begin(data_)) {
}

BitReader::~BitReader() {}

std::uint16_t BitReader::read_u16(std::size_t num_bits) {
	if (num_bits > 16) {
		throw std::runtime_error("Tried to read " + std::to_string(num_bits) + ", when at most 16 bits can be read with read_u16.");
	}

	std::uint16_t val = 0;
	while (num_bits >= curr_bit_ && !eof()) {
		num_bits -= curr_bit_;
		val |= (bitops::rbits(*curr_byte_, curr_bit_) << num_bits);
		curr_bit_ = 8;
		++curr_byte_;
	}

	if (num_bits > 0 && !eof()) {
		val |= (bitops::mbits(*curr_byte_, curr_bit_, (curr_bit_ - num_bits)));
		curr_bit_ -= num_bits;
		num_bits = 0;
	}

	if (num_bits != 0) {
		overread_ = true;
	}

	return val;
}

std::uint8_t BitReader::read_bit() {
	if (eof()) {
		overread_ = true;
		return 0;
	}

	// read one bit
	std::uint8_t bit = bitops::bitn(*curr_byte_, --curr_bit_);
	if (curr_bit_ == 0) {
		++curr_byte_;
		curr_bit_ = 8;
	}

	return bit;
}

void BitReader::rewind_bits(std::size_t num_bits) {
	if (eof()) {
		overread_ = false;
	} else if (num_bits <= 8 - curr_bit_) {
		curr_bit_ += num_bits;
		return;
	}
	curr_bit_ += num_bits;
	const auto num_bytes_rewound = curr_bit_ / 8;
	auto num_to_rewind = std::min(num_bytes_rewound, std::size_t(std::distance(std::begin(data_), curr_byte_)));
	auto new_pos = std::distance(std::begin(data_), curr_byte_) - num_to_rewind;
	curr_byte_ = std::next(std::begin(data_), new_pos);
	curr_bit_ %= 8;
	if (curr_bit_ == 0) {
		++curr_byte_;
		curr_bit_ = 8;
	}
}

/* -----------------------------------------------
to skip padding from current byte
----------------------------------------------- */

std::uint8_t BitReader::unpad(std::uint8_t fillbit) {
	if ((curr_bit_ == 8) || eof()) {
		return fillbit;
	} else {
		fillbit = read_bit();
		if (curr_bit_ < 8) {
			++curr_byte_;
			curr_bit_ = 8;
		}
	}

	return fillbit;
}

bool BitReader::eof() const {
	return curr_byte_ == std::end(data_);
}

bool BitReader::overread() const {
	return overread_;
}
