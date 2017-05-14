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

	void write_n_zero_bits(std::size_t n);

	void write_n_one_bits(std::size_t n);

	void pad();

	std::vector<std::uint8_t> get_data() const;


private:
	std::vector<std::uint8_t> data_;
	std::uint8_t curr_byte_ = 0;
	std::size_t curr_bit_ = 0;
};

#endif
