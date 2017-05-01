#ifndef BITOPS_H
#define BITOPS_H

#include <cstdint>
#include <vector>

namespace bitops {

/*
 * Where m is the number of bits in type T, applies an AND mask for the lowest m - n bits to val (where n is defined on 0 <= n <= m) (i.e. returns the n rightmost bits).
 */
template <class T>
constexpr T rbits(T val, std::size_t n) {
	return val & (std::numeric_limits<T>::max() >> (sizeof val * 8 - n));
}

/*
* Where m is the number of bits in type T and n is 0 <= n <= m, returns val right-shifted by m - n bits (i.e. the n leftmost bits).
*/
template <class T>
constexpr T lbits(T val, std::size_t n) {
	return val >> (sizeof val * 8 - n);
}

/*
 * Equivalent to rbits(val, l) >> r. (in essence, returns the bits between l and r indices, right-shifted
 * to the zero bit position). Assumes 0 <= l <= r <= sizeof val * 8.
 */
template <class T>
constexpr T mbits(T val, std::size_t l, std::size_t r) {
	return bitops::rbits(val, l) >> r;
}

constexpr std::uint8_t left_nibble(std::uint8_t byte) {
	return lbits(byte, 4);
}

constexpr std::uint8_t right_nibble(std::uint8_t byte) {
	return rbits(byte, 4);
}

/*
 * Returns the nth bit in val, either 0 or 1.
 */
template <class T>
constexpr std::uint8_t bitn(T val, std::size_t n) {
	return (val >> n) & 0x1;
}
}

/* -----------------------------------------------
	class to read arrays bitwise
	----------------------------------------------- */
class BitReader {
public:
	BitReader(const std::vector<std::uint8_t>& bytes);
	~BitReader();
	std::uint16_t read_u16(std::size_t num_bits);
	std::uint8_t read_bit();
	std::uint8_t unpad(std::uint8_t fillbit);
	bool eof() const;
	bool overread() const;

private:
	const std::vector<std::uint8_t> data_;
	std::vector<std::uint8_t>::const_iterator curr_byte_; // The position in the data of the byte being read.
	std::size_t curr_bit_ = 8; // The position of the next bit in the current byte.
	bool overread_ = false; // Tried to read more bits than available in the reader.
	bool eof_ = false; // Read all the bits in the reader.
};


/* -----------------------------------------------
	class to write arrays bitwise
	----------------------------------------------- */
class BitWriter {
public:
	BitWriter(std::size_t size);
	~BitWriter();
	void write_u16(std::uint16_t val, std::size_t num_bits);
	void write_bit(std::uint8_t bit);
	void set_fillbit(std::uint8_t fillbit);
	void pad();
	std::vector<std::uint8_t> get_data();
	std::size_t getpos() const;

private:

	std::uint8_t fillbit_ = 1;
	std::vector<std::uint8_t> data_;
	std::size_t curr_byte_ = 0; // The position in the data of the byte being written.
	std::size_t curr_bit_ = 8; // The position of the next bit in the current byte.
};

#endif