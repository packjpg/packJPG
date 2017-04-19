/*
This file contains special classes for bitwise
reading and writing of arrays
*/

#include "bitops.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

#if defined(_WIN32) || defined(WIN32)
#include <fcntl.h>
#include <io.h>
#endif

BitReader::BitReader(const std::vector<std::uint8_t>& bits) :
	data(bits),
	cbyte(std::begin(data)),
	eof_(bits.empty()) {}

BitReader::~BitReader() {}

/* -----------------------------------------------
	reads n bits from abitreader
	----------------------------------------------- */

unsigned int BitReader::read(int nbits) {
	unsigned int retval = 0;

	// safety check for eof
	if (eof()) {
		overread_ = true;
		return 0;
	}

	while (nbits >= cbit) {
		nbits -= cbit;
		retval |= (bitops::RBITS(*cbyte, cbit) << nbits);
		cbit = 8;
		++cbyte;
		if (cbyte == std::end(data)) {
			eof_ = true;
			return retval;
		}
	}

	if (nbits > 0) {
		retval |= (MBITS(*cbyte, cbit, (cbit - nbits)));
		cbit -= nbits;
	}

	return retval;
}

std::uint8_t BitReader::read_bit() {
	// safety check for eof
	if (eof()) {
		overread_ = true;
		return 0;
	}

	// read one bit
	std::uint8_t bit = bitops::BITN(*cbyte, --cbit);
	if (cbit == 0) {
		++cbyte;
		if (cbyte == std::end(data)) {
			eof_ = true;
		}
		cbit = 8;
	}

	return bit;
}

/* -----------------------------------------------
	to skip padding from current byte
	----------------------------------------------- */

std::uint8_t BitReader::unpad(std::uint8_t fillbit) {
	if ((cbit == 8) || eof()) {
		return fillbit;
	} else {
		fillbit = read(1);
		if (cbit < 8) {
			++cbyte;
			cbit = 8;
			eof_ = cbyte == std::end(data);
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

BitWriter::BitWriter(int size) : data(std::max(size, 65536)) {}

BitWriter::~BitWriter() {}

/* -----------------------------------------------
	writes n bits to abitwriter
	----------------------------------------------- */

void BitWriter::write(unsigned int val, int nbits) {
	// test if pointer beyond flush treshold
	if (cbyte > (data.size() - 5)) {
		data.resize(data.size() * 2);
	}

	// write data
	while (nbits >= cbit) {
		data[cbyte] |= (MBITS32(val, nbits, (nbits - cbit)));
		nbits -= cbit;
		cbyte++;
		cbit = 8;
	}

	if (nbits > 0) {
		data[cbyte] |= ((RBITS32(val, nbits)) << (cbit - nbits));
		cbit -= nbits;
	}
}

/* -----------------------------------------------
	writes one bit to abitwriter
	----------------------------------------------- */

void BitWriter::write_bit(std::uint8_t bit) {

	// write data
	if (bit) {
		data[cbyte] |= 0x1 << (--cbit);
	} else {
		--cbit;
	}
	if (cbit == 0) {
		// test if pointer beyond flush treshold
		cbyte++;
		if (cbyte > (data.size() - 5)) {
			data.resize(data.size() * 2);
		}
		cbit = 8;
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
	while (cbit < 8) {
		write(fillbit_, 1);
	}
}

std::vector<std::uint8_t> BitWriter::get_data() {
	pad(); // Pad the last bits of the data before returning it.
	data.resize(cbyte);
	return data;
}

/* -----------------------------------------------
	gets size of data array from abitwriter
	----------------------------------------------- */

int BitWriter::getpos() const {
	return cbyte;
}