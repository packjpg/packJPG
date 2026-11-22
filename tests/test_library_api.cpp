#include "test_framework.h"

// Bug #13 regression: including BOTH packjpglib.h and packjpgdll.h in the same translation unit must compile cleanly!
#include "packjpglib.h"
#include "packjpgdll.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#if !defined(_WIN32) && !defined(WIN32)
#include <sys/stat.h>
#endif

namespace {

// Valid 8x8 grayscale baseline JPEG (DC=0 -> '00', AC EOB -> '0', padded '11111' -> 0x1F)
std::vector<unsigned char> make_minimal_jpeg() {
	std::vector<unsigned char> jpg = {
		0xFF, 0xD8,
		0xFF, 0xDB, 0x00, 0x43, 0x00,
	};
	jpg.insert(jpg.end(), 64, 16);
	const unsigned char tail[] = {
		0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x08, 0x00, 0x08, 0x01, 0x01, 0x11, 0x00,
		0xFF, 0xC4, 0x00, 0x1F, 0x00,
		0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
		0xFF, 0xC4, 0x00, 0x14, 0x10,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00,
		0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3F, 0x00,
		0x1F, 0xFF, 0xD9,
	};
	jpg.insert(jpg.end(), std::begin(tail), std::end(tail));
	return jpg;
}

} // namespace

TEST_CASE("library: version, PJGLIB_MSG_SIZE, pjglib_free, and IEEE subnormal preservation") {
	REQUIRE_EQ(int(PJGLIB_MSG_SIZE), 128);
	pjglib_free(nullptr);

	// Bug #13 regression: linking packJPG must NOT enable FTZ/DAZ via -ffast-math crtfastmath.o
	volatile double subnormal = std::numeric_limits<double>::denorm_min();
	volatile double halved = subnormal * 1.0;
	REQUIRE(halved > 0.0);

	const char* ver = pjglib_version_info();
	const char* name = pjglib_short_name();
	REQUIRE(ver != nullptr);
	REQUIRE(name != nullptr);
	REQUIRE(std::strstr(ver, "v2.5") != nullptr);
	REQUIRE(std::strncmp(name, "packJPG v2.5", 12) == 0);
}

TEST_CASE("library: memory-to-memory compress/decompress via pjglib_free and out_type validation") {
	auto jpg = make_minimal_jpeg();
	char msg[PJGLIB_MSG_SIZE] = {0};

	unsigned char* pjg_buf = nullptr;
	unsigned int pjg_size = 0;
	pjglib_init_streams(jpg.data(), 1, static_cast<int>(jpg.size()), nullptr, 1);
	REQUIRE(pjglib_convert_stream2mem(&pjg_buf, &pjg_size, msg));
	REQUIRE(pjg_buf != nullptr);
	REQUIRE(pjg_size > 2);
	REQUIRE_EQ(char(pjg_buf[0]), 'J');
	REQUIRE_EQ(char(pjg_buf[1]), 'S');

	unsigned char* rt_buf = nullptr;
	unsigned int rt_size = 0;
	pjglib_init_streams(pjg_buf, 1, static_cast<int>(pjg_size), nullptr, 1);
	REQUIRE(pjglib_convert_stream2mem(&rt_buf, &rt_size, msg));
	REQUIRE(rt_buf != nullptr);
	REQUIRE_EQ(std::size_t(rt_size), jpg.size());
	REQUIRE_EQ(std::memcmp(rt_buf, jpg.data(), jpg.size()), 0);

	pjglib_free(pjg_buf);
	pjglib_free(rt_buf);

	// Bug #13 regression: pjglib_convert_stream2mem must reject sessions initialized with out_type != 1
	pjg_test::TempTestDir tmp_dir("pjglib_outtype");
	const auto dummy_file = (tmp_dir.path() / "unused_target.pjg").string();
	unsigned char* wrong_buf = reinterpret_cast<unsigned char*>(std::uintptr_t(0x1));
	unsigned int wrong_sz = 999;
	pjglib_init_streams(jpg.data(), 1, static_cast<int>(jpg.size()), const_cast<char*>(dummy_file.c_str()), 0);
	REQUIRE(!pjglib_convert_stream2mem(&wrong_buf, &wrong_sz, msg));
	REQUIRE(wrong_buf == nullptr);
	REQUIRE_EQ(wrong_sz, 0u);
	REQUIRE(!std::filesystem::exists(dummy_file));
}

