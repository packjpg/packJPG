#include "test_framework.h"

#include "bitops.h"
#include "path_utils.h"

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

TEST_CASE("bitops: MemoryReader and MemoryWriter operations and null-pointer guard") {
	MemoryWriter mw;
	mw.write(static_cast<const std::uint8_t*>(nullptr), 0);
	REQUIRE(!mw.error());

	const std::vector<std::uint8_t> chunk = {10, 20, 30, 40, 50};
	mw.write(chunk);
	mw.write_byte(60);
	mw.write(std::array<std::uint8_t, 2>{70, 80});
	REQUIRE_EQ(mw.num_bytes_written(), std::size_t(8));
	REQUIRE(!mw.error());
	REQUIRE_EQ(int(mw.data()[0]), 10);

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

	// Bug #18 regression: non-zero write from nullptr sets error flag instead of UB memcpy(dst, nullptr, n)
	MemoryWriter mw_null;
	mw_null.write(static_cast<const std::uint8_t*>(nullptr), 5);
	REQUIRE(mw_null.error());
	REQUIRE_EQ(mw_null.num_bytes_written(), std::size_t(0));
}

TEST_CASE("bitops: FileWriter and FileReader >32KB round-trip, truncating reset, and UTF-8 paths") {
	pjg_test::TempTestDir tmp_dir("pjg_fileio");
	const auto tmp_path = path_to_utf8(tmp_dir.path() / "packjpg_unit_fileio.bin");
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

	// Bug #10 regression: multi-script UTF-8 filename round-trips cleanly via path_from_utf8 / fopen_utf8
	const std::string utf8_name = path_to_utf8(tmp_dir.path()) + "/unicode_\xCE\xB1\xCE\xB2_\xE7\x8C\xAB.bin";
	{
		FileWriter fw_u8(utf8_name);
		fw_u8.write(std::vector<std::uint8_t>{0x42, 0x99, 0xFE});
		REQUIRE(fw_u8.flush());
		REQUIRE(!fw_u8.error());
	}
	REQUIRE(file_exists(utf8_name));
	{
		FileReader fr_u8(utf8_name);
		REQUIRE(fr_u8.get_data() == (std::vector<std::uint8_t>{0x42, 0x99, 0xFE}));
	}
	REQUIRE(remove_file_utf8(utf8_name));
	REQUIRE(!file_exists(utf8_name));

	REQUIRE_THROWS(FileReader(tmp_path + ".nonexistent"));
	REQUIRE_THROWS(FileReader(path_to_utf8(tmp_dir.path())));
}

