#include "bitwriter.h"

#include "bitops.h"

BitWriter::BitWriter(std::uint8_t fillbit) : data_(65536), fillbit_(fillbit) {
}

BitWriter::~BitWriter() {
}

void BitWriter::write_u16(std::uint16_t val, std::size_t num_bits) {
	// Resize if necessary
	if (curr_byte_ > (data_.size() - 5)) {
		data_.resize(data_.size() * 2);
	}

	// write data
	while (num_bits >= curr_bit_) {
		data_[curr_byte_] |= bitops::mbits(val, num_bits, num_bits - curr_bit_);
		num_bits -= curr_bit_;
		curr_byte_++;
		curr_bit_ = 8;
	}

	if (num_bits > 0) {
		data_[curr_byte_] |= bitops::rbits(val, num_bits) << (curr_bit_ - num_bits);
		curr_bit_ -= num_bits;
	}
}

/* -----------------------------------------------
writes one bit to abitwriter
----------------------------------------------- */

void BitWriter::write_bit(std::uint8_t bit) {

	// write data
	if (bit) {
		curr_bit_--;
		data_[curr_byte_] |= 0x1 << curr_bit_;
	} else {
		curr_bit_--;
	}
	if (curr_bit_ == 0) {
		curr_byte_++;
		if (curr_byte_ == data_.size()) {
			data_.resize(data_.size() * 2);
		}
		curr_bit_ = 8;
	}
}

/* -----------------------------------------------
pads data using fillbit
----------------------------------------------- */

void BitWriter::pad() {
	while (curr_bit_ < 8) {
		write_bit(fillbit_);
	}
}

std::vector<std::uint8_t> BitWriter::get_data() {
	pad(); // Pad the last bits of the data before returning it.
	data_.resize(curr_byte_);
	return data_;
}

/* -----------------------------------------------
gets size of data array from abitwriter
----------------------------------------------- */

std::size_t BitWriter::get_bytes_written() const {
	return curr_byte_;
}
