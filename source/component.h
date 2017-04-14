#ifndef COMPONENT_H
#define COMPONENT_H

#include <cstdint>
#include <vector>

#include "dct8x8.h"
#include "pjpgtbl.h"

constexpr int clamp(int val, int lo, int hi) {
	return (val < lo) ? lo : (val > hi ? hi : val);
}

struct Component {
	std::vector<std::uint8_t> zdstdata; // zero distribution (# of non-zeroes) lists (for higher 7x7 block)
	std::vector<std::uint8_t> eobxhigh; // eob in x direction (for higher 7x7 block)
	std::vector<std::uint8_t> eobyhigh; // eob in y direction (for higher 7x7 block)
	std::vector<std::uint8_t> zdstxlow; // # of non zeroes for first row
	std::vector<std::uint8_t> zdstylow; // # of non zeroes for first column

	std::array<std::uint8_t, 64> freqscan; // optimized order for frequency scans (only pointers to scans)

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
	int sid = -1; // statistical identity
	int jid = -1; // jpeg internal id

	int segm_cnt = 10; // number of segments
	int nois_trs = 6; // bit pattern noise threshold


	int quant(int bp) const {
		return qtable[bp];
	}

	int max_v(int bp) const {
		return (quant(bp) > 0) ? (pjg::freqmax[bp] + quant(bp) - 1) / quant(bp) : 0;
	}

#if !defined(BUILD_LIB) && defined(DEV_BUILD)
	// Inverse DCT transform using precalc tables (fast).
	int idct_2d_fst_8x8(int dpos, int ix, int iy) const {
	// calculate start index
		const int ixy = ((iy << 3) + ix) << 6;

	// begin transform
		int idct = 0;
		idct += colldata[0][dpos] * adpt_idct_8x8[ixy + 0];
		idct += colldata[1][dpos] * adpt_idct_8x8[ixy + 1];
		idct += colldata[5][dpos] * adpt_idct_8x8[ixy + 2];
		idct += colldata[6][dpos] * adpt_idct_8x8[ixy + 3];
		idct += colldata[14][dpos] * adpt_idct_8x8[ixy + 4];
		idct += colldata[15][dpos] * adpt_idct_8x8[ixy + 5];
		idct += colldata[27][dpos] * adpt_idct_8x8[ixy + 6];
		idct += colldata[28][dpos] * adpt_idct_8x8[ixy + 7];
		idct += colldata[2][dpos] * adpt_idct_8x8[ixy + 8];
		idct += colldata[4][dpos] * adpt_idct_8x8[ixy + 9];
		idct += colldata[7][dpos] * adpt_idct_8x8[ixy + 10];
		idct += colldata[13][dpos] * adpt_idct_8x8[ixy + 11];
		idct += colldata[16][dpos] * adpt_idct_8x8[ixy + 12];
		idct += colldata[26][dpos] * adpt_idct_8x8[ixy + 13];
		idct += colldata[29][dpos] * adpt_idct_8x8[ixy + 14];
		idct += colldata[42][dpos] * adpt_idct_8x8[ixy + 15];
		idct += colldata[3][dpos] * adpt_idct_8x8[ixy + 16];
		idct += colldata[8][dpos] * adpt_idct_8x8[ixy + 17];
		idct += colldata[12][dpos] * adpt_idct_8x8[ixy + 18];
		idct += colldata[17][dpos] * adpt_idct_8x8[ixy + 19];
		idct += colldata[25][dpos] * adpt_idct_8x8[ixy + 20];
		idct += colldata[30][dpos] * adpt_idct_8x8[ixy + 21];
		idct += colldata[41][dpos] * adpt_idct_8x8[ixy + 22];
		idct += colldata[43][dpos] * adpt_idct_8x8[ixy + 23];
		idct += colldata[9][dpos] * adpt_idct_8x8[ixy + 24];
		idct += colldata[11][dpos] * adpt_idct_8x8[ixy + 25];
		idct += colldata[18][dpos] * adpt_idct_8x8[ixy + 26];
		idct += colldata[24][dpos] * adpt_idct_8x8[ixy + 27];
		idct += colldata[31][dpos] * adpt_idct_8x8[ixy + 28];
		idct += colldata[40][dpos] * adpt_idct_8x8[ixy + 29];
		idct += colldata[44][dpos] * adpt_idct_8x8[ixy + 30];
		idct += colldata[53][dpos] * adpt_idct_8x8[ixy + 31];
		idct += colldata[10][dpos] * adpt_idct_8x8[ixy + 32];
		idct += colldata[19][dpos] * adpt_idct_8x8[ixy + 33];
		idct += colldata[23][dpos] * adpt_idct_8x8[ixy + 34];
		idct += colldata[32][dpos] * adpt_idct_8x8[ixy + 35];
		idct += colldata[39][dpos] * adpt_idct_8x8[ixy + 36];
		idct += colldata[45][dpos] * adpt_idct_8x8[ixy + 37];
		idct += colldata[52][dpos] * adpt_idct_8x8[ixy + 38];
		idct += colldata[54][dpos] * adpt_idct_8x8[ixy + 39];
		idct += colldata[20][dpos] * adpt_idct_8x8[ixy + 40];
		idct += colldata[22][dpos] * adpt_idct_8x8[ixy + 41];
		idct += colldata[33][dpos] * adpt_idct_8x8[ixy + 42];
		idct += colldata[38][dpos] * adpt_idct_8x8[ixy + 43];
		idct += colldata[46][dpos] * adpt_idct_8x8[ixy + 44];
		idct += colldata[51][dpos] * adpt_idct_8x8[ixy + 45];
		idct += colldata[55][dpos] * adpt_idct_8x8[ixy + 46];
		idct += colldata[60][dpos] * adpt_idct_8x8[ixy + 47];
		idct += colldata[21][dpos] * adpt_idct_8x8[ixy + 48];
		idct += colldata[34][dpos] * adpt_idct_8x8[ixy + 49];
		idct += colldata[37][dpos] * adpt_idct_8x8[ixy + 50];
		idct += colldata[47][dpos] * adpt_idct_8x8[ixy + 51];
		idct += colldata[50][dpos] * adpt_idct_8x8[ixy + 52];
		idct += colldata[56][dpos] * adpt_idct_8x8[ixy + 53];
		idct += colldata[59][dpos] * adpt_idct_8x8[ixy + 54];
		idct += colldata[61][dpos] * adpt_idct_8x8[ixy + 55];
		idct += colldata[35][dpos] * adpt_idct_8x8[ixy + 56];
		idct += colldata[36][dpos] * adpt_idct_8x8[ixy + 57];
		idct += colldata[48][dpos] * adpt_idct_8x8[ixy + 58];
		idct += colldata[49][dpos] * adpt_idct_8x8[ixy + 59];
		idct += colldata[57][dpos] * adpt_idct_8x8[ixy + 60];
		idct += colldata[58][dpos] * adpt_idct_8x8[ixy + 61];
		idct += colldata[62][dpos] * adpt_idct_8x8[ixy + 62];
		idct += colldata[63][dpos] * adpt_idct_8x8[ixy + 63];

		return idct;
	}
#endif

