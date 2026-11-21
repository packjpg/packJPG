#include "test_framework.h"

#include "packjpglib.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

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

TEST_CASE("library: version and short name queries") {
	const char* ver = pjglib_version_info();
	const char* name = pjglib_short_name();
	REQUIRE(ver != nullptr);
	REQUIRE(name != nullptr);
	REQUIRE(std::strstr(ver, "packJPG") != nullptr);
	REQUIRE(std::strstr(name, "packJPG") != nullptr);
}

TEST_CASE("library: memory-to-memory compress and decompress round-trip") {
	auto jpg = make_minimal_jpeg();
	char msg[256] = {0};

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

	std::free(pjg_buf);
	std::free(rt_buf);
}

TEST_CASE("library: file-to-file round-trip and recovery after invalid input") {
	const auto dir = std::filesystem::temp_directory_path();
	const auto in_jpg = (dir / "pjglib_test_in.jpg").string();
	const auto out_pjg = (dir / "pjglib_test_out.pjg").string();
	const auto rt_jpg = (dir / "pjglib_test_rt.jpg").string();

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

	std::filesystem::remove(in_jpg);
	std::filesystem::remove(out_pjg);
	std::filesystem::remove(rt_jpg);
}
