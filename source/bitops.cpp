/*
This file contains special classes for bitwise
reading and writing of arrays
*/

#include "bitops.h"

#include <algorithm>
#include <array>
#include <vector>

BitReader::BitReader(const std::vector<std::uint8_t>& bits) :
	data_(bits),
	curr_byte_(std::begin(data_)),
	eof_(bits.empty()) {}

BitReader::~BitReader() {}

std::uint16_t BitReader::read_u16(std::size_t num_bits) {
	// safety check for eof
	if (eof()) {
		overread_ = true;
		return 0;
	} else if (num_bits > 16) {
		return 0; // throw an exception?
	}

	std::uint16_t val = 0;
	while (num_bits >= curr_bit_) {
		num_bits -= curr_bit_;
		val |= (bitops::rbits(*curr_byte_, curr_bit_) << num_bits);
		curr_bit_ = 8;
		++curr_byte_;
		if (curr_byte_ == std::end(data_)) {
			eof_ = true;
			return val;
		}
	}

	if (num_bits > 0) {
		val |= (bitops::mbits(*curr_byte_, curr_bit_, (curr_bit_ - num_bits)));
		curr_bit_ -= num_bits;
	}

	return val;
}

std::uint8_t BitReader::read_bit() {
	// safety check for eof
	if (eof()) {
		overread_ = true;
		return 0;
	}

	// read one bit
	std::uint8_t bit = bitops::bitn(*curr_byte_, --curr_bit_);
	if (curr_bit_ == 0) {
		++curr_byte_;
		if (curr_byte_ == std::end(data_)) {
			eof_ = true;
		}
		curr_bit_ = 8;
	}

	return bit;
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
			eof_ = curr_byte_ == std::end(data_);
		}
	}

	return fillbit;
}

bool BitReader::eof() const {
	return eof_;
}

bool BitReader::overread() const {
	return overread_;
}

BitWriter::BitWriter(std::size_t size) : data_(std::max(size, std::size_t(65536))) {}

BitWriter::~BitWriter() {}

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
	Sets the fillbit for padding data.
   ----------------------------------------------- */
void BitWriter::set_fillbit(std::uint8_t fillbit) {
	fillbit_ = fillbit;
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

std::size_t BitWriter::getpos() const {
	return curr_byte_;
}