TEST_CASE("library: file-to-file round-trip and recovery after invalid input") {
	pjg_test::TempTestDir tmp_dir("pjglib_f2f");
	const auto in_jpg = (tmp_dir.path() / "pjglib_test_in.jpg").string();
	const auto out_pjg = (tmp_dir.path() / "pjglib_test_out.pjg").string();
	const auto rt_jpg = (tmp_dir.path() / "pjglib_test_rt.jpg").string();

	auto jpg = make_minimal_jpeg();
	{
		std::ofstream ofs(in_jpg, std::ios::binary);
		ofs.write(reinterpret_cast<const char*>(jpg.data()), static_cast<std::streamsize>(jpg.size()));
	}

	// First feed an unknown/corrupt input and ensure subsequent valid call succeeds cleanly
	unsigned char bad_magic[8] = {'N', 'O', 'T', '_', 'J', 'P', 'G', '!'};
	char msg[256] = {0};
	unsigned char* dummy_out = nullptr;
	unsigned int dummy_sz = 0;
	pjglib_init_streams(bad_magic, 1, sizeof(bad_magic), nullptr, 1);
	REQUIRE(!pjglib_convert_stream2mem(&dummy_out, &dummy_sz, msg));
	REQUIRE(std::strlen(msg) > 0);

	std::vector<char> in_mut(in_jpg.begin(), in_jpg.end());
	in_mut.push_back('\0');
	std::vector<char> pjg_mut(out_pjg.begin(), out_pjg.end());
	pjg_mut.push_back('\0');
	std::vector<char> rt_mut(rt_jpg.begin(), rt_jpg.end());
	rt_mut.push_back('\0');

	REQUIRE(pjglib_convert_file2file(in_mut.data(), pjg_mut.data(), msg));
	REQUIRE(pjglib_convert_file2file(pjg_mut.data(), rt_mut.data(), msg));

	std::vector<unsigned char> restored;
	{
		std::ifstream ifs(rt_jpg, std::ios::binary);
		restored.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	}
	REQUIRE(restored == jpg);
}

