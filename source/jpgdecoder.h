#ifndef JPGDECODER_H
#define JPGDECODER_H

#include <memory>
#include <vector>

#include "bitreader.h"

#include "component.h"
#include "codingstatus.h"
#include "huffcodes.h"
#include "hufftree.h"
#include "scaninfo.h"
#include "segment.h"
#include "frameinfo.h"
#include <algorithm>
#include <algorithm>
#include <algorithm>

class JpgDecoder {
public:

	// JPEG decoding routine.
	void decode(FrameInfo& frame_info, const std::vector<Segment>& segments, const std::vector<std::uint8_t>& huffdata);
	// Checks range of values, error if out of bounds.
	void check_value_range(const std::vector<Component>& components);

	std::uint8_t get_padbit();

private:
	// Sequential block decoding routine.
	int block_seq(const HuffTree& dctree, const HuffTree& actree, std::array<std::int16_t, 64>& block);
	// Progressive DC decoding routine.
	void dc_prg_fs(const HuffTree& dctree, std::array<std::int16_t, 64>& block);
	// Progressive AC decoding routine.
	int ac_prg_fs(const HuffTree& actree, const ScanInfo& scan_info, std::array<std::int16_t, 64>& block, int& eobrun);
	// Progressive DC SA decoding routine.
	void dc_prg_sa(std::array<std::int16_t, 64>& block);
	// Progressive AC SA decoding routine.
	int ac_prg_sa(const HuffTree& actree, const ScanInfo& scan_info, std::array<std::int16_t, 64>& block, int& eobrun);
	// Run of EOB SA decoding routine.
	void eobrun_sa(const ScanInfo& scan_info, std::array<std::int16_t, 64>& block);

	// Skips the eobrun, calculates next position.
	CodingStatus skip_eobrun(const Component& cmpt, int rsti, int& dpos, int& rstw, int& eobrun);

	void build_trees(const std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2>& hcodes,
		std::array<std::array<std::unique_ptr<HuffTree>, 4>, 2>& htrees);

	static constexpr int devli(int s, int n) {
		return (n >= 1 << (s - 1)) ? n : n + 1 - (1 << s);
	}

	static constexpr int e_devli(int s, int n) {
		return n + (1 << s);
	}

	bool padbit_set = false;
	std::uint8_t padbit = -1;

	std::unique_ptr<BitReader> huffr; // bitwise reader for image data
};

#endif