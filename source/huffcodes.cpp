#include "huffcodes.h"

HuffCodes::HuffCodes(const std::vector<std::uint8_t>& counts, const std::vector<std::uint8_t>& values) {
	int k = 0;
	int code = 0;

	// symbol-value of code is its position in the table
	for (int i = 0; i < 16; i++) {
		for (std::uint8_t j = 0; j < counts[i]; j++) {
			clen[values[k]] = 1 + i;
			cval[values[k]] = code;

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