TEST_CASE("bitops: zero-bit write_u16, MemoryReader null guards, and StreamWriter closed-FILE* safety") {
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

	pjg_test::TempTestDir tmp_dir("pjg_streamio");
	const auto stream_path = path_to_utf8(tmp_dir.path() / "packjpg_stream_io.bin");
	{
		FILE* out = fopen_utf8(stream_path, "wb");
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
		FILE* in = fopen_utf8(stream_path, "rb");
		REQUIRE(in != nullptr);
		StreamReader sr(in);
		std::fclose(in);
		REQUIRE(sr.get_data() == (std::vector<std::uint8_t>{1, 2, 3, 4}));
	}
	// Verify StreamWriter::reset() discards buffered data so destructor writes nothing
	{
		FILE* out = fopen_utf8(stream_path, "wb");
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
	// Bug #12 regression: unused StreamWriter destroyed AFTER caller closes FILE* must NOT touch closed FILE*!
	{
		FILE* out = fopen_utf8(stream_path, "wb");
		REQUIRE(out != nullptr);
		auto abandoned_sw = std::make_unique<StreamWriter>(out);
		std::fclose(out);
		abandoned_sw.reset();
	}
}

TEST_CASE("path_utils and FileWriter: exclusive create, close/write error state, non-replacing commit, UTF-8 wildcards, and stack-based directory walk") {
	pjg_test::TempTestDir tmp_dir("pjg_ut_p3_path");

	// 1. Exclusive FileWriter creation succeeds on new path and rejects existing path
	const std::string excl_path = path_to_utf8(tmp_dir.path() / "exclusive.bin");
	{
		FileWriter fw(excl_path, /*exclusive=*/true);
		const std::uint8_t payload[] = {1, 2, 3, 4};
		REQUIRE_EQ(fw.write(payload, sizeof(payload)), sizeof(payload));
		REQUIRE(!fw.error());
		REQUIRE_EQ(fw.get_data().size(), sizeof(payload));
		REQUIRE(fw.close());
		REQUIRE(!fw.error());
		REQUIRE_EQ(fw.num_bytes_written(), sizeof(payload));
	}
	bool threw_on_existing = false;
	try {
		FileWriter fw_dup(excl_path, /*exclusive=*/true);
	} catch (const std::runtime_error&) {
		threw_on_existing = true;
	}
	REQUIRE(threw_on_existing);

	// 2. Null-pointer non-zero writes set sticky error on FileWriter and StreamWriter
	const std::string null_err_path = path_to_utf8(tmp_dir.path() / "null_err.bin");
	{
		FileWriter fw(null_err_path);
		REQUIRE_EQ(fw.write(nullptr, 5), std::size_t(0));
		REQUIRE(fw.error());
		REQUIRE(!fw.close());
	}
	{
		FILE* fp = fopen_utf8(null_err_path, "wb");
		REQUIRE(fp != nullptr);
		StreamWriter sw(fp);
		REQUIRE_EQ(sw.write(nullptr, 5), std::size_t(0));
		REQUIRE(sw.error());
		std::fclose(fp);
	}

	// 3. Non-replacing commit_staging_file returns TargetExists without clobbering existing file
	const std::string keep_path = path_to_utf8(tmp_dir.path() / "keep.pjg");
	const std::string stage_path = path_to_utf8(tmp_dir.path() / "stage.part");
	{
		FileWriter fw_keep(keep_path);
		const std::uint8_t orig[] = {'K', 'E', 'E', 'P'};
		fw_keep.write(orig, sizeof(orig));
		REQUIRE(fw_keep.close());

		FileWriter fw_stage(stage_path);
		const std::uint8_t repl[] = {'N', 'E', 'W'};
		fw_stage.write(repl, sizeof(repl));
		REQUIRE(fw_stage.close());
	}
	std::string commit_err;
	REQUIRE(commit_staging_file(stage_path, keep_path, /*allow_replace=*/false, commit_err) == CommitStatus::TargetExists);
	{
		FileReader fr_keep(keep_path);
		const auto keep_bytes = fr_keep.get_data();
		REQUIRE_EQ(keep_bytes.size(), std::size_t(4));
		REQUIRE_EQ(keep_bytes[0], std::uint8_t('K'));
	}
	const std::string fresh_dst = path_to_utf8(tmp_dir.path() / "keep_.pjg");
	REQUIRE(commit_staging_file(stage_path, fresh_dst, /*allow_replace=*/false, commit_err) == CommitStatus::Ok);
	REQUIRE(file_exists(fresh_dst));
	REQUIRE(!file_exists(stage_path));

	// 4. UTF-8 code-point '?' and '*' wildcard matching
	REQUIRE(wildcard_match("b?.jpg", "bx.jpg", false));
	REQUIRE(wildcard_match("b?.jpg", "b\xC3\xA9.jpg", false));         // 2-byte UTF-8 'é'
	REQUIRE(wildcard_match("b?.jpg", "b\xE7\x8C\xAB.jpg", false));     // 3-byte UTF-8 '猫'
	REQUIRE(wildcard_match("b?.jpg", "b\xF0\x9F\x98\x80.jpg", false)); // 4-byte UTF-8
	REQUIRE(!wildcard_match("b?.jpg", "b\xC3\xA9x.jpg", false));
	REQUIRE(wildcard_match("*_\xE7\x8C\xAB_?.pjg", "photo_\xE7\x8C\xAB_\xC3\xA9.pjg", false));

	// 5. Stack-based recursive directory walk + directory-symlink target equivalence
	const auto tree_root = tmp_dir.path() / "tree";
	const auto sub_a = tree_root / "a" / "deep";
	const auto sub_b = tree_root / "b";
	std::filesystem::create_directories(sub_a);
	std::filesystem::create_directories(sub_b);
	{
		FileWriter(path_to_utf8(tree_root / "top.jpg")).close();
		FileWriter(path_to_utf8(sub_a / "nested.jpeg")).close();
		FileWriter(path_to_utf8(sub_b / "other.pjg")).close();
	}
#if !defined(_WIN32) && !defined(WIN32)
	const auto alias_dir = tmp_dir.path() / "alias_b";
	std::error_code sym_ec;
	std::filesystem::create_directory_symlink(sub_b, alias_dir, sym_ec);
	if (!sym_ec) {
		// Non-existent leaf inside symlinked directory must be recognized as the same target slot!
		REQUIRE(paths_refer_to_same_target_location(
			path_to_utf8(sub_b / "future_out.pjg"),
			path_to_utf8(alias_dir / "future_out.pjg")));
	}
	if (::geteuid() != 0) {
		const auto locked_sub = tree_root / "a" / "locked";
		std::filesystem::create_directories(locked_sub);
		::chmod(locked_sub.c_str(), 0000);
		bool had_glob = false;
		const auto items = expand_cli_inputs({path_to_utf8(tree_root)}, /*recursive=*/true, {}, had_glob);
		::chmod(locked_sub.c_str(), 0700);
		REQUIRE(had_glob);
		REQUIRE_EQ(items.size(), std::size_t(3));
	}
#else
	bool had_glob = false;
	const auto items = expand_cli_inputs({path_to_utf8(tree_root)}, /*recursive=*/true, {}, had_glob);
	REQUIRE(had_glob);
	REQUIRE_EQ(items.size(), std::size_t(3));
#endif

	// 6. Multi-part extension unique filename keeps underscore on base stem ("photo_.cmp0.pgm")
	const auto pgm_first = path_to_utf8(tmp_dir.path() / "photo.cmp0.pgm");
	FileWriter(pgm_first).close();
	REQUIRE_EQ(
		make_unique_filename(path_to_utf8(tmp_dir.path() / "photo.jpg"), "cmp0.pgm"),
		path_to_utf8(tmp_dir.path() / "photo_.cmp0.pgm"));

	// 7. can_overwrite_or_create_target reports user-facing display_name when target is a directory
	std::string dir_err;
	const std::string display_link = "user_link.pjg";
	REQUIRE(!can_overwrite_or_create_target(path_to_utf8(sub_b), dir_err, &display_link));
	REQUIRE(dir_err.find(display_link) != std::string::npos);
}

TEST_CASE("fourth-pass bitops and path_utils: owned StreamWriter, non-regular FileWriter reset/count, symlinked-dir '../' symlink, nonfatal noreplace errno, and long paths") {
	pjg_test::TempTestDir tmp_dir("pjg_ut_p4_io");

	// 1. Bug #8: StreamWriter(fp, own_stream=true) buffers get_data(), flushes+closes on close(), and discards cleanly on reset()
	const std::string owned_ok = path_to_utf8(tmp_dir.path() / "owned_ok.bin");
	{
		FILE* fp = fopen_utf8(owned_ok, "wb");
		REQUIRE(fp != nullptr);
		StreamWriter sw(fp, /*own_stream=*/true);
		sw.write(std::vector<std::uint8_t>{0x11, 0x22, 0x33});
		REQUIRE_EQ(sw.num_bytes_written(), std::size_t(3));
		REQUIRE(sw.get_data() == (std::vector<std::uint8_t>{0x11, 0x22, 0x33}));
		REQUIRE(sw.close());
		REQUIRE(!sw.error());
		REQUIRE(!sw.write_byte(0x44));
		REQUIRE(sw.error());
	}
	REQUIRE(FileReader(owned_ok).get_data() == (std::vector<std::uint8_t>{0x11, 0x22, 0x33}));

	const std::string owned_abort = path_to_utf8(tmp_dir.path() / "owned_abort.bin");
	{
		FILE* fp = fopen_utf8(owned_abort, "wb");
		REQUIRE(fp != nullptr);
		StreamWriter sw(fp, /*own_stream=*/true);
		sw.write(std::vector<std::uint8_t>{0xDE, 0xAD, 0xBE, 0xEF});
		sw.reset();
	}
	REQUIRE_EQ(FileReader(owned_abort).get_size(), std::size_t(0));

	// 2. Bug #8 & #15: FileWriter on /dev/null or NUL tracks exact bytes_written_ and survives reset()
#if defined(_WIN32) || defined(WIN32)
	const std::string null_dev = "NUL";
	REQUIRE(is_non_regular_stream_target("NUL"));
	REQUIRE(is_non_regular_stream_target(path_to_utf8(tmp_dir.path() / "NUL")));
	REQUIRE(is_non_regular_stream_target("\\\\.\\NUL"));
#else
	const std::string null_dev = "/dev/null";
	REQUIRE(is_non_regular_stream_target("/dev/null"));
#endif
	{
		FileWriter fw_null(null_dev);
		fw_null.write(std::vector<std::uint8_t>{1, 2, 3, 4, 5});
		REQUIRE_EQ(fw_null.num_bytes_written(), std::size_t(5));
		REQUIRE(!fw_null.error());
		fw_null.reset();
		REQUIRE(!fw_null.error());
		REQUIRE_EQ(fw_null.num_bytes_written(), std::size_t(0));
		fw_null.write_byte(0x7F);
		REQUIRE_EQ(fw_null.num_bytes_written(), std::size_t(1));
		REQUIRE(fw_null.close());
	}

	// 3. Bug #9: Symlinked parent directory + "../target.pjg" relative symlink resolves relative to canonical parent!
	{
		const auto real_parent = tmp_dir.path() / "real_parent";
		const auto real_sub    = real_parent / "sub";
		const auto sym_dir     = tmp_dir.path() / "sym_sub";
		std::filesystem::create_directories(real_sub);
		std::error_code ec;
		std::filesystem::create_directory_symlink(real_sub, sym_dir, ec);
		if (!ec) {
			const auto link_in_sym = sym_dir / "out.pjg";
			std::filesystem::create_symlink(std::filesystem::path("..") / "target.pjg", link_in_sym, ec);
			if (!ec) {
				std::string resolved;
				std::string err;
				REQUIRE(resolve_overwrite_target_utf8(path_to_utf8(link_in_sym), resolved, err));
				FileWriter(resolved).write(std::vector<std::uint8_t>{'O', 'K'});
				REQUIRE(file_exists(path_to_utf8(real_parent / "target.pjg")));
				REQUIRE(!file_exists(path_to_utf8(tmp_dir.path() / "target.pjg")));
			}
		}
	}

	// 4. Bug #10: Non-overwrite fallback errno predicate accepts exFAT/FAT32/SMB ENOTSUP & EACCES, rejects ENOSPC/EIO
#if !defined(_WIN32) && !defined(WIN32)
	REQUIRE(is_nonfatal_noreplace_errno(EPERM));
	REQUIRE(is_nonfatal_noreplace_errno(ENOSYS));
	REQUIRE(is_nonfatal_noreplace_errno(EINVAL));
	REQUIRE(is_nonfatal_noreplace_errno(EACCES));
#ifdef EOPNOTSUPP
	REQUIRE(is_nonfatal_noreplace_errno(EOPNOTSUPP));
#endif
#ifdef ENOTSUP
	REQUIRE(is_nonfatal_noreplace_errno(ENOTSUP));
#endif
	REQUIRE(!is_nonfatal_noreplace_errno(ENOSPC));
	REQUIRE(!is_nonfatal_noreplace_errno(EIO));
#endif

	// 5. Bug #15: >= 245-char path round-trips through ensure_parent_directory, FileWriter, FileWriter::get_data, and FileReader
	{
		std::filesystem::path deep = tmp_dir.path();
		while (path_to_utf8(deep).size() < 245) {
			deep /= std::string(28, 'p');
		}
		const std::string long_file = path_to_utf8(deep / "long_leaf_\xCE\xBB.bin");
		REQUIRE(ensure_parent_directory(long_file));
		{
			FileWriter fw_long(long_file);
			fw_long.write(std::vector<std::uint8_t>{0xCA, 0xFE, 0xBA, 0xBE});
			REQUIRE(fw_long.get_data() == (std::vector<std::uint8_t>{0xCA, 0xFE, 0xBA, 0xBE}));
			REQUIRE(fw_long.close());
		}
		REQUIRE(FileReader(long_file).get_data() == (std::vector<std::uint8_t>{0xCA, 0xFE, 0xBA, 0xBE}));
		REQUIRE(remove_file_utf8(long_file));
	}

	// 6. Bug #9: make_output_base handles trailing slash on explicit input path cleanly
	REQUIRE_EQ(make_output_base("photos/sample.jpg", "out_dir/", ""), std::string("out_dir/sample.jpg"));

	// 7. BitReader::setpos negative/past-EOF clamps, Reader::get_c_data, StreamWriter ~StreamWriter flush, and FileReader/StreamReader methods
	{
		const std::uint8_t raw[2] = {0xA5, 0x5A};
		BitReader br(raw, 2);
		br.setpos(-3, 4);
		REQUIRE_EQ(br.getpos(), 0);
		REQUIRE_EQ(br.getbitp(), 8);
		br.setpos(5, 3);
		REQUIRE(br.eof());
		REQUIRE_EQ(br.peof(), std::int64_t((5 - 2) * 8 + (8 - 3)));

		const std::string sr_path = path_to_utf8(tmp_dir.path() / "stream_fwd.bin");
		{
			FILE* fp = fopen_utf8(sr_path, "wb");
			REQUIRE(fp != nullptr);
			{
				StreamWriter sw_dtor(fp, /*own_stream=*/true);
				sw_dtor.write(std::vector<std::uint8_t>{0x10, 0x20, 0x30, 0x40});
			} // ~StreamWriter auto-flushes and closes fp!
		}
		FileReader fr(sr_path);
		unsigned char* c_copy = fr.get_c_data();
		REQUIRE(c_copy != nullptr);
		REQUIRE_EQ(c_copy[0], 0x10);
		REQUIRE_EQ(c_copy[3], 0x40);
		std::free(c_copy);
		REQUIRE_EQ(fr.read_byte(), 0x10);
		std::vector<std::uint8_t> into_vec;
		REQUIRE_EQ(fr.read(into_vec, 2, 0), std::size_t(2));
		fr.rewind_bytes(1);
		REQUIRE_EQ(fr.num_bytes_read(), std::size_t(2));
		REQUIRE(!fr.end_of_reader());

		FILE* rfp = fopen_utf8(sr_path, "rb");
		REQUIRE(rfp != nullptr);
		StreamReader sr(rfp);
		REQUIRE_EQ(sr.read_byte(), 0x10);
		std::vector<std::uint8_t> sr_into;
		REQUIRE_EQ(sr.read(sr_into, 2, 0), std::size_t(2));
		sr.rewind_bytes(1);
		REQUIRE_EQ(sr.num_bytes_read(), std::size_t(2));
		REQUIRE(!sr.end_of_reader());
		std::fclose(rfp);
	}
}
