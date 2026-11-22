#include "test_framework.h"

#include "aricoder.h"
#include "bitops.h"

#include <cstdint>
#include <vector>

namespace {

void encode_with_model_s(ArithmeticEncoder& enc, model_s& model, int value) {
	symbol s{};
	int esc = 0;
	do {
		esc = model.convert_int_to_symbol(value, &s);
		enc.encode(&s);
	} while (esc);
	model.update_model(value);
}

int decode_with_model_s(ArithmeticDecoder& dec, model_s& model) {
	symbol s{};
	int value = 0;
	do {
		model.get_symbol_scale(&s);
		const uint32_t count = dec.decode_count(&s);
		value = model.convert_symbol_to_int(count, &s);
		dec.decode(&s);
	} while (value == int(ESCAPE_SYMBOL));
	model.update_model(value);
	return value;
}

void encode_with_model_b(ArithmeticEncoder& enc, model_b& model, int bit) {
	symbol s{};
	model.convert_int_to_symbol(bit, &s);
	enc.encode(&s);
	model.update_model(bit);
}

int decode_with_model_b(ArithmeticDecoder& dec, model_b& model) {
	symbol s{};
	model.get_symbol_scale(&s);
	const uint32_t count = dec.decode_count(&s);
	const int bit = model.convert_symbol_to_int(count, &s);
	dec.decode(&s);
	model.update_model(bit);
	return bit;
}

} // namespace

TEST_CASE("aricoder: ArithmeticBitWriter run-length bit packing") {
	ArithmeticBitWriter abw;
	abw.write_bit<1>();
	abw.write_n_zero_bits(19);
	abw.write_n_one_bits(13);
	abw.pad();
	const auto data = abw.get_data();
	REQUIRE_EQ(data.size(), std::size_t(5));
	// Bit pattern: 1, 19x 0, 13x 1, 7x 0 -> 10000000 00000000 00001111 11111111 10000000
	REQUIRE_EQ(int(data[0]), 0x80);
	REQUIRE_EQ(int(data[1]), 0x00);
	REQUIRE_EQ(int(data[2]), 0x0F);
	REQUIRE_EQ(int(data[3]), 0xFF);
	REQUIRE_EQ(int(data[4]), 0x80);
}

TEST_CASE("aricoder: model_s multi-order context, exclusions, and rescaling round-trip") {
	std::vector<int> seq;
	seq.reserve(1500);
	for (int i = 0; i < 1500; ++i) {
		// Mix skewed low values (triggers E3 scaling and table rescaling) with occasional larger symbols
		seq.push_back((i % 17 == 0) ? (i % 15) : (i % 4));
	}

	MemoryWriter mw;
	{
		ArithmeticEncoder enc(mw);
		model_s mod(16, 8, 2, 32); // small c_lim=32 forces frequent rescale_table() calls
		int prev1 = 0;
		int prev2 = 0;
		for (int v : seq) {
			mod.shift_context(prev1 & 7);
			mod.shift_context(prev2 & 7);
			mod.exclude_symbols(14);
			encode_with_model_s(enc, mod, v);
			if (v == 11) {
				mod.flush_model();
			}
			prev2 = prev1;
			prev1 = v;
		}
		enc.finalize();
	}

	MemoryReader mr(mw.get_data());
	ArithmeticDecoder dec(mr);
	model_s mod(16, 8, 2, 32);
	int prev1 = 0;
	int prev2 = 0;
	for (std::size_t idx = 0; idx < seq.size(); ++idx) {
		mod.shift_context(prev1 & 7);
		mod.shift_context(prev2 & 7);
		mod.exclude_symbols(14);
		const int got = decode_with_model_s(dec, mod);
		REQUIRE_EQ(got, seq[idx]);
		if (got == 11) {
			mod.flush_model();
		}
		prev2 = prev1;
		prev1 = got;
	}
}