TEST_CASE("library: FILE* stream2stream round-trip, closed-FILE* session reset safety, and clean discard") {
	pjg_test::TempTestDir tmp_dir("pjglib_fp");
	const auto in_path = (tmp_dir.path() / "pjglib_fp_in.bin").string();
	const auto mid_path = (tmp_dir.path() / "pjglib_fp_mid.bin").string();
	const auto out_path = (tmp_dir.path() / "pjglib_fp_out.bin").string();

	const auto jpg = make_minimal_jpeg();
	{
		std::ofstream ofs(in_path, std::ios::binary);
		ofs.write(reinterpret_cast<const char*>(jpg.data()), static_cast<std::streamsize>(jpg.size()));
	}

	char msg[256] = {0};
	{
		FILE* fin = std::fopen(in_path.c_str(), "rb");
		FILE* fout = std::fopen(mid_path.c_str(), "wb");
		REQUIRE(fin != nullptr && fout != nullptr);
		pjglib_init_streams(fin, 2, 0, fout, 2);
		REQUIRE(pjglib_convert_stream2stream(msg));
		std::fclose(fin);
		std::fclose(fout);
	}
	{
		FILE* fin = std::fopen(mid_path.c_str(), "rb");
		FILE* fout = std::fopen(out_path.c_str(), "wb");
		REQUIRE(fin != nullptr && fout != nullptr);
		pjglib_init_streams(fin, 2, 0, fout, 2);
		REQUIRE(pjglib_convert_stream2stream(msg));
		std::fclose(fin);
		std::fclose(fout);
	}
	{
		std::ifstream ifs(out_path, std::ios::binary);
		std::vector<unsigned char> restored((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		REQUIRE(restored == jpg);
	}

	// Bug #12 regression: abandoned pjglib_init_streams(..., out_type=2) followed by fclose() and re-init must NOT touch closed FILE*!
	{
		FILE* fin = std::fopen(in_path.c_str(), "rb");
		FILE* fout = std::fopen(out_path.c_str(), "wb");
		REQUIRE(fin != nullptr && fout != nullptr);
		pjglib_init_streams(fin, 2, 0, fout, 2);
		std::fclose(fin);
		std::fclose(fout);
		pjglib_init_streams(const_cast<unsigned char*>(jpg.data()), 1, static_cast<int>(jpg.size()), nullptr, 1);
		unsigned char* mem_out = nullptr;
		unsigned int mem_sz = 0;
		REQUIRE(pjglib_convert_stream2mem(&mem_out, &mem_sz, msg));
		pjglib_free(mem_out);
	}

	// Corrupt input with FILE* output must leave output FILE* completely empty
	{
		std::ofstream ofs(in_path, std::ios::binary);
		ofs.write("JS\x19\xff\xff\xff", 6);
	}
	{
		FILE* fin = std::fopen(in_path.c_str(), "rb");
		FILE* fout = std::fopen(out_path.c_str(), "wb");
		REQUIRE(fin != nullptr && fout != nullptr);
		pjglib_init_streams(fin, 2, 0, fout, 2);
		REQUIRE(!pjglib_convert_stream2stream(msg));
		std::fclose(fin);
		std::fclose(fout);
		REQUIRE_EQ(std::filesystem::file_size(out_path), std::uintmax_t(0));
	}

	// Null/invalid pjglib_init_streams arguments fail gracefully
	pjglib_init_streams(nullptr, 0, 0, nullptr, 1);
	REQUIRE(!pjglib_convert_stream2stream(msg));
	pjglib_init_streams(nullptr, 1, 10, nullptr, 1);
	REQUIRE(!pjglib_convert_stream2stream(msg));
	pjglib_init_streams(nullptr, 2, 0, nullptr, 1);
	REQUIRE(!pjglib_convert_stream2stream(msg));
}

TEST_CASE("library: valid 256-symbol depth-16 AC Huffman table (262 internal nodes) round-trips") {
	std::vector<unsigned char> jpg = {
		0xFF, 0xD8,
		0xFF, 0xDB, 0x00, 0x43, 0x00,
	};
	jpg.insert(jpg.end(), 64, 16);
	const unsigned char sof_and_dc[] = {
		0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x08, 0x00, 0x08, 0x01, 0x01, 0x11, 0x00,
		0xFF, 0xC4, 0x00, 0x14, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00,
	};
	jpg.insert(jpg.end(), std::begin(sof_and_dc), std::end(sof_and_dc));

	const unsigned char ac_hdr[] = {
		0xFF, 0xC4, 0x01, 0x13, 0x10,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF,
	};
	jpg.insert(jpg.end(), std::begin(ac_hdr), std::end(ac_hdr));
	for (int s = 0; s < 256; ++s) {
		jpg.push_back(static_cast<unsigned char>(s));
	}
	const unsigned char sos_and_scan[] = {
		0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3F, 0x00,
		0x00, 0x00,
		0xFF, 0xD9,
	};
	jpg.insert(jpg.end(), std::begin(sos_and_scan), std::end(sos_and_scan));

	unsigned char* pjg = nullptr;
	unsigned int pjg_sz = 0;
	pjglib_init_streams(jpg.data(), 1, static_cast<int>(jpg.size()), nullptr, 1);
	REQUIRE(pjglib_convert_stream2mem(&pjg, &pjg_sz, nullptr));

	unsigned char* rt = nullptr;
	unsigned int rt_sz = 0;
	pjglib_init_streams(pjg, 1, static_cast<int>(pjg_sz), nullptr, 1);
	REQUIRE(pjglib_convert_stream2mem(&rt, &rt_sz, nullptr));
	REQUIRE_EQ(std::size_t(rt_sz), jpg.size());
	REQUIRE_EQ(std::memcmp(rt, jpg.data(), jpg.size()), 0);

	pjglib_free(pjg);
	pjglib_free(rt);
}

TEST_CASE("library: atomic staging preserves pre-existing target file on warning or decode failure (Finding #9)") {
	pjg_test::TempTestDir tmp_dir("pjglib_atomic");
	const auto in_jpg = (tmp_dir.path() / "pjglib_safe_in.jpg").string();
	const auto important_pjg = (tmp_dir.path() / "pjglib_important.pjg").string();
	const auto bad_in = (tmp_dir.path() / "pjglib_not_an_image.bin").string();
	const auto warn_jpg_path = (tmp_dir.path() / "pjglib_warn.jpg").string();
	const auto corrupt_jpg_path = (tmp_dir.path() / "pjglib_corrupt.jpg").string();
	const auto untouched_out = (tmp_dir.path() / "pjglib_untouched_out.pjg").string();
	const auto pre_existing = (tmp_dir.path() / "pjglib_preexisting.bin").string();

	const auto jpg = make_minimal_jpeg();
	{
		std::ofstream ofs(in_jpg, std::ios::binary);
		ofs.write(reinterpret_cast<const char*>(jpg.data()), static_cast<std::streamsize>(jpg.size()));
	}
	{
		std::ofstream ofs(bad_in, std::ios::binary);
		ofs.write("NOT_A_JPEG_OR_PJG", 17);
	}
	{
		// Valid JPEG magic + unknown marker FF F0 -> passes header read, triggers errorlevel=1 warning in strict mode!
		auto warn_jpg = jpg;
		const unsigned char unk[] = {0xFF, 0xF0, 0x00, 0x04, 'X', 'Y'};
		warn_jpg.insert(warn_jpg.begin() + 2, std::begin(unk), std::end(unk));
		std::ofstream ofs(warn_jpg_path, std::ios::binary);
		ofs.write(reinterpret_cast<const char*>(warn_jpg.data()), static_cast<std::streamsize>(warn_jpg.size()));
	}
	{
		// Valid JPEG magic + truncated header -> fails in read_jpeg AFTER magic check!
		const unsigned char trunc_jpg[] = {0xFF, 0xD8, 0xFF, 0xC0, 0x00, 0x0B, 0x08};
		std::ofstream ofs(corrupt_jpg_path, std::ios::binary);
		ofs.write(reinterpret_cast<const char*>(trunc_jpg), sizeof(trunc_jpg));
	}
	{
		std::ofstream ofs(pre_existing, std::ios::binary);
		ofs.write("KEEP_ME", 7);
	}

	char msg[256] = {0};

	// 1. Create a valid important_pjg and snapshot its exact bytes
	REQUIRE(pjglib_convert_file2file(in_jpg.c_str(), important_pjg.c_str(), msg));
	REQUIRE(std::filesystem::exists(important_pjg));
	std::vector<unsigned char> expected_pjg_bytes;
	{
		std::ifstream ifs(important_pjg, std::ios::binary);
		expected_pjg_bytes.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	}
	REQUIRE(expected_pjg_bytes.size() > 10);

	// 2. Bug #9 regression: converting a warning JPEG or corrupt JPEG onto important_pjg MUST leave important_pjg 100% intact!
	REQUIRE(!pjglib_convert_file2file(warn_jpg_path.c_str(), important_pjg.c_str(), msg));
	REQUIRE(std::filesystem::exists(important_pjg));
	{
		std::ifstream ifs(important_pjg, std::ios::binary);
		const std::vector<unsigned char> after_warn((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		REQUIRE(after_warn == expected_pjg_bytes);
	}

	REQUIRE(!pjglib_convert_file2file(corrupt_jpg_path.c_str(), important_pjg.c_str(), msg));
	REQUIRE(std::filesystem::exists(important_pjg));
	{
		std::ifstream ifs(important_pjg, std::ios::binary);
		const std::vector<unsigned char> after_corrupt((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		REQUIRE(after_corrupt == expected_pjg_bytes);
	}

	// 3. Missing input and self-clobber also leave all existing files intact
	const auto missing_in = (tmp_dir.path() / "pjglib_definitely_missing_input.jpg").string();
	REQUIRE(!pjglib_convert_file2file(missing_in.c_str(), untouched_out.c_str(), msg));
	REQUIRE(!std::filesystem::exists(untouched_out));
	REQUIRE(!pjglib_convert_stream2stream(msg));
	REQUIRE(std::filesystem::exists(important_pjg));

	REQUIRE(!pjglib_convert_file2file(in_jpg.c_str(), in_jpg.c_str(), msg));
	REQUIRE_EQ(std::filesystem::file_size(in_jpg), jpg.size());

	REQUIRE(!pjglib_convert_file2file(bad_in.c_str(), untouched_out.c_str(), msg));
	REQUIRE(!std::filesystem::exists(untouched_out));
	REQUIRE(!pjglib_convert_file2file(bad_in.c_str(), pre_existing.c_str(), msg));
	REQUIRE_EQ(std::filesystem::file_size(pre_existing), std::uintmax_t(7));

	pjglib_init_streams(const_cast<unsigned char*>(jpg.data()), 1, static_cast<int>(jpg.size()), nullptr, 1);
	REQUIRE(!pjglib_convert_stream2stream(msg));
}

TEST_CASE("library: file2file supports special targets (/dev/null, FIFO), writes through symlinks, and preserves file permissions") {
	pjg_test::TempTestDir tmp_dir("pjg_ut_special");
	const auto jpg = make_minimal_jpeg();
	const auto in_jpg = (tmp_dir.path() / "src.jpg").string();
	{
		std::ofstream ofs(in_jpg, std::ios::binary);
		ofs.write(reinterpret_cast<const char*>(jpg.data()), static_cast<std::streamsize>(jpg.size()));
	}

	char msg[256] = {0};

#if defined(_WIN32) || defined(WIN32)
	REQUIRE(pjglib_convert_file2file(in_jpg.c_str(), "NUL", msg));
#else
	// 1. Direct write to character device /dev/null
	REQUIRE(pjglib_convert_file2file(in_jpg.c_str(), "/dev/null", msg));

	// 2. Direct write to a POSIX FIFO named pipe without replacing the FIFO inode!
	const auto fifo_path = (tmp_dir.path() / "out_pipe.pjg").string();
	if (::mkfifo(fifo_path.c_str(), 0600) == 0) {
		std::vector<unsigned char> piped_pjg;
		std::thread reader([&]() {
			std::ifstream ifs(fifo_path, std::ios::binary);
			piped_pjg.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		});
		REQUIRE(pjglib_convert_file2file(in_jpg.c_str(), fifo_path.c_str(), msg));
		reader.join();

		std::error_code ec;
		REQUIRE(std::filesystem::is_fifo(fifo_path, ec));
		REQUIRE(piped_pjg.size() > 10);

		unsigned char* rt_buf = nullptr;
		unsigned int rt_size = 0;
		pjglib_init_streams(piped_pjg.data(), 1, static_cast<int>(piped_pjg.size()), nullptr, 1);
		REQUIRE(pjglib_convert_stream2mem(&rt_buf, &rt_size, msg));
		REQUIRE_EQ(std::size_t(rt_size), jpg.size());
		REQUIRE(std::memcmp(rt_buf, jpg.data(), jpg.size()) == 0);
		pjglib_free(rt_buf);
	}

	// 3. Symlink write-through + 0600 private file permission preservation on overwrite
	const auto real_dir = tmp_dir.path() / "real_sub";
	std::filesystem::create_directories(real_dir);
	const auto real_pjg = (real_dir / "secret.pjg").string();
	{
		std::ofstream ofs(real_pjg, std::ios::binary);
		ofs.write("OLD", 3);
	}
	REQUIRE_EQ(::chmod(real_pjg.c_str(), 0600), 0);

	const auto link_pjg = (tmp_dir.path() / "link_to_secret.pjg").string();
	std::error_code sym_ec;
	std::filesystem::create_symlink(real_pjg, link_pjg, sym_ec);
	REQUIRE(!sym_ec);

	// Self-clobber through a symlink pointing at the input file must still be refused!
	const auto link_to_in = (tmp_dir.path() / "link_to_src.jpg").string();
	std::filesystem::create_symlink(in_jpg, link_to_in, sym_ec);
	REQUIRE(!sym_ec);
	REQUIRE(!pjglib_convert_file2file(in_jpg.c_str(), link_to_in.c_str(), msg));
	REQUIRE_EQ(std::filesystem::file_size(in_jpg), jpg.size());

	// Overwriting link_pjg must keep link_pjg as a symlink, update real_pjg, and preserve 0600 mode!
	REQUIRE(pjglib_convert_file2file(in_jpg.c_str(), link_pjg.c_str(), msg));
	REQUIRE(std::filesystem::is_symlink(link_pjg));

	struct stat st{};
	REQUIRE_EQ(::stat(real_pjg.c_str(), &st), 0);
	REQUIRE_EQ(static_cast<unsigned int>(st.st_mode & 0777u), 0600u);

	const auto rt_jpg = (tmp_dir.path() / "roundtrip.jpg").string();
	REQUIRE(pjglib_convert_file2file(real_pjg.c_str(), rt_jpg.c_str(), msg));
	{
		std::ifstream ifs(rt_jpg, std::ios::binary);
		const std::vector<unsigned char> got((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		REQUIRE(got == jpg);
	}
#endif
}

