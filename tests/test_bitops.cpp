#include "test_framework.h"

#include "bitops.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>

TEST_CASE("bitops: BitWriter and BitReader round-trip across bit widths") {
	BitWriter bw(1);
	bw.write_bit(1);
	bw.write_bit(0);
	bw.write_u16(0x05, 3);     // 101
	bw.write_u16(0xABCD, 16);  // cross-byte 16-bit write
	bw.write_u16(0x007F, 7);
	bw.write_bit(0);
	bw.write_bit(1);           // leaves partial byte -> padded with 1s

	auto bytes = bw.get_bytes();
	REQUIRE(!bytes.empty());

	BitReader br(bytes.data(), static_cast<int>(bytes.size()));
	REQUIRE_EQ(int(br.read_bit()), 1);
	REQUIRE_EQ(int(br.read_bit()), 0);
	REQUIRE_EQ(br.read(3), 0x05u);
	REQUIRE_EQ(br.read(16), 0xABCDu);
	REQUIRE_EQ(br.read(7), 0x007Fu);
	REQUIRE_EQ(int(br.read_bit()), 0);
	REQUIRE_EQ(int(br.read_bit()), 1);
	REQUIRE_EQ(br.unpad(0), 1);
	REQUIRE(br.eof());
	REQUIRE_EQ(br.unpad(-1), -1);

	std::vector<unsigned char> aligned = {0xAB};
	BitReader br_aligned(aligned.data(), static_cast<int>(aligned.size()));
	REQUIRE_EQ(br_aligned.unpad(-1), -1);
	REQUIRE_EQ(br_aligned.read(8), 0xABu);
	REQUIRE_EQ(br_aligned.unpad(-1), -1);
}

TEST_CASE("bitops: BitReader setpos, rewind_bits, and past-EOF accounting") {
	std::vector<unsigned char> buf = {0xF0, 0x5A, 0xCC};
	BitReader br(buf.data(), static_cast<int>(buf.size()));

	REQUIRE_EQ(br.read(4), 0x0Fu);
	REQUIRE_EQ(br.read(8), 0x05u);
	br.rewind_bits(6);
	REQUIRE_EQ(br.read(6), 0x05u);

	br.setpos(2, 4);
	REQUIRE_EQ(br.read(4), 0x0Cu);
	REQUIRE(br.eof());
	REQUIRE_EQ(br.peof(), 0);

	// Past-EOF reads accumulate peof bits and return zero bits
	REQUIRE_EQ(br.read(5), 0u);
	REQUIRE_EQ(br.peof(), 5);
	br.rewind_bits(3);
	REQUIRE_EQ(br.peof(), 2);
	br.rewind_bits(4);
	REQUIRE(!br.eof());
	REQUIRE_EQ(br.read(2), 0x0u);
	// Exact byte-boundary rewind_bits must keep cbit == 8 and cbyte == 1 (never cbit == 0)
	std::vector<unsigned char> boundary_buf = {0xAA, 0x55, 0xF0, 0x0F};
	BitReader br2(boundary_buf.data(), static_cast<int>(boundary_buf.size()));
	REQUIRE_EQ(br2.read(12), 0xAA5u);
	br2.rewind_bits(4);
	REQUIRE_EQ(br2.getpos(), 1);
	REQUIRE_EQ(br2.getbitp(), 8);
	REQUIRE_EQ(br2.peek_9_fast(), (0x55u << 1) | 1u);
	REQUIRE_EQ(int(br2.read_bit()), 0);
	REQUIRE_EQ(int(br2.read_bit()), 1);
	REQUIRE_EQ(br2.read(6), 0x15u);

	REQUIRE_EQ(bit_length_u32(0u), 0);
	REQUIRE_EQ(bit_length_u32(0x80000000u), 32);
	REQUIRE_EQ(bit_length_u32(0xFFFFFFFFu), 32);
	REQUIRE_EQ(fdiv2(-2147483647 - 1, 1), -1073741824);
}

TEST_CASE("bitops: BitWriter get_c_bytes matches std::free contract") {
	BitWriter bw(0);
	bw.write_u16(0x1234, 16);
	bw.write_u16(0x5, 4);
	unsigned char* raw = bw.get_c_bytes();
	REQUIRE(raw != nullptr);
	REQUIRE_EQ(bw.num_bytes_written(), std::size_t(3));
	REQUIRE_EQ(int(raw[0]), 0x12);
	REQUIRE_EQ(int(raw[1]), 0x34);
	REQUIRE_EQ(int(raw[2]), 0x50);
	std::free(raw);
}

