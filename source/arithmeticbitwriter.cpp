#include "arithmeticbitwriter.h"

void ArithmeticBitWriter::write_n_zero_bits(std::size_t n) {
	if (n + curr_bit_ >= 8) {
		auto remainingBits = 8 - curr_bit_;
		n -= remainingBits;
		curr_byte_ <<= remainingBits;
		data_.emplace_back(curr_byte_);
		curr_bit_ = 0;
	}

	while (n >= 8) {
		data_.emplace_back(0);
		n -= 8;
	}

	curr_byte_ <<= n;
	curr_bit_ += n;
}

void ArithmeticBitWriter::write_n_one_bits(std::size_t n) {
	constexpr std::uint8_t all_ones = std::numeric_limits<std::uint8_t>::max();
	if (n + curr_bit_ >= 8) {
		auto remainingBits = 8 - curr_bit_;
		n -= remainingBits;
		curr_byte_ <<= remainingBits;
		curr_byte_ |= all_ones >> (8 - remainingBits);
		data_.emplace_back(curr_byte_);
		curr_bit_ = 0;
	}

	while (n >= 8) {
		data_.emplace_back(all_ones);
		n -= 8;
	}

	curr_byte_ = (curr_byte_ << n) | (all_ones >> (8 - n));
	curr_bit_ += n;
}

void ArithmeticBitWriter::pad() {
	while (curr_bit_ > 0) {
		write_bit<0>();
	}
}

std::vector<std::uint8_t> ArithmeticBitWriter::get_data() const {
	return data_;
}
