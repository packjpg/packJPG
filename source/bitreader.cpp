#include "bitreader.h"

#include "bitops.h"

BitReader::BitReader(const std::vector<std::uint8_t>& bits) :
	data_(bits),
	curr_byte_(std::begin(data_)),
	eof_(bits.empty()) {
}

BitReader::~BitReader() {
}

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