TEST_CASE("bitops: MemoryReader and MemoryWriter operations") {
	MemoryWriter mw;
	const std::vector<std::uint8_t> chunk = {10, 20, 30, 40, 50};
	mw.write(chunk);
	mw.write_byte(60);
	mw.write(std::array<std::uint8_t, 2>{70, 80});
	REQUIRE_EQ(mw.num_bytes_written(), std::size_t(8));
	REQUIRE(!mw.error());

	MemoryReader mr(mw.get_data());
	REQUIRE_EQ(mr.get_size(), std::size_t(8));
	REQUIRE_EQ(int(mr.read_byte()), 10);
	mr.skip(2);
	REQUIRE_EQ(mr.num_bytes_read(), std::size_t(3));
	REQUIRE_EQ(int(mr.read_byte()), 40);
	mr.rewind_bytes(3);
	REQUIRE_EQ(int(mr.read_byte()), 20);
	mr.rewind();
	REQUIRE_EQ(mr.num_bytes_read(), std::size_t(0));

	std::vector<std::uint8_t> out;
	REQUIRE_EQ(mr.read(out, 100, 2), std::size_t(8));
	REQUIRE_EQ(out.size(), std::size_t(10));
	REQUIRE_EQ(int(out[2]), 10);
	REQUIRE_EQ(int(out[9]), 80);
	REQUIRE(mr.end_of_reader());
	REQUIRE_THROWS(mr.read_byte());
}

TEST_CASE("bitops: FileWriter and FileReader >32KB round-trip and truncating reset") {
	const auto tmp_path = (std::filesystem::temp_directory_path() / "packjpg_unit_fileio.bin").string();
	std::vector<std::uint8_t> payload(40000);
	for (std::size_t i = 0; i < payload.size(); ++i) {
		payload[i] = static_cast<std::uint8_t>((i * 37 + 13) & 0xFF);
	}

	{
		FileWriter fw(tmp_path);
		fw.write(payload);
		REQUIRE_EQ(fw.num_bytes_written(), payload.size());
		REQUIRE(fw.get_data() == payload);
		fw.reset();
		REQUIRE_EQ(fw.num_bytes_written(), std::size_t(0));
		fw.write_byte(0xAA);
		fw.write_byte(0xBB);
		REQUIRE_EQ(fw.num_bytes_written(), std::size_t(2));
		REQUIRE(!fw.error());
	}

	{
		FileReader fr(tmp_path);
		REQUIRE_EQ(fr.get_size(), std::size_t(2));
		REQUIRE(fr.get_data() == (std::vector<std::uint8_t>{0xAA, 0xBB}));
	}

	std::filesystem::remove(tmp_path);
	REQUIRE_THROWS(FileReader(tmp_path + ".nonexistent"));
	REQUIRE_THROWS(FileReader(std::filesystem::temp_directory_path().string()));
}

TEST_CASE("bitops: zero-bit write_u16, MemoryReader null guards, and FILE* StreamReader/Writer") {
	BitWriter bw(0);
	bw.write_u16(0xFFFF, 0);
	REQUIRE_EQ(bw.num_bytes_written(), std::size_t(0));
	bw.write_u16(0xA5, 8);
	REQUIRE(bw.get_bytes() == std::vector<std::uint8_t>{0xA5});

	MemoryReader empty_mr(nullptr, 0);
	REQUIRE(empty_mr.end_of_reader());
	REQUIRE_EQ(empty_mr.get_size(), std::size_t(0));
	REQUIRE_THROWS(MemoryReader(nullptr, 4));
	REQUIRE_THROWS(StreamReader(nullptr));
	REQUIRE_THROWS(StreamWriter(nullptr));

	const auto stream_path = (std::filesystem::temp_directory_path() / "packjpg_stream_io.bin").string();
	{
		FILE* out = std::fopen(stream_path.c_str(), "wb");
		REQUIRE(out != nullptr);
		{
			StreamWriter sw(out);
			sw.write(std::vector<std::uint8_t>{1, 2});
			REQUIRE(sw.flush());
			sw.write(std::vector<std::uint8_t>{3, 4});
			REQUIRE(sw.flush());
			REQUIRE(!sw.error());
			REQUIRE(sw.get_data() == (std::vector<std::uint8_t>{1, 2, 3, 4}));
		}
		std::fclose(out);
	}
	{
		FILE* in = std::fopen(stream_path.c_str(), "rb");
		REQUIRE(in != nullptr);
		StreamReader sr(in);
		std::fclose(in);
		REQUIRE(sr.get_data() == (std::vector<std::uint8_t>{1, 2, 3, 4}));
	}
	// Verify StreamWriter::reset() discards buffered data so destructor writes nothing
	{
		FILE* out = std::fopen(stream_path.c_str(), "wb");
		REQUIRE(out != nullptr);
		{
			StreamWriter sw(out);
			sw.write(std::vector<std::uint8_t>{9, 9, 9});
			sw.reset();
		}
		std::fclose(out);
		FileReader fr(stream_path);
		REQUIRE_EQ(fr.get_size(), std::size_t(0));
	}
	std::filesystem::remove(stream_path);
}

