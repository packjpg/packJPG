#ifndef BITOPS_H
#define BITOPS_H

#include <cstdint>
#include <limits>

namespace bitops {

/*
* If val is below min, returns min, if val is above max, max is returned, otherwise val is returned.
*/
constexpr int clamp(int val, int min, int max) {
	return (val < min) ? min : (val > max ? max : val);
}

constexpr std::uint16_t pack(std::uint8_t left, std::uint8_t right) {
	return std::uint16_t(std::uint16_t(left) << 8) + std::uint16_t(right);
}

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

#endif