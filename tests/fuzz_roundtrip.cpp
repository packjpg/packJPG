#include "bitops.h"
#include "codec_pipeline.h"
#include "packjpglib.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

namespace {

bool has_clean_stripped_header(const std::vector<std::uint8_t>& jpg) {
	if (jpg.size() < 4 || jpg[0] != 0xFF || jpg[1] != 0xD8) {
		return false;
	}
	std::size_t pos = 2;
	while (pos + 4 <= jpg.size()) {
		if (jpg[pos] != 0xFF) {
			return false;
		}
		const std::uint8_t type = jpg[pos + 1];
		if (type == 0xDA) {
			return true;
		}
		if ((type >= 0xE0 && type <= 0xEF) || type == 0xFE) {
			return false;
		}
		const std::size_t len = 2u + ((static_cast<std::size_t>(jpg[pos + 2]) << 8) | jpg[pos + 3]);
		if (len < 4 || pos + len > jpg.size()) {
			return false;
		}
		pos += len;
	}
	return false;
}

bool compress_with_custom_settings(
	const std::uint8_t* data,
	std::size_t size,
	std::vector<std::uint8_t>& out_pjg) {
	CodecContext ctx;
	ctx.filetype = F_JPG;
	ctx.err_tol  = 1;
	ctx.auto_set = false;
	for (std::size_t c = 0; c < 4; ++c) {
		const std::uint8_t b0 = data[(c * 7u) % size];
		const std::uint8_t b1 = data[(c * 13u + 1u) % size];
		ctx.nois_trs[c] = static_cast<unsigned char>(b0 % 11u);
		ctx.segm_cnt[c] = static_cast<unsigned char>(1u + (b1 % 49u));
	}
	ctx.str_in = std::make_unique<MemoryReader>(data, size);
	unsigned char soi[2]{};
	ctx.str_in->read(soi, 2);
	auto mem_out = std::make_unique<MemoryWriter>();
	MemoryWriter* mem_out_ptr = mem_out.get();
	ctx.str_out = std::move(mem_out);
	ctx.reset_buffers();
	process_standard_file(ctx);
	if (ctx.errorlevel >= ctx.err_tol) {
		return false;
	}
	out_pjg = mem_out_ptr->take_data();
	return true;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
	if (size < 2 || size > (1u << 20)) {
		return 0;
	}

	const bool is_jpg = (data[0] == 0xFF && data[1] == 0xD8);
	const bool is_pjg = (data[0] == 'J' && data[1] == 'S');
	const std::uint8_t sel = data[size - 1];

	// 1. Strict C API round-trip (err_tol = 1): any accepted JPEG must round-trip byte-for-byte identically.
	unsigned char* out_buf = nullptr;
	unsigned int out_size = 0;
	pjglib_init_streams(const_cast<std::uint8_t*>(data), 1, static_cast<int>(size), nullptr, 1);
	const bool ok = pjglib_convert_stream2mem(&out_buf, &out_size, nullptr);

	if (ok) {
		if (out_buf == nullptr) {
			std::abort();
		}
		if (is_jpg) {
			if (out_size < 3 || out_buf[0] != 'J' || out_buf[1] != 'S') {
				std::abort();
			}
			unsigned char* rt_buf = nullptr;
			unsigned int rt_size = 0;
			pjglib_init_streams(out_buf, 1, static_cast<int>(out_size), nullptr, 1);
			if (!pjglib_convert_stream2mem(&rt_buf, &rt_size, nullptr) ||
				rt_buf == nullptr ||
				rt_size != size ||
				std::memcmp(rt_buf, data, size) != 0) {
				std::abort();
			}
			pjglib_free(rt_buf);

			// 2. In-place C++ API aliasing check (in_data points into out_data).
			if ((sel & 3u) == 0u) {
				std::vector<std::uint8_t> alias_buf(data, data + size);
				if (!packjpg::convert_memory(alias_buf.data(), alias_buf.size(), alias_buf, nullptr, false, 1) ||
					alias_buf.size() != out_size ||
					std::memcmp(alias_buf.data(), out_buf, out_size) != 0) {
					std::abort();
				}
			}

			// 3. Custom (-s / -t, auto_set = false) lossless round-trip differential check.
			std::vector<std::uint8_t> custom_pjg;
			std::vector<std::uint8_t> custom_rt;
			if (!compress_with_custom_settings(data, size, custom_pjg) ||
				!packjpg::convert_memory(custom_pjg.data(), custom_pjg.size(), custom_rt, nullptr, false, 1) ||
				custom_rt.size() != size ||
				std::memcmp(custom_rt.data(), data, size) != 0) {
				std::abort();
			}
		} else if (is_pjg) {
			if (out_size < 4 || out_buf[0] != 0xFF || out_buf[1] != 0xD8) {
				std::abort();
			}
		}
	}
	pjglib_free(out_buf);

	// 4. Metadata-discard (-d) differential & idempotence oracle on candidate JPEGs.
	if (is_jpg && (ok || (sel & 3u) == 1u)) {
		std::vector<std::uint8_t> pjg_d;
		if (packjpg::convert_memory(data, size, pjg_d, nullptr, true, 1)) {
			std::vector<std::uint8_t> jpg_stripped;
			std::vector<std::uint8_t> pjg_re;
			if (!packjpg::convert_memory(pjg_d.data(), pjg_d.size(), jpg_stripped, nullptr, false, 1) ||
				!has_clean_stripped_header(jpg_stripped) ||
				!packjpg::convert_memory(jpg_stripped.data(), jpg_stripped.size(), pjg_re, nullptr, false, 1) ||
				pjg_re != pjg_d) {
				std::abort();
			}
		}
	}

	// 5. Warning-tolerant (-p, err_tol = 2) compress + decompress stress under ASan/UBSan.
	if ((sel & 1u) != 0u) {
		std::vector<std::uint8_t> tolerant_out;
		const bool disc_meta = (sel & 4u) != 0u;
		if (packjpg::convert_memory(data, size, tolerant_out, nullptr, disc_meta, 2) &&
			tolerant_out.size() >= 2 &&
			tolerant_out[0] == 'J' && tolerant_out[1] == 'S') {
			std::vector<std::uint8_t> tolerant_rt;
			if (packjpg::convert_memory(tolerant_out.data(), tolerant_out.size(), tolerant_rt, nullptr, disc_meta, 2)) {
				if (tolerant_rt.size() < 4 || tolerant_rt[0] != 0xFF || tolerant_rt[1] != 0xD8) {
					std::abort();
				}
			}
		}
	}

	return 0;
}