	// Filter DC coefficients.
	void predict_dc() {
		// apply prediction, store prediction error instead of DC
		const int absmaxp = max_v(0);
		const int corr_f = (2 * absmaxp) + 1;

		for (int dpos = colldata[0].size() - 1; dpos > 0; dpos--) {
			auto& coef = colldata[0][dpos];
			coef -= dc_1ddct_predictor(dpos); // 1d dct
			// fix range
			if (coef > absmaxp) {
				coef -= corr_f;
			} else if (coef < -absmaxp) {
				coef += corr_f;
			}
		}
	}

	// Unpredict DC coefficients.
	void unpredict_dc() {
		// remove prediction, store DC instead of prediction error
		const int absmaxp = max_v(0);
		const int corr_f = (2 * absmaxp) + 1;

		for (int dpos = 1; dpos < bc; dpos++) {
			auto& coef = colldata[0][dpos];
			coef += dc_1ddct_predictor(dpos); // 1d dct predictor
			// fix range
			if (coef > absmaxp) {
				coef -= corr_f;
			} else if (coef < -absmaxp) {
				coef += corr_f;
			}
		}
	}

	// Adapt ICOS tables for quantizer tables
	void adapt_icos() {
		std::array<std::uint16_t, 64> quant; // local copy of quantization	

		// make a local copy of the quantization values, check
		for (int ipos = 0; ipos < quant.size(); ipos++) {
			quant[ipos] = this->quant(pjg::zigzag[ipos]);
			if (quant[ipos] >= 2048) { // if this is true, it can be safely assumed (for 8 bit JPEG), that all coefficients are zero
				quant[ipos] = 0;
			}
		}
		// adapt idct 8x8 table
		for (int ipos = 0; ipos < adpt_idct_8x8.size(); ipos++) {
			adpt_idct_8x8[ipos] = dct::icos_idct_8x8[ipos] * quant[ipos % 64];
		}
		// adapt idct 1x8 table
		for (int ipos = 0; ipos < adpt_idct_1x8.size(); ipos++) {
			adpt_idct_1x8[ipos] = dct::icos_idct_1x8[ipos] * quant[(ipos % 8) * 8];
		}
		// adapt idct 8x1 table
		for (int ipos = 0; ipos < adpt_idct_8x1.size(); ipos++) {
			adpt_idct_8x1[ipos] = dct::icos_idct_1x8[ipos] * quant[ipos % 8];
		}
	}

