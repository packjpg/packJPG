#ifndef COMPONENT_H
#define COMPONENT_H

#include <array>
#include <cstdint>
#include <tuple>
#include <vector>

#include "codingstatus.h"

class Component {
public:
	int quant(std::size_t bp) const;

	int max_v(std::size_t bp) const;

	// Filter DC coefficients.
	void predict_dc();

	// Unpredict DC coefficients.
	void unpredict_dc();

	// Adapt ICOS tables for quantizer tables
	void adapt_icos();

	// Calculate zero distribution lists.
	// This functions counts, for each DCT block, the number of non-zero coefficients
	std::tuple<std::vector<std::uint8_t>, std::vector<std::uint8_t>, std::vector<std::uint8_t>> calc_zdst_lists() const;

	// Calculates next position (non interleaved).
	CodingStatus next_mcuposn(int rsti, int& dpos, int& rstw) const;

	int idct_2d_fst_8x8(int dpos, int ix, int iy) const;

	std::array<std::vector<int16_t>, 64> colldata; // Collection sorted DCT coefficients.

	std::array<std::uint16_t, 64> qtable{}; // quantization table
	int huffdc = -1; // no of huffman table (DC)
	int huffac = -1; // no of huffman table (AC)
	int sfv = -1; // sample factor vertical
	int sfh = -1; // sample factor horizontal	
	int mbs = -1; // blocks in mcu
	int bcv = -1; // block count vertical (interleaved)
	int bch = -1; // block count horizontal (interleaved)
	int bc = -1; // block count (all) (interleaved)
	int ncv = -1; // block count vertical (non interleaved)
	int nch = -1; // block count horizontal (non interleaved)
	std::size_t sid = 0; // statistical identity
	int jid = -1; // jpeg internal id

	int segm_cnt = 10; // number of segments
	int nois_trs = 6; // bit pattern noise threshold

private:
	std::array<int, 8 * 8 * 8 * 8> adpt_idct_8x8{};	// precalculated/adapted values for idct (8x8)
	std::array<int, 1 * 1 * 8 * 8> adpt_idct_1x8; // precalculated/adapted values for idct (1x8)
	std::array<int, 8 * 8 * 1 * 1> adpt_idct_8x1; // precalculated/adapted values for idct (8x1)

	// Calculates the next position (noninterleaved).
	inline int calc_next_pos_noninterleaved(int curr_pos) const;

	CodingStatus determine_status_noninterleaved(int rsti, int dpos, int& rstw) const;

	// Inverse DCT transform using precalc tables (fast).
	int idct_2d_fst_8x1(std::size_t dpos, int ix) const;

	// Inverse DCT transform using precalc tables (fast).
	int idct_2d_fst_1x8(std::size_t dpos, int iy) const;
	// 1D DCT predictor for DC coefficients.
	int dc_1ddct_predictor(std::size_t dpos);
};


#endif
