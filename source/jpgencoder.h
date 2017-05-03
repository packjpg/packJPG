#ifndef JPGENCODER_H
#define JPGENCODER_H

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "bitwriter.h"

#include "codingstatus.h"
#include "frameinfo.h"
#include "huffcodes.h"
#include "scaninfo.h"
#include "segment.h"
#include "writer.h"

class JpgEncoder {
public:
	JpgEncoder(Writer& jpg_output_writer, const std::vector<Segment>& segments);
	// JPEG encoding routine.
	void recode(FrameInfo& frame_info, std::uint8_t padbit);
	// Merges header & image data to jpeg.
	void merge(const std::vector<std::uint8_t>& garbage_data, std::vector<std::uint8_t>& rst_err);

private:
	// encoding for interleaved data.
	CodingStatus encode_interleaved(const FrameInfo& frame_info, int rsti, int& cmp, int& dpos, int& rstw, int& csc, int& mcu, int& sub);
	// encoding for non interleaved data.
	CodingStatus encode_noninterleaved(const FrameInfo& frame_info, int rsti, int cmp, int& dpos, int& rstw);

	// Sequential block encoding routine.
	void block_seq(const HuffCodes& dc_table, const HuffCodes& ac_table);
	// Progressive DC encoding routine.
	void dc_prg_fs(const HuffCodes& dc_table);
	// Progressive AC encoding routine.
	void ac_prg_fs(const HuffCodes& ac_table, int& eobrun);
	// Progressive DC SA encoding routine.
	void dc_prg_sa();
	// Progressive AC SA encoding routine.
	void ac_prg_sa(const HuffCodes& ac_table, int& eobrun);
	// Run of EOB encoding routine.
	void eobrun(const HuffCodes& ac_table, int& eobrun);
	// Correction bits encoding routine.
	void crbits();

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

	void copy_colldata_to_block_in_scan(const Component& component, int dpos);

	void sequential_interleaved(const Component& component, int cmp, int dpos);

	void dc_successive_first_stage(const Component& component, int cmp, int dpos);
	void dc_successive_later_stage(const Component& component, int dpos);

	Writer& jpg_output_writer_;

	std::unique_ptr<BitWriter> huffw_; // Bitwise writer for image data.
	std::unique_ptr<Writer> storw_; // Bytewise writer for storage of correction bits.
	std::array<std::int16_t, 64> block_{};  // Store block for coefficientss.
	ScanInfo scan_info_;

	std::map<int, std::unique_ptr<HuffCodes>> dc_tables_;
	std::map<int, std::unique_ptr<HuffCodes>> ac_tables_;

	std::array<int, 4> lastdc_;  // last dc for each component (used for diff coding)

	const std::vector<Segment>& segments_;

	std::vector<std::uint8_t> huffman_data_;

	std::vector<std::size_t> rstp_; // restart markers positions in huffdata
	std::vector<std::size_t> scnp_; // scan start positions in huffdata
};

#endif