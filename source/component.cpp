#include "component.h"

#include "bitops.h"
#include "dct8x8.h"
#include "pjpgtbl.h"
#include "zerodistribution.h"

std::uint16_t Component::quant(std::size_t bp) const {
	return qtable[bp];
}

int Component::max_v(std::size_t bp) const {
	return (quant(bp) > 0) ? (pjg::freqmax[bp] + quant(bp) - 1) / quant(bp) : 0;
}

// Filter DC coefficients.
void Component::predict_dc() {
	// apply prediction, store prediction error instead of DC
	const auto absmaxp = std::int16_t(max_v(0));
	const auto corr_f = std::int16_t((2 * absmaxp) + 1);

	for (std::size_t dpos = colldata[0].size() - 1; dpos > 0; dpos--) {
		auto& coeff = colldata[0][dpos];
		coeff -= std::int16_t(dc_1ddct_predictor(dpos)); // 1d dct
		// fix range
		if (coeff > absmaxp) {
			coeff -= corr_f;
		} else if (coeff < -absmaxp) {
			coeff += corr_f;
		}
	}
}

// Unpredict DC coefficients.
void Component::unpredict_dc() {
	// remove prediction, store DC instead of prediction error
	const auto absmaxp = std::int16_t(max_v(0));
	const auto corr_f = std::int16_t((2 * absmaxp) + 1);

	for (std::size_t dpos = 1; dpos < colldata[0].size(); dpos++) {
		auto& coeff = colldata[0][dpos];
		coeff += std::int16_t(dc_1ddct_predictor(dpos)); // 1d dct predictor
		// fix range
		if (coeff > absmaxp) {
			coeff -= corr_f;
		} else if (coeff < -absmaxp) {
			coeff += corr_f;
		}
	}
}

// Adapt ICOS tables for quantizer tables
void Component::adapt_icos() {
	std::array<std::uint16_t, 64> quant; // local copy of quantization	

	// make a local copy of the quantization values, check
	for (std::size_t ipos = 0; ipos < quant.size(); ipos++) {
		quant[ipos] = this->quant(pjg::zigzag[ipos]);
		if (quant[ipos] >= 2048) { // if this is true, it can be safely assumed (for 8 bit JPEG), that all coefficients are zero
			quant[ipos] = 0;
		}
	}
	// adapt idct 8x8 table
	for (std::size_t ipos = 0; ipos < 64 * 64; ipos++) {
		adpt_idct_8x8[ipos] = dct::icos_idct_8x8[ipos] * quant[ipos % 64];
	}
	// adapt idct 1x8 table
	for (std::size_t ipos = 0; ipos < adpt_idct_1x8.size(); ipos++) {
		adpt_idct_1x8[ipos] = dct::icos_idct_1x8[ipos] * quant[(ipos % 8) * 8];
	}
	// adapt idct 8x1 table
	for (std::size_t ipos = 0; ipos < adpt_idct_8x1.size(); ipos++) {
		adpt_idct_8x1[ipos] = dct::icos_idct_1x8[ipos] * quant[ipos % 8];
	}
}

ZeroDistribution Component::calc_zdst_lists() const {
	ZeroDistribution zero_data(bc);

	// calculate # on non-zeroes per block (separately for lower 7x7 block & first row/column)
	for (std::size_t bpos = 1; bpos < colldata.size(); bpos++) {
		const int b_x = pjg::unzigzag[bpos] % 8;
		const int b_y = pjg::unzigzag[bpos] / 8;
		if (b_x == 0) {
			for (std::size_t dpos = 0; dpos < colldata[bpos].size(); dpos++) {
				if (colldata[bpos][dpos] != 0) {
					zero_data.zdstylow[dpos]++;
				}
			}
		} else if (b_y == 0) {
			for (std::size_t dpos = 0; dpos < colldata[bpos].size(); dpos++) {
				if (colldata[bpos][dpos] != 0) {
					zero_data.zdstxlow[dpos]++;
				}
			}
		} else {
			for (std::size_t dpos = 0; dpos < colldata[bpos].size(); dpos++) {
				if (colldata[bpos][dpos] != 0) {
					zero_data.zero_dist_list[dpos]++;
				}
			}
		}
	}
	return zero_data;
}

