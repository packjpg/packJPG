#ifndef JPGENCODER_H
#define JPGENCODER_H

#include <cstdint>
#include <map>
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
	CodingStatus encode_interleaved(const FrameInfo& frame_info, const ScanInfo& scan_info, std::map<int, std::unique_ptr<HuffCodes>>& dc_tables, std::map<int, std::unique_ptr<HuffCodes>>& ac_tables, std::array<std::int16_t, 64>& block, BitWriter& huffw, int rsti, int& cmp, int& dpos, int& rstw, int& csc, int& mcu, int& sub);
	// encoding for non interleaved data.
	CodingStatus encode_noninterleaved(const FrameInfo& frame_info, const ScanInfo& scan_info, std::map<int, std::unique_ptr<HuffCodes>>& dc_tables, std::map<int, std::unique_ptr<HuffCodes>>& ac_tables, std::array<std::int16_t, 64>& block, BitWriter& huffw, MemoryWriter& storw, int rsti, int& cmp, int& dpos, int& rstw);


	// Sequential block encoding routine.
	void block_seq(BitWriter& huffw, const HuffCodes& dctbl, const HuffCodes& actbl, const std::array<std::int16_t, 64>& block);
	// Progressive DC encoding routine.
	void dc_prg_fs(BitWriter& huffw, const HuffCodes& dctbl, const std::array<std::int16_t, 64>& block);
	// Progressive AC encoding routine.
	void ac_prg_fs(BitWriter& huffw, const HuffCodes& actbl, const ScanInfo& scan_info, const std::array<std::int16_t, 64>& block, int& eobrun);
	// Progressive DC SA encoding routine.
	void dc_prg_sa(BitWriter& huffw, const std::array<std::int16_t, 64>& block);
	// Progressive AC SA encoding routine.
	void ac_prg_sa(BitWriter& huffw, Writer& storw, const HuffCodes& actbl, const ScanInfo& scan_info, const std::array<std::int16_t, 64>& block, int& eobrun);
	// Run of EOB encoding routine.
	void eobrun(BitWriter& huffw, const HuffCodes& actbl, int& eobrun);
	// Correction bits encoding routine.
	void crbits(BitWriter& huffw, Writer& storw);

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

	Writer& jpg_output_writer_;

	const std::vector<Segment>& segments_;

	std::vector<std::uint8_t> huffman_data_;

	std::vector<std::size_t> rstp_; // restart markers positions in huffdata
	std::vector<std::size_t> scnp_; // scan start positions in huffdata
};

#endif