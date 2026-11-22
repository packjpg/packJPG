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

TEST_CASE("core codec regressions: high-ratio solid image, 320KB generic trailer, 16-bit DQT, custom->auto_set reuse, AC refine DHT, and err_tol=2 PJG") {
	// 1. Solid 3072x3072 RGB 4:4:4 baseline JPEG (442,368 blocks -> >600 blocks/byte, exceeding old 512x cap!)
	{
		std::vector<std::uint8_t> solid_rgb = {
			0xFF, 0xD8,
			0xFF, 0xDB, 0x00, 0x43, 0x00,
		};
		solid_rgb.insert(solid_rgb.end(), 64, 16);
		// SOF0: 3072x3072, 3 components (1x1 each -> 442,368 total 8x8 blocks)
		const std::uint8_t sof3[] = {
			0xFF, 0xC0, 0x00, 0x11, 0x08,
			0x0C, 0x00, 0x0C, 0x00, // 3072 x 3072
			0x03,
			0x01, 0x11, 0x00,
			0x02, 0x11, 0x00,
			0x03, 0x11, 0x00,
			// DC table 0: symbol 0 -> 1-bit '0'
			0xFF, 0xC4, 0x00, 0x14, 0x00,
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00,
			// AC table 0: symbol 0 (EOB) -> 1-bit '0'
			0xFF, 0xC4, 0x00, 0x14, 0x10,
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00,
			// SOS: 3 components, all using Td=0/Ta=0; each block is 2 zero bits ('00') -> 442,368 * 2 bits = 110,592 zero bytes!
			0xFF, 0xDA, 0x00, 0x0C, 0x03, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00, 0x3F, 0x00,
		};
		solid_rgb.insert(solid_rgb.end(), std::begin(sof3), std::end(sof3));
		solid_rgb.insert(solid_rgb.end(), 110592, 0x00);
		solid_rgb.push_back(0xFF);
		solid_rgb.push_back(0xD9);

		std::vector<std::uint8_t> pjg;
		std::vector<std::uint8_t> rt;
		std::string err;
		REQUIRE(packjpg::convert_memory(solid_rgb.data(), solid_rgb.size(), pjg, &err));
		REQUIRE(pjg.size() < 4096u); // >27:1 lossless compression over 110 KB JPEG!
		REQUIRE(packjpg::convert_memory(pjg.data(), pjg.size(), rt, &err));

		REQUIRE(rt == solid_rgb);

		// Calling jpg_setup_imginfo directly with ctx.str_in == nullptr must also allow >65,536 blocks
		CodecContext hdr_ctx;
		hdr_ctx.filetype = F_PJG;
		hdr_ctx.hdrdata.assign(solid_rgb.begin() + 2, solid_rgb.begin() + 2 + 69 + 19);
		REQUIRE(jpg_setup_imginfo(hdr_ctx));
		REQUIRE_EQ(hdr_ctx.cmpnfo[0].bc, 147456);
	}



	// 2. Valid 8x8 JPEG with 320 KB of zero-filled trailing data (>1,000:1 generic stream compression ratio)
	{
		auto jpg_with_trailer = make_td0_ta1_jpeg(3);
		jpg_with_trailer.insert(jpg_with_trailer.end(), 320 * 1024, 0x00);
		std::vector<std::uint8_t> pjg;
		std::vector<std::uint8_t> rt;
		std::string err;
		REQUIRE(packjpg::convert_memory(jpg_with_trailer.data(), jpg_with_trailer.size(), pjg, &err));
		REQUIRE(pjg.size() < 1024u);
		REQUIRE(packjpg::convert_memory(pjg.data(), pjg.size(), rt, &err));
		REQUIRE(rt == jpg_with_trailer);
	}

	// 3. 16-bit DQT (Pq=1, quant=50000) SOF1 16x16 JPEG with non-zero low-AC coefficients (exercises 64-bit pjg_lakh_context)
	{
		std::vector<std::uint8_t> jpg16 = {
			0xFF, 0xD8,
			0xFF, 0xDB, 0x00, 0x83, 0x10, // Pq=1 (16-bit), Tq=0, length = 2 + 1 + 128 = 131 (0x0083)
		};
		for (int i = 0; i < 64; ++i) {
			const std::uint16_t q = (i == 0) ? 16u : 50000u;
			jpg16.push_back(static_cast<std::uint8_t>(q >> 8));
			jpg16.push_back(static_cast<std::uint8_t>(q & 0xFF));
		}
		// SOF1 (extended sequential): 16x16 grayscale (4 blocks)
		const std::uint8_t sof1[] = {
			0xFF, 0xC1, 0x00, 0x0B, 0x08, 0x00, 0x10, 0x00, 0x10, 0x01, 0x01, 0x11, 0x00,
		};
		jpg16.insert(jpg16.end(), std::begin(sof1), std::end(sof1));
		jpg16.push_back(0xFF); jpg16.push_back(0xC4); jpg16.push_back(0x00); jpg16.push_back(0x1F); jpg16.push_back(0x00);
		jpg16.insert(jpg16.end(), std_huff_tables[0], std_huff_tables[0] + std_huff_lengths[0]);
		jpg16.push_back(0xFF); jpg16.push_back(0xC4); jpg16.push_back(0x00); jpg16.push_back(static_cast<std::uint8_t>(3 + std_huff_lengths[2])); jpg16.push_back(0x10);
		jpg16.insert(jpg16.end(), std_huff_tables[2], std_huff_tables[2] + std_huff_lengths[2]);

		HuffCodes dc_hc{}, ac_hc{};
		HuffTree  dc_ht{}, ac_ht{};
		jpg_build_huffcodes(std_huff_tables[0], std_huff_tables[0] + 16, &dc_hc, &dc_ht);
		jpg_build_huffcodes(std_huff_tables[2], std_huff_tables[2] + 16, &ac_hc, &ac_ht);

		BitWriter bw(1);
		for (int blk = 0; blk < 4; ++blk) {
			bw.write_u16(dc_hc.cval[0], dc_hc.clen[0]); // DC diff = 0
			// Set first row/col AC coefficients to +1 (z=0, s=1 -> symbol 0x01, bit '1') for several zigzag positions
			for (int k = 1; k <= 15; ++k) {
				bw.write_u16(ac_hc.cval[0x01], ac_hc.clen[0x01]);
				bw.write_bit(1);
			}
			bw.write_u16(ac_hc.cval[0x00], ac_hc.clen[0x00]); // EOB
		}
		const auto raw_scan = bw.take_bytes();
		const std::uint8_t sos[] = {0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3F, 0x00};
		jpg16.insert(jpg16.end(), std::begin(sos), std::end(sos));
		for (std::uint8_t b : raw_scan) {
			jpg16.push_back(b);
			if (b == 0xFF) jpg16.push_back(0x00);
		}
		jpg16.push_back(0xFF); jpg16.push_back(0xD9);

		std::vector<std::uint8_t> pjg16;
		std::vector<std::uint8_t> rt16;
		std::string err16;
		REQUIRE(packjpg::convert_memory(jpg16.data(), jpg16.size(), pjg16, &err16));
		REQUIRE(packjpg::convert_memory(pjg16.data(), pjg16.size(), rt16, &err16));
		REQUIRE(rt16 == jpg16);
	}

	// 4. Custom-header PJG (auto_set=false) followed by standard PJG (auto_set=true) on the same CodecContext
	{
		const auto base_jpg = make_td0_ta1_jpeg(19);
		CodecContext ctx;
		ctx.auto_set  = false;
		ctx.nois_trs  = {2, 2, 2, 2};
		ctx.segm_cnt  = {25, 25, 25, 25};
		ctx.filetype  = F_JPG;
		ctx.str_in    = std::make_unique<MemoryReader>(base_jpg.data(), base_jpg.size());
		auto custom_out = std::make_unique<MemoryWriter>();
		MemoryWriter* custom_ptr = custom_out.get();
		ctx.str_out   = std::move(custom_out);
		std::uint8_t m[2];
		ctx.str_in->read(m, 2);
		ctx.reset_buffers();
		process_standard_file(ctx);
		REQUIRE_EQ(ctx.errorlevel, 0);
		const auto custom_pjg = custom_ptr->take_data();
		REQUIRE_EQ(int(custom_pjg[2]), 0x00); // custom settings marker

		std::vector<std::uint8_t> std_pjg;
		REQUIRE(packjpg::convert_memory(base_jpg.data(), base_jpg.size(), std_pjg));
		REQUIRE_EQ(int(std_pjg[2]), kAppVersion); // standard auto_set marker

		// Decompress custom_pjg, then immediately decompress std_pjg on the SAME CodecContext!
		CodecContext dec_ctx;
		const std::vector<std::uint8_t>* const sources[] = {&custom_pjg, &std_pjg};
		for (const auto* pjg_src : sources) {

			dec_ctx.errorlevel = 0;
			dec_ctx.filetype   = F_PJG;
			dec_ctx.str_in     = std::make_unique<MemoryReader>(pjg_src->data(), pjg_src->size());
			auto j_out         = std::make_unique<MemoryWriter>();
			MemoryWriter* j_ptr = j_out.get();
			dec_ctx.str_out    = std::move(j_out);
			dec_ctx.str_in->read(m, 2);
			dec_ctx.reset_buffers();
			process_standard_file(dec_ctx);
			REQUIRE_EQ(dec_ctx.errorlevel, 0);
			REQUIRE(j_ptr->take_data() == base_jpg);
		}
	}

	// 5. Progressive AC refinement scan referencing missing AC Huffman table is rejected cleanly
	{
		std::vector<std::uint8_t> bad_prog = {
			0xFF, 0xD8,
			0xFF, 0xDB, 0x00, 0x43, 0x00,
		};
		bad_prog.insert(bad_prog.end(), 64, 16);
		const std::uint8_t tail[] = {
			0xFF, 0xC2, 0x00, 0x0B, 0x08, 0x00, 0x08, 0x00, 0x08, 0x01, 0x01, 0x11, 0x00, // SOF2 progressive
			0xFF, 0xC4, 0x00, 0x14, 0x00,
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00,
			// Scan 0: DC initial (Ss=0, Se=0, Ah=0, Al=0)
			0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
			0x00,
			// Scan 1: AC refinement (Ss=1, Se=63, Ah=1, Al=0 -> 0x10) using undefined AC table 2 (0x02)!
			0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x02, 0x01, 0x3F, 0x10,
			0x00,
			0xFF, 0xD9,
		};
		bad_prog.insert(bad_prog.end(), std::begin(tail), std::end(tail));
		std::vector<std::uint8_t> out;
		std::string err;
		REQUIRE(!packjpg::convert_memory(bad_prog.data(), bad_prog.size(), out, &err, false, 2));
	}

	// 6. JPEG with unknown marker (FF F0) compressed with err_tol=2 decompresses cleanly with default err_tol=1
	{
		auto quirky_jpg = make_td0_ta1_jpeg(7);
		// Insert FF F0 (length 4: 0x00, 0x04, 'A', 'B') right after SOI (0xFF, 0xD8)
		const std::uint8_t unk_seg[] = {0xFF, 0xF0, 0x00, 0x04, 'A', 'B'};
		quirky_jpg.insert(quirky_jpg.begin() + 2, std::begin(unk_seg), std::end(unk_seg));

		std::vector<std::uint8_t> pjg;
		std::vector<std::uint8_t> rt;
		std::string err;
		REQUIRE(!packjpg::convert_memory(quirky_jpg.data(), quirky_jpg.size(), pjg, &err, false, 1));
		REQUIRE(packjpg::convert_memory(quirky_jpg.data(), quirky_jpg.size(), pjg, &err, false, 2));
		// Default strict err_tol=1 must decompress that .pjg back to exact quirky_jpg!
		REQUIRE(packjpg::convert_memory(pjg.data(), pjg.size(), rt, &err, false, 1));
		REQUIRE(rt == quirky_jpg);
	}

	// 7. Finding #16 regression: duplicate DHT filler symbol at length 16 must NOT overwrite primary shorter codeword under -p (err_tol=2)
	{
		std::vector<std::uint8_t> dup_dht_jpg = {
			0xFF, 0xD8,
			0xFF, 0xDB, 0x00, 0x43, 0x00,
		};
		dup_dht_jpg.insert(dup_dht_jpg.end(), 64, 16);
		const std::uint8_t sof_and_tables[] = {
			0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x08, 0x00, 0x08, 0x01, 0x01, 0x11, 0x00,
			// DC table 0: symbol 0 -> 1-bit '0'
			0xFF, 0xC4, 0x00, 0x14, 0x00,
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00,
			// AC table 0: clen[0]=1 ('0' -> 0x00) AND clen[15]=2 (two 16-bit filler leaves: 0x01 and duplicate 0x00!)
			0xFF, 0xC4, 0x00, 0x16, 0x10,
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
			0x00, 0x01, 0x00,
			// SOS: DC='0', AC EOB='0' (primary 1-bit code!), padded '111111' -> 0x3F
			0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3F, 0x00,
			0x3F,
			0xFF, 0xD9,
		};
		dup_dht_jpg.insert(dup_dht_jpg.end(), std::begin(sof_and_tables), std::end(sof_and_tables));

		std::vector<std::uint8_t> pjg;
		std::vector<std::uint8_t> rt;
		std::string err;
		REQUIRE(!packjpg::convert_memory(dup_dht_jpg.data(), dup_dht_jpg.size(), pjg, &err, false, 1));
		REQUIRE(packjpg::convert_memory(dup_dht_jpg.data(), dup_dht_jpg.size(), pjg, &err, false, 2));
		REQUIRE(packjpg::convert_memory(pjg.data(), pjg.size(), rt, &err, false, 1));
		REQUIRE(rt == dup_dht_jpg);
	}

	// 8. Finding #17 regression: dct_rescale handles saturated INT_MAX / INT_MIN IDCT outputs without signed 32-bit overflow UB
	{
		REQUIRE_EQ(dct_rescale(0), 0);
		REQUIRE_EQ(dct_rescale(4096), 1);
		REQUIRE_EQ(dct_rescale(-4096), -1);
		REQUIRE_EQ(dct_rescale(2147483647), 262144);
		REQUIRE_EQ(dct_rescale(-2147483647 - 1), -262144);
	}

	// 9. Finding #18 & #14c regressions: FF FF fill byte rejection, err_tol=2 truncated huge-SOF bound, and auto_set segm_cnt/nois_trs reset
	{
		auto fill_jpg = make_td0_ta1_jpeg(4);
		// Inject extra 0xFF fill byte before first marker (0xFF 0xFF 0xDB)
		fill_jpg.insert(fill_jpg.begin() + 2, 0xFF);
		std::vector<std::uint8_t> out;
		std::string err;
		REQUIRE(!packjpg::convert_memory(fill_jpg.data(), fill_jpg.size(), out, &err, false, 2));

		// Truncated sequential JPEG declaring 16384x16384 (4,194,304 blocks) with only 2 scan bytes under err_tol=2
		std::vector<std::uint8_t> trunc_huge = {
			0xFF, 0xD8,
			0xFF, 0xDB, 0x00, 0x43, 0x00,
		};
		trunc_huge.insert(trunc_huge.end(), 64, 16);
		const std::uint8_t huge_tail[] = {
			0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x40, 0x00, 0x40, 0x00, 0x01, 0x01, 0x11, 0x00,
			0xFF, 0xC4, 0x00, 0x14, 0x00,
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xFF, 0xC4, 0x00, 0x14, 0x10,
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3F, 0x00,
			0x00, 0x00,
			0xFF, 0xD9,
		};
		trunc_huge.insert(trunc_huge.end(), std::begin(huge_tail), std::end(huge_tail));
		REQUIRE(!packjpg::convert_memory(trunc_huge.data(), trunc_huge.size(), out, &err, false, 2));

		// Verify jpg_setup_imginfo resets unused component slots [1..3] when auto_set == true
		const auto gray_jpg = make_td0_ta1_jpeg(1);
		CodecContext dirty_ctx;
		dirty_ctx.filetype = F_JPG;
		dirty_ctx.auto_set = true;
		dirty_ctx.segm_cnt = {49, 49, 49, 49};
		dirty_ctx.nois_trs = {9, 9, 9, 9};
		dirty_ctx.str_in   = std::make_unique<MemoryReader>(gray_jpg.data(), gray_jpg.size());
		std::uint8_t m[2];
		dirty_ctx.str_in->read(m, 2);
		dirty_ctx.reset_buffers();
		REQUIRE(read_jpeg(dirty_ctx));
		REQUIRE_EQ(int(dirty_ctx.segm_cnt[1]), 10);
		REQUIRE_EQ(int(dirty_ctx.segm_cnt[2]), 10);
		REQUIRE_EQ(int(dirty_ctx.segm_cnt[3]), 10);
		REQUIRE_EQ(int(dirty_ctx.nois_trs[1]), 6);
		REQUIRE_EQ(int(dirty_ctx.nois_trs[2]), 6);
		REQUIRE_EQ(int(dirty_ctx.nois_trs[3]), 6);

		// 18-byte E3 0x40000000 fixed-point PJG unit discovered by libFuzzer must reject in milliseconds!
		const std::uint8_t slow18_pjg[] = {
			0x4A, 0x53, 0x19, 0xFA, 0xFF, 0x00, 0x1B, 0x01, 0x07,
			0xFF, 0xFF, 0xFF, 0x1B, 0x01, 0x00, 0x7F, 0x77, 0xFF,
		};
		REQUIRE(!packjpg::convert_memory(slow18_pjg, sizeof(slow18_pjg), out, &err, false, 1));
	}
}

