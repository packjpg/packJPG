#ifndef JPGENCODER_H
#define JPGENCODER_H

#include <cstdint>
#include <memory>
#include <vector>

#include "bitops.h"
#include "huffcodes.h"
#include "segment.h"
#include "writer.h"

class JpgEncoder {
public:
	// JPEG encoding routine.
	void recode(const std::vector<Segment>& segments);
	// Merges header & image data to jpeg.
	void merge(const std::unique_ptr<Writer>& str_out, const std::vector<Segment>& segments);

private:
	// Sequential block encoding routine.
	int block_seq(const std::unique_ptr<BitWriter>& huffw, const HuffCodes& dctbl, const HuffCodes& actbl, const std::array<std::int16_t, 64>& block);
	// Progressive DC encoding routine.
	void dc_prg_fs(const std::unique_ptr<BitWriter>& huffw, const HuffCodes& dctbl, const std::array<std::int16_t, 64>& block);
	// Progressive AC encoding routine.
	int ac_prg_fs(const std::unique_ptr<BitWriter>& huffw, const HuffCodes& actbl, const std::array<std::int16_t, 64>& block,
		int* eobrun, int from, int to);
	// Progressive DC SA encoding routine.
	void dc_prg_sa(const std::unique_ptr<BitWriter>& huffw, const std::array<std::int16_t, 64>& block);
	// Progressive AC SA encoding routine.
	int ac_prg_sa(const std::unique_ptr<BitWriter>& huffw, const std::unique_ptr<MemoryWriter>& storw, const HuffCodes& actbl,
	              const std::array<std::int16_t, 64>& block, int* eobrun, int from, int to);
	// Run of EOB encoding routine.
	void eobrun(const std::unique_ptr<BitWriter>& huffw, const HuffCodes& actbl, int* eobrun);
	// Correction bits encoding routine.
	void crbits(const std::unique_ptr<BitWriter>& huffw, const std::unique_ptr<MemoryWriter>& storw);

	static constexpr std::int16_t fdiv2(std::int16_t v, int p) {
		return (v < 0) ? -((-v) >> p) : (v >> p);
	}

	static constexpr int envli(int s, int v) {
		return (v > 0) ? v : v - 1 + (1 << s);
	}

	static constexpr int e_envli(int s, int v) {
		return v - (1 << s);
	}

	static constexpr int bitlen(int v) {
		int length = 0;
		while ((v >> length) != 0) {
			length++;
		}
		return length;
	}

	std::vector<std::uint32_t> rstp; // restart markers positions in huffdata
	std::vector<std::uint32_t> scnp; // scan start positions in huffdata
};

#endif