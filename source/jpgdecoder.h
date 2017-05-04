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

class JpgDecoder {
public:

	// JPEG decoding routine.
	void decode(FrameInfo& frame_info, const std::vector<Segment>& segments, const std::vector<std::uint8_t>& huffdata);
	// Checks range of values, error if out of bounds.
	void check_value_range(const std::vector<Component>& components);

	std::uint8_t get_padbit();

private:
	// Sequential block decoding routine.
	int block_seq(const HuffTree& dctree, const HuffTree& actree);
	// Progressive DC decoding routine.
	void dc_prg_fs(const HuffTree& dctree);
	// Progressive AC decoding routine.
	int ac_prg_fs(const HuffTree& actree, int& eobrun);
	// Progressive DC SA decoding routine.
	void dc_prg_sa();
	// Progressive AC SA decoding routine.
	int ac_prg_sa(const HuffTree& actree, int& eobrun);
	// Run of EOB SA decoding routine.
	void eobrun_sa();

	// Skips the eobrun, calculates next position.
	CodingStatus skip_eobrun(const Component& component, int rsti, int& dpos, int& rstw, int& eobrun);

	void build_trees();

	static constexpr int devli(int s, int n) {
		return (n >= 1 << (s - 1)) ? n : n + 1 - (1 << s);
	}

	static constexpr int e_devli(int s, int n) {
		return n + (1 << s);
	}

	void decode_sequential_block(Component& component, int cmp, int dpos);
	void decode_successive_approx_first_stage(Component& component, int cmp, int dpos);
	void decode_success_approx_later_stage(Component& component, int dpos);

	std::array<int, 4> lastdc{}; // last dc for each component
	std::array<std::int16_t, 64> block{}; // store block for coeffs
	ScanInfo scan_info;

	std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2> hcodes; // huffman codes
	std::array<std::array<std::unique_ptr<HuffTree>, 4>, 2> htrees; // huffman decoding trees

	bool padbit_set = false;
	std::uint8_t padbit = -1;

	std::unique_ptr<BitReader> huffr; // bitwise reader for image data
};

#endif