#ifndef JPGDECODER_H
#define JPGDECODER_H

#include <memory>
#include <vector>

#include "component.h"
#include "codingstatus.h"
#include "huffcodes.h"
#include "hufftree.h"
#include "jpegtype.h"
#include "segment.h"

class JpgDecoder {
public:

	// JPEG decoding routine.
	void decode(JpegType jpegtype, const std::vector<Segment>& segments, std::vector<Component>& cmpts, const std::vector<std::uint8_t>& huffdata);
	// Checks range of values, error if out of bounds.
	void check_value_range(const std::vector<Component>& cmpts);

private:
	// Sequential block decoding routine.
	int block_seq(const HuffTree& dctree, const HuffTree& actree, short* block);
	// Progressive DC decoding routine.
	CodingStatus dc_prg_fs(const HuffTree& dctree, short* block);
	// Progressive AC decoding routine.
	int ac_prg_fs(const HuffTree& actree, short* block,
		int* eobrun, int from, int to);
	// Progressive DC SA decoding routine.
	void dc_prg_sa(short* block);
	// Progressive AC SA decoding routine.
	int ac_prg_sa(const HuffTree& actree, short* block,
		int* eobrun, int from, int to);
	// Run of EOB SA decoding routine.
	void eobrun_sa(short* block, int from, int to);

	// Skips the eobrun, calculates next position.
	CodingStatus skip_eobrun(const Component& cmpt, int rsti, int* dpos, int* rstw, int* eobrun);

	void build_trees(const std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2>& hcodes,
		std::array<std::array<std::unique_ptr<HuffTree>, 4>, 2>& htrees);

	static constexpr int devli(int s, int n) {
		return (n >= 1 << (s - 1)) ? n : n + 1 - (1 << s);
	}

	static constexpr int e_devli(int s, int n) {
		return n + (1 << s);
	}

	std::unique_ptr<BitReader> huffr; // bitwise reader for image data
};

#endif