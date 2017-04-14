#ifndef HUFFCODES_H
#define HUFFCODES_H

#include <array>
#include <cstdint>

struct HuffCodes {
	std::array<std::uint16_t, 256> cval{};
	std::array<std::uint16_t, 256> clen{};
	std::uint16_t max_eobrun = 0;

	// Constructs Huffman codes from DHT data.
	HuffCodes(const std::uint8_t* dht_clen, const std::uint8_t* dht_cval) {
		int k = 0;
		int code = 0;

		// symbol-value of code is its position in the table
		for (int i = 0; i < 16; i++) {
			for (int j = 0; j < static_cast<int>(dht_clen[i]); j++) {
				clen[static_cast<int>(dht_cval[k])] = 1 + i;
				cval[static_cast<int>(dht_cval[k])] = code;

				k++;
				code++;
			}
			code = code << 1;
		}

		// find out eobrun max value
		for (int i = 14; i >= 0; i--) {
			if (clen[i << 4] > 0) {
				max_eobrun = (2 << i) - 1;
				break;
			}
		}
	}
};

#endif