TEST_CASE("pass-3 codec regressions: AC-only progressive SOF2 budget and merge_jpeg leading/consecutive RST reconstruction") {
	// 1. Crafted <200-byte 65535x2048 4-component (4x4) AC-only SOF2 progressive header (>8.3M blocks, missing DC scans)
	//    must be rejected BEFORE allocating colldata in both strict (err_tol=1) and tolerant (err_tol=2) modes!
	{
		std::vector<std::uint8_t> ac_only_sof2 = {
			0xFF, 0xD8,
			0xFF, 0xDB, 0x00, 0x43, 0x00,
		};
		ac_only_sof2.insert(ac_only_sof2.end(), 64, 16);
		const std::uint8_t sof2_tail[] = {
			// SOF2: height=2048 (0x0800), width=65535 (0xFFFF), 4 components, 4x4 sampling -> >8.38M blocks (>1 GiB colldata if allocated)
			0xFF, 0xC2, 0x00, 0x14, 0x08, 0x08, 0x00, 0xFF, 0xFF, 0x04,
			0x01, 0x44, 0x00,
			0x02, 0x44, 0x00,
			0x03, 0x44, 0x00,
			0x04, 0x44, 0x00,
			// Minimal AC table 0 with EOB14 (0xE0) -> 1-bit '0'
			0xFF, 0xC4, 0x00, 0x14, 0x10,
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xE0,
			// SOS: AC-only scan (Ss=1, Se=63, Ah=0, Al=0) on component 1 — no DC scans present!
			0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x01, 0x3F, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0xFF, 0xD9,
		};
		ac_only_sof2.insert(ac_only_sof2.end(), std::begin(sof2_tail), std::end(sof2_tail));

		for (int tol : {1, 2}) {
			CodecContext ctx;
			ctx.filetype = F_JPG;
			ctx.err_tol  = tol;
			ctx.str_in   = std::make_unique<MemoryReader>(ac_only_sof2.data(), ac_only_sof2.size());
			std::uint8_t m[2];
			REQUIRE_EQ(ctx.str_in->read(m, 2), std::size_t(2));
			ctx.reset_buffers();
			REQUIRE(!read_jpeg(ctx));
			REQUIRE(ctx.colldata[0][0].empty());
		}
	}

	// 2. Leading RST marker (0xFFFFFFFF sentinel at scan byte 0) + consecutive mid-scan RST markers
	//    must be reconstructed byte-for-byte by merge_jpeg under err_tol = 2!
	{
		std::vector<std::uint8_t> rst_jpg = {
			0xFF, 0xD8,
			0xFF, 0xDB, 0x00, 0x43, 0x00,
		};
		rst_jpg.insert(rst_jpg.end(), 64, 8);
		const std::uint8_t rst_tail[] = {
			0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x08, 0x00, 0x10, 0x01, 0x01, 0x11, 0x00,
			0xFF, 0xDD, 0x00, 0x04, 0x00, 0x01, // DRI = 1
			0xFF, 0xC4, 0x00, 0x14, 0x00,
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xFF, 0xC4, 0x00, 0x14, 0x10,
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3F, 0x00,
			// Leading FFD0 at scan start (byte 0), entropy byte 0x3F, consecutive FFD1 + FFD2, entropy byte 0x3F, trailing FFD3!
			0xFF, 0xD0,
			0x3F,
			0xFF, 0xD1, 0xFF, 0xD2,
			0x3F,
			0xFF, 0xD3,
			0xFF, 0xD9,
		};
		rst_jpg.insert(rst_jpg.end(), std::begin(rst_tail), std::end(rst_tail));

		CodecContext ctx;
		ctx.filetype = F_JPG;
		ctx.err_tol  = 2;
		ctx.str_in   = std::make_unique<MemoryReader>(rst_jpg.data(), rst_jpg.size());
		auto merged  = std::make_unique<MemoryWriter>();
		MemoryWriter* merged_ptr = merged.get();
		ctx.str_out  = std::move(merged);
		std::uint8_t m[2];
		REQUIRE_EQ(ctx.str_in->read(m, 2), std::size_t(2));
		ctx.reset_buffers();
		REQUIRE(read_jpeg(ctx));
		REQUIRE_EQ(ctx.errorlevel, 1);
		REQUIRE(!ctx.rstp.empty());
		REQUIRE_EQ(ctx.rstp[0], 0xFFFFFFFFu);
		REQUIRE(merge_jpeg(ctx));
		REQUIRE(merged_ptr->take_data() == rst_jpg);
	}
}