	// Calculate zero distribution lists.
	// This functions counts, for each DCT block, the number of non-zero coefficients
	void calc_zdst_lists() {
		// calculate # on non-zeroes per block (separately for lower 7x7 block & first row/column)
		for (int bpos = 1; bpos < colldata.size(); bpos++) {
			const int b_x = pjg::unzigzag[bpos] % 8;
			const int b_y = pjg::unzigzag[bpos] / 8;
			if (b_x == 0) {
				for (int dpos = 0; dpos < colldata[bpos].size(); dpos++) {
					if (colldata[bpos][dpos] != 0) {
						zdstylow[dpos]++;
					}
				}
			} else if (b_y == 0) {
				for (int dpos = 0; dpos < colldata[bpos].size(); dpos++) {
					if (colldata[bpos][dpos] != 0) {
						zdstxlow[dpos]++;
					}
				}
			} else {
				for (int dpos = 0; dpos < colldata[bpos].size(); dpos++) {
					if (colldata[bpos][dpos] != 0) {
						zdstdata[dpos]++;
					}
				}
			}
		}
	}

private:

	std::array<int, 8 * 8 * 8 * 8> adpt_idct_8x8; // precalculated/adapted values for idct (8x8)
	std::array<int, 1 * 1 * 8 * 8> adpt_idct_1x8; // precalculated/adapted values for idct (1x8)
	std::array<int, 8 * 8 * 1 * 1> adpt_idct_8x1; // precalculated/adapted values for idct (8x1)

	// Inverse DCT transform using precalc tables (fast).
	int idct_2d_fst_8x1(int dpos, int ix) const {
		// calculate start index
		const int ixy = ix << 3;

		// begin transform
		int idct = 0;
		idct += colldata[0][dpos] * adpt_idct_8x1[ixy + 0];
		idct += colldata[1][dpos] * adpt_idct_8x1[ixy + 1];
		idct += colldata[5][dpos] * adpt_idct_8x1[ixy + 2];
		idct += colldata[6][dpos] * adpt_idct_8x1[ixy + 3];
		idct += colldata[14][dpos] * adpt_idct_8x1[ixy + 4];
		idct += colldata[15][dpos] * adpt_idct_8x1[ixy + 5];
		idct += colldata[27][dpos] * adpt_idct_8x1[ixy + 6];
		idct += colldata[28][dpos] * adpt_idct_8x1[ixy + 7];

		return idct;
	}

	// Inverse DCT transform using precalc tables (fast).
	int idct_2d_fst_1x8(int dpos, int iy) const {
		// calculate start index
		const int ixy = iy << 3;

		// begin transform
		int idct = 0;
		idct += colldata[0][dpos] * adpt_idct_1x8[ixy + 0];
		idct += colldata[2][dpos] * adpt_idct_1x8[ixy + 1];
		idct += colldata[3][dpos] * adpt_idct_1x8[ixy + 2];
		idct += colldata[9][dpos] * adpt_idct_1x8[ixy + 3];
		idct += colldata[10][dpos] * adpt_idct_1x8[ixy + 4];
		idct += colldata[20][dpos] * adpt_idct_1x8[ixy + 5];
		idct += colldata[21][dpos] * adpt_idct_1x8[ixy + 6];
		idct += colldata[35][dpos] * adpt_idct_1x8[ixy + 7];

		return idct;
	}

	// 1D DCT predictor for DC coefficients.
	int dc_1ddct_predictor(int dpos) {
		const int w = bch;
		const int px = dpos % w;
		const int py = dpos / w;

		// Store current block DC coefficient:
		const auto swap = colldata[0][dpos];
		colldata[0][dpos] = short(0);

		// Calculate prediction:
		int pred = 0;
		if (px > 0 && py > 0) {
			const int pa = idct_2d_fst_8x1(dpos - 1, 7);
			const int xa = idct_2d_fst_8x1(dpos, 0);

			const int pb = idct_2d_fst_1x8(dpos - w, 7);
			const int xb = idct_2d_fst_1x8(dpos, 0);

			pred = ((pa - xa) + (pb - xb)) * 4;
		} else if (px > 0) {
			const int pa = idct_2d_fst_8x1(dpos - 1, 7);
			const int xa = idct_2d_fst_8x1(dpos, 0);

			pred = (pa - xa) * 8;
		} else if (py > 0) {
			const int pb = idct_2d_fst_1x8(dpos - w, 7);
			const int xb = idct_2d_fst_1x8(dpos, 0);

			pred = (pb - xb) * 8;
		}

		// Write back current DCT coefficient:
		colldata[0][dpos] = swap;

		// Clamp and quantize predictor:
		pred = clamp(pred, -(1024 * dct::DCT_RSC_FACTOR), 1016 * dct::DCT_RSC_FACTOR);
		pred = pred / quant(0);
		pred = dct::DCT_RESCALE(pred);

		return pred;
	}
};


#endif
