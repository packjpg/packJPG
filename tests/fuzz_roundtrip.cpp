#include "codec_pipeline.h"
#include "packjpglib.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
	if (size < 2 || size > (1u << 20)) {
		return 0;
	}

	// 1. Strict mode (err_tol = 1): any accepted JPEG must round-trip byte-for-byte identically.
	unsigned char* out_buf = nullptr;
	unsigned int out_size = 0;
	pjglib_init_streams(const_cast<std::uint8_t*>(data), 1, static_cast<int>(size), nullptr, 1);
	const bool ok = pjglib_convert_stream2mem(&out_buf, &out_size, nullptr);

	if (ok && out_buf != nullptr && data[0] == 0xFF && data[1] == 0xD8) {
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

	// 2. Also exercise warning-tolerant (-p, err_tol=2) and metadata-stripping (-d, discard_meta=true)
	//    paths on sampled inputs to cover jpg_rebuild_header and warning recovery under ASan/UBSan.
	if ((data[size - 1] & 3u) == 1u) {
		std::vector<std::uint8_t> tolerant_out;
		const bool disc_meta = (data[size - 1] & 4u) != 0u;
		if (packjpg::convert_memory(data, size, tolerant_out, nullptr, disc_meta, 2) &&
			tolerant_out.size() >= 2 &&
			tolerant_out[0] == 'J' && tolerant_out[1] == 'S') {
			std::vector<std::uint8_t> tolerant_rt;
			packjpg::convert_memory(tolerant_out.data(), tolerant_out.size(), tolerant_rt, nullptr, disc_meta, 2);
		}
	}

	return 0;
}