TEST_CASE("aricoder: model_b binary context coding round-trip") {
	std::vector<int> bits;
	bits.reserve(2000);
	for (int i = 0; i < 2000; ++i) {
		bits.push_back(((i * 29 + (i >> 3)) % 7 == 0) ? 1 : 0);
	}

	MemoryWriter mw;
	{
		ArithmeticEncoder enc(mw);
		model_b mod(16, 2, 64);
		for (std::size_t i = 0; i < bits.size(); ++i) {
			mod.shift_context(int(i & 15));
			mod.shift_context(int((i >> 2) & 15));
			encode_with_model_b(enc, mod, bits[i]);
			if (i == 999) {
				mod.flush_model();
			}
		}
		enc.finalize();
	}

	MemoryReader mr(mw.get_data());
	ArithmeticDecoder dec(mr);
	model_b mod(16, 2, 64);
	for (std::size_t i = 0; i < bits.size(); ++i) {
		mod.shift_context(int(i & 15));
		mod.shift_context(int((i >> 2) & 15));
		REQUIRE_EQ(decode_with_model_b(dec, mod), bits[i]);
		if (i == 999) {
			mod.flush_model();
		}
	}
}

TEST_CASE("aricoder: rejects out-of-bounds context shifts and zero-scale decode") {
	model_s ms(8, 4, 1, 256);
	REQUIRE_THROWS(ms.shift_context(4));

	model_b mb(4, 1, 256);
	REQUIRE_THROWS(mb.shift_context(4));

	const std::vector<std::uint8_t> raw = {0x00, 0x00, 0x00, 0x00};
	MemoryReader mr(raw);
	ArithmeticDecoder dec(mr);
	symbol bad{0, 0, 0};
	REQUIRE_THROWS(dec.decode_count(&bad));
}

TEST_CASE("aricoder: large-alphabet model_s (max_s > 64) and >64KB streaming flush") {
	// Exercises vector-backed scoreboard (max_s=257) and >64KB ArithmeticBitWriter sink flushing
	constexpr std::size_t kCount = 90000;
	std::vector<int> data;
	data.reserve(kCount);
	for (std::size_t i = 0; i < kCount; ++i) {
		const std::uint32_t mod_bound = ((i & 63u) == 0u) ? 256u : 257u;
		data.push_back(static_cast<int>((i * 131u + (i >> 5)) % mod_bound));
	}

	MemoryWriter mw;
	{
		ArithmeticEncoder enc(mw);
		model_s mod(257, 4, 1, 255);
		for (std::size_t i = 0; i < kCount; ++i) {
			mod.shift_context(static_cast<int>(i & 3u));
			if ((i & 63u) == 0u) {
				mod.exclude_symbols(255);
			}
			encode_with_model_s(enc, mod, data[i]);
		}
		enc.finalize();
	}
	REQUIRE(mw.num_bytes_written() > ArithmeticBitWriter::kFlushThreshold);

	MemoryReader mr(mw.take_data());
	ArithmeticDecoder dec(mr);
	model_s mod(257, 4, 1, 255);
	for (std::size_t i = 0; i < kCount; ++i) {
		mod.shift_context(static_cast<int>(i & 3u));
		if ((i & 63u) == 0u) {
			mod.exclude_symbols(255);
		}
		REQUIRE_EQ(decode_with_model_s(dec, mod), data[i]);
	}
}

TEST_CASE("aricoder: >1200 pending E3 underflow bits finalized with clow >= 0.25 decode cleanly") {
	constexpr int kE3Count = 1200;
	symbol straddle{1, 3, 4};  // [0.25, 0.75) -> increments nrbits without emitting bits
	symbol stay_high{3, 7, 8}; // [0.375, 0.875) -> stops E3 with clow >= 0.25 and nrbits == 1200

	MemoryWriter mw;
	{
		ArithmeticEncoder enc(mw);
		for (int i = 0; i < kE3Count; ++i) {
			enc.encode(&straddle);
		}
		enc.encode(&stay_high);
		enc.finalize();
	}
	// Entire 1201-symbol stream compresses to a single byte (0x80) relying on synthesized EOF zero bytes!
	REQUIRE_EQ(mw.num_bytes_written(), std::size_t(1));

	MemoryReader mr(mw.take_data());
	ArithmeticDecoder dec(mr);
	for (int i = 0; i < kE3Count; ++i) {
		symbol q{0, 0, 4};
		const unsigned int cnt = dec.decode_count(&q);
		REQUIRE(cnt >= 1u && cnt < 3u);
		q.low_count  = 1;
		q.high_count = 3;
		dec.decode(&q);
	}
	symbol q_end{0, 0, 8};
	const unsigned int end_cnt = dec.decode_count(&q_end);
	REQUIRE(end_cnt >= 3u && end_cnt < 7u);
	q_end.low_count  = 3;
	q_end.high_count = 7;
	dec.decode(&q_end);
}

