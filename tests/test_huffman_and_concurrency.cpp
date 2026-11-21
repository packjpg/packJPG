#include "test_framework.h"

#include "codec_pipeline.h"
#include "jpeg_codec.h"
#include "packjpglib.h"

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace {

// Baseline 8x8 grayscale JPEG where SOS selects Td=0 (DC table 0) and Ta=1 (AC table 1).
// DC diff = +5 -> s=3, envli(3, 5)=5 ('101'); AC [1] = -2 -> s=2, z=0 -> symbol 0x02, envli(2, -2)=1 ('01'), then EOB (0x00).
std::vector<std::uint8_t> make_td0_ta1_jpeg(int dc_val) {
	std::vector<std::uint8_t> jpg = {
		0xFF, 0xD8,
		0xFF, 0xDB, 0x00, 0x43, 0x00,
	};
	jpg.insert(jpg.end(), 64, 8);

	// SOF0: 8x8, 1 component (id=1, sf=1x1, qt=0)
	const std::uint8_t sof[] = {
		0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x08, 0x00, 0x08, 0x01, 0x01, 0x11, 0x00,
	};
	jpg.insert(jpg.end(), std::begin(sof), std::end(sof));

	// DHT DC table 0 (0x00): symbols 0..11 (standard luma DC)
	jpg.push_back(0xFF); jpg.push_back(0xC4);
	jpg.push_back(0x00); jpg.push_back(0x1F);
	jpg.push_back(0x00);
	jpg.insert(jpg.end(), std_huff_tables[0], std_huff_tables[0] + std_huff_lengths[0]);

	// DHT AC table 1 (0x11) ONLY (note: AC table 0 is intentionally NOT defined!)
	jpg.push_back(0xFF); jpg.push_back(0xC4);
	jpg.push_back(0x00); jpg.push_back(static_cast<std::uint8_t>(3 + std_huff_lengths[3]));
	jpg.push_back(0x11);
	jpg.insert(jpg.end(), std_huff_tables[3], std_huff_tables[3] + std_huff_lengths[3]);

	// Build canonical Huffman codes for DC0 and AC1 to encode one valid block
	HuffCodes dc_hc{}, ac_hc{};
	HuffTree  dc_ht{}, ac_ht{};
	jpg_build_huffcodes(std_huff_tables[0], std_huff_tables[0] + 16, &dc_hc, &dc_ht);
	jpg_build_huffcodes(std_huff_tables[3], std_huff_tables[3] + 16, &ac_hc, &ac_ht);

	BitWriter bw(1);
	const int dc_s = bitlen_2048n(dc_val);
	bw.write_u16(dc_hc.cval[dc_s], dc_hc.clen[dc_s]);
	bw.write_u16(static_cast<std::uint16_t>(envli(dc_s, dc_val)), static_cast<std::size_t>(dc_s));
	// AC[1] = -2 (z=0, s=2 -> symbol 0x02), then EOB (0x00)
	bw.write_u16(ac_hc.cval[0x02], ac_hc.clen[0x02]);
	bw.write_u16(static_cast<std::uint16_t>(envli(2, -2)), 2);
	bw.write_u16(ac_hc.cval[0x00], ac_hc.clen[0x00]);
	const auto scan_raw = bw.take_bytes();

	// SOS: 1 component, id=1, Td=0 / Ta=1 -> 0x01!
	const std::uint8_t sos[] = {
		0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x01, 0x00, 0x3F, 0x00,
	};
	jpg.insert(jpg.end(), std::begin(sos), std::end(sos));
	for (std::uint8_t b : scan_raw) {
		jpg.push_back(b);
		if (b == 0xFF) jpg.push_back(0x00);
	}
	jpg.push_back(0xFF); jpg.push_back(0xD9);
	return jpg;
}

} // namespace

