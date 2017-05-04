#ifndef BITWRITER_H
#define BITWRITER_H

#include <cstdint>
#include <vector>

class BitWriter {
public:
	BitWriter(std::uint8_t padbit);
	~BitWriter();
	void write_u16(std::uint16_t val, std::size_t num_bits);
	void write_bit(std::uint8_t bit);
	void pad();
	std::vector<std::uint8_t> get_data();
	std::size_t getpos() const;

private:

	std::uint8_t fillbit_;
	std::vector<std::uint8_t> data_;
	std::size_t curr_byte_ = 0; // The position in the data of the byte being written.
	std::size_t curr_bit_ = 8; // The position of the next bit in the current byte.
};

#endif