CodingStatus Component::next_mcuposn(int rsti, int& dpos, int& rstw) const {
	dpos = this->calc_next_pos_noninterleaved(dpos);
	return this->determine_status_noninterleaved(rsti, dpos, rstw);
}

int Component::calc_next_pos_noninterleaved(int curr_pos) const {
	int next_pos = curr_pos + 1;

	// fix for non interleaved mcu - horizontal
	if (bch != nch) {
		if (next_pos % bch == nch) {
			next_pos += (bch - nch);
		}
	}

	// fix for non interleaved mcu - vertical
	if (bcv != ncv) {
		if (next_pos / bch == ncv) {
			next_pos = bc;
		}
	}
	return next_pos;
}

CodingStatus Component::determine_status_noninterleaved(int rsti, int dpos, int& rstw) const {
	// check position
	if (dpos >= bc) {
		return CodingStatus::DONE;
	} else if (rsti > 0) {
		rstw--;
		if (rstw == 0) {
			return CodingStatus::RESTART;
		}
	}

	return CodingStatus::OKAY;
}

int Component::idct_2d_fst_8x1(std::size_t dpos, int ix) const {
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

int Component::idct_2d_fst_1x8(std::size_t dpos, int iy) const {
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

int Component::idct_2d_fst_8x8(int dpos, int ix, int iy) const {
	int idct = 0;

	// calculate start index
	const int ixy = ((iy << 3) + ix) << 6;

	// begin transform
	idct += colldata[0][dpos]  * adpt_idct_8x8[ixy + 0];
	idct += colldata[1][dpos]  * adpt_idct_8x8[ixy + 1];
	idct += colldata[5][dpos]  * adpt_idct_8x8[ixy + 2];
	idct += colldata[6][dpos]  * adpt_idct_8x8[ixy + 3];
	idct += colldata[14][dpos] * adpt_idct_8x8[ixy + 4];
	idct += colldata[15][dpos] * adpt_idct_8x8[ixy + 5];
	idct += colldata[27][dpos] * adpt_idct_8x8[ixy + 6];
	idct += colldata[28][dpos] * adpt_idct_8x8[ixy + 7];
	idct += colldata[2][dpos]  * adpt_idct_8x8[ixy + 8];
	idct += colldata[4][dpos]  * adpt_idct_8x8[ixy + 9];
	idct += colldata[7][dpos]  * adpt_idct_8x8[ixy + 10];
	idct += colldata[13][dpos] * adpt_idct_8x8[ixy + 11];
	idct += colldata[16][dpos] * adpt_idct_8x8[ixy + 12];
	idct += colldata[26][dpos] * adpt_idct_8x8[ixy + 13];
	idct += colldata[29][dpos] * adpt_idct_8x8[ixy + 14];
	idct += colldata[42][dpos] * adpt_idct_8x8[ixy + 15];
	idct += colldata[3][dpos]  * adpt_idct_8x8[ixy + 16];
	idct += colldata[8][dpos]  * adpt_idct_8x8[ixy + 17];
	idct += colldata[12][dpos] * adpt_idct_8x8[ixy + 18];
	idct += colldata[17][dpos] * adpt_idct_8x8[ixy + 19];
	idct += colldata[25][dpos] * adpt_idct_8x8[ixy + 20];
	idct += colldata[30][dpos] * adpt_idct_8x8[ixy + 21];
	idct += colldata[41][dpos] * adpt_idct_8x8[ixy + 22];
	idct += colldata[43][dpos] * adpt_idct_8x8[ixy + 23];
	idct += colldata[9][dpos]  * adpt_idct_8x8[ixy + 24];
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

int Component::dc_1ddct_predictor(std::size_t dpos) {
	const int w = bch;
	const auto px = dpos % w;
	const auto py = dpos / w;

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
	pred = bitops::clamp(pred, -(1024 * dct::DCT_RSC_FACTOR), 1016 * dct::DCT_RSC_FACTOR);
	pred = pred / quant(0);
	pred = dct::DCT_RESCALE(pred);

	return pred;
}
