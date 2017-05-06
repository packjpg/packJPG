#ifndef ARITHMETICBITWRITER_H
#define ARITHMETICBITWRITER_H

#include <cstdint>
#include <vector>

class ArithmeticBitWriter {
public:
	template <std::uint8_t bit>
	void write_bit() {
		// add bit at last position
		curr_byte_ = (curr_byte_ << 1) | bit;
		// increment bit position
		curr_bit_++;

		// write bit if done
		if (curr_bit_ == 8) {
			data_.emplace_back(curr_byte_);
			curr_bit_ = 0;
		}
	}

	void write_n_zero_bits(std::size_t n) {
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

	void write_n_one_bits(std::size_t n) {
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

	void pad() {
		while (curr_bit_ > 0) {
			write_bit<0>();
		}
	}

	std::vector<std::uint8_t> get_data() const {
		return data_;
	}


private:
	std::vector<std::uint8_t> data_;
	std::uint8_t curr_byte_ = 0;
	std::size_t curr_bit_ = 0;
};

#endif