TEST_CASE("codec helpers: VLI, fdiv2, and clamped bitlen lookups") {
	for (int v = -1023; v <= 1023; ++v) {
		const int s = bitlen_2048n(v);
		const int encoded = envli(s, v);
		REQUIRE_EQ(devli(s, encoded), v);
	}
	REQUIRE_EQ(fdiv2(-7, 1), -3);
	REQUIRE_EQ(fdiv2(7, 1), 3);
	REQUIRE_EQ(bitlen_1024p(-5), 0);
	REQUIRE_EQ(bitlen_1024p(0), 0);
	REQUIRE_EQ(bitlen_1024p(1024), 11);
	REQUIRE_EQ(bitlen_1024p(2047), 11);
	REQUIRE_EQ(bitlen_2048n(-5000), 12);
	REQUIRE_EQ(bitlen_2048n(5000), 11);
	REQUIRE_EQ(jpg_vli_bitlen(-2048), 12);
	REQUIRE_EQ(jpg_vli_bitlen(2047), 11);
	REQUIRE_EQ(jpg_vli_bitlen(2048), 12);
	REQUIRE_EQ(jpg_vli_bitlen(4095), 12);

	const std::vector<unsigned char> mixed_pad = {0xAE}; // 101 | 01110 -> mixed 0/1 padbits
	BitReader br_mixed(mixed_pad.data(), 1);
	REQUIRE_EQ(br_mixed.read(3), 0x05u);
	REQUIRE_EQ(int(br_mixed.unpad(0)), 2);
}

TEST_CASE("jpeg_codec: sequential JPEG with distinct Td=0 and Ta=1 Huffman tables round-trips") {
	const auto jpg = make_td0_ta1_jpeg(12);
	std::vector<std::uint8_t> pjg;
	std::vector<std::uint8_t> restored;
	std::string err;

	REQUIRE(packjpg::convert_memory(jpg.data(), jpg.size(), pjg, &err));
	REQUIRE(err.empty());
	REQUIRE(packjpg::convert_memory(pjg.data(), pjg.size(), restored, &err));
	REQUIRE(restored == jpg);
}

TEST_CASE("concurrency: parallel packjpg::convert_memory and thread_local pjglib_* are race-free") {
	std::vector<std::vector<std::uint8_t>> inputs;
	for (int i = 0; i < 8; ++i) {
		inputs.push_back(make_td0_ta1_jpeg((i - 4) * 9));
	}

	std::atomic<int> failures{0};
	std::vector<std::thread> threads;
	threads.reserve(inputs.size());

	for (std::size_t t = 0; t < inputs.size(); ++t) {
		threads.emplace_back([&, t]() {
			const auto& src = inputs[t];
			for (int iter = 0; iter < 12; ++iter) {
				// 1. Test C++17 reentrant CodecContext API
				std::vector<std::uint8_t> pjg;
				std::vector<std::uint8_t> rt;
				if (!packjpg::convert_memory(src.data(), src.size(), pjg) ||
					!packjpg::convert_memory(pjg.data(), pjg.size(), rt) ||
					rt != src) {
					failures.fetch_add(1);
					return;
				}

				// 2. Test legacy C API thread_local session isolation
				unsigned char* c_pjg = nullptr;
				unsigned int c_pjg_sz = 0;
				pjglib_init_streams(const_cast<std::uint8_t*>(src.data()), 1, static_cast<int>(src.size()), nullptr, 1);
				if (!pjglib_convert_stream2mem(&c_pjg, &c_pjg_sz, nullptr) || c_pjg == nullptr) {
					failures.fetch_add(1);
					return;
				}

				unsigned char* c_rt = nullptr;
				unsigned int c_rt_sz = 0;
				pjglib_init_streams(c_pjg, 1, static_cast<int>(c_pjg_sz), nullptr, 1);
				const bool ok = pjglib_convert_stream2mem(&c_rt, &c_rt_sz, nullptr);
				std::free(c_pjg);
				if (!ok || c_rt == nullptr || c_rt_sz != src.size() || std::memcmp(c_rt, src.data(), src.size()) != 0) {
					std::free(c_rt);
					failures.fetch_add(1);
					return;
				}
				std::free(c_rt);
			}
		});
	}

	for (auto& th : threads) {
		th.join();
	}
	REQUIRE_EQ(failures.load(), 0);
}
