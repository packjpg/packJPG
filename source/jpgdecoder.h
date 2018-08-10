#ifndef JPGDECODER_H
#define JPGDECODER_H

#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

#include "bitreader.h"

#include "component.h"
#include "codingstatus.h"
#include "frameinfo.h"
#include "huffcodes.h"
#include "hufftree.h"
#include "scaninfo.h"
#include "segment.h"

class JpgDecoder {
public:

	JpgDecoder(const FrameInfo& frame_info, const std::vector<Segment>& segments, const std::vector<std::uint8_t>& huffman_data);

	// JPEG decoding routine. Returns the padbit used in the huffman coding.
	std::uint8_t decode(std::vector<Component>& components);

private:
	// Sequential block decoding routine.
	std::size_t block_seq(const HuffTree& dctree, const HuffTree& actree);
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

	void check_huffman_tables_available(const std::vector<Component>& components, int scans_finished);
	void decode_scan(std::vector<Component>& components, int restart_interval);
	CodingStatus decode_interleaved_data(std::vector<Component>& components, int rsti, int& cmp, int& dpos, int& mcu, int& csc, int& sub);
	CodingStatus decode_noninterleaved_data(std::vector<Component>& components, int rsti, int cmp, int& dpos);

	void check_eobrun(const Component& component, int eob, int& eobrun, int& peobrun);
	void decode_sequential_block(Component& component, int cmp, int dpos);
	void decode_successive_approx_first_stage(Component& component, int cmp, int dpos);
	void decode_success_approx_later_stage(Component& component, int dpos);

	const FrameInfo& frame_info_;
	const std::vector<Segment>& segments_;

	std::array<std::int16_t, 4> lastdc_{}; // last dc for each component
	std::array<std::int16_t, 64> block_{}; // store block for coeffs
	ScanInfo scan_info_;

	std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2> hcodes_; // huffman codes
	std::array<std::array<std::unique_ptr<HuffTree>, 4>, 2> htrees_; // huffman decoding trees

	bool padbit_set_ = false;
	std::uint8_t padbit_ = std::numeric_limits<std::uint8_t>::max();

	std::unique_ptr<BitReader> huffman_reader_; // bitwise reader for image data
};

#endif