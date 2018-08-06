#ifndef BITREADER_H
#define BITREADER_H

#include <cstdint>
#include <vector>

class BitReader {
public:
	BitReader(const std::vector<std::uint8_t>& bytes);
	~BitReader() = default;
	std::uint16_t read_u16(std::size_t num_bits);
	std::uint8_t read_bit();
	void rewind_bits(std::size_t num_bits);
	std::uint8_t unpad(std::uint8_t fillbit);
	bool eof() const;
	bool overread() const;

private:
	const std::vector<std::uint8_t> data_;
	std::vector<std::uint8_t>::const_iterator curr_byte_; // The position in the data of the byte being read.
	std::size_t curr_bit_ = 8; // The position of the next bit in the current byte.
	bool overread_ = false; // Tried to read more bits than available in the reader.
};

#endif