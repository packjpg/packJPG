#include "packjpglib.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
	if (size < 2 || size > (1u << 20)) {
		return 0;
	}

	unsigned char* out_buf = nullptr;
	unsigned int out_size = 0;
	pjglib_init_streams(const_cast<std::uint8_t*>(data), 1, static_cast<int>(size), nullptr, 1);
	const bool ok = pjglib_convert_stream2mem(&out_buf, &out_size, nullptr);

	if (ok && out_buf != nullptr && data[0] == 0xFF && data[1] == 0xD8) {
		// Differential invariant: any accepted JPEG must decompress back byte-for-byte identically!
		unsigned char* rt_buf = nullptr;
		unsigned int rt_size = 0;
		pjglib_init_streams(out_buf, 1, static_cast<int>(out_size), nullptr, 1);
		if (!pjglib_convert_stream2mem(&rt_buf, &rt_size, nullptr) ||
			rt_size != size ||
			std::memcmp(rt_buf, data, size) != 0) {
			std::abort();
		}
		std::free(rt_buf);
	}

	std::free(out_buf);
	return 0;
}
