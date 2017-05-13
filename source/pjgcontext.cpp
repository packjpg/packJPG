#include "pjgcontext.h"

PjgContext::PjgContext(const Component& component) : abs_coeffs_(component.bc) {
	const auto w = component.bch;
	quick_abs_coeffs_[0] = abs_coeffs_.data() + (0 + ((-2) * w)); // top-top
	quick_abs_coeffs_[1] = abs_coeffs_.data() + (-1 + ((-1) * w)); // top-left
	quick_abs_coeffs_[2] = abs_coeffs_.data() + (0 + ((-1) * w)); // top
	quick_abs_coeffs_[3] = abs_coeffs_.data() + (1 + ((-1) * w)); // top-right
	quick_abs_coeffs_[4] = abs_coeffs_.data() + (-2 + ((0) * w)); // left-left
	quick_abs_coeffs_[5] = abs_coeffs_.data() + (-1 + ((0) * w)); // left
}

void PjgContext::reset_store() {
	std::fill(std::begin(abs_coeffs_), std::end(abs_coeffs_), static_cast<std::uint16_t>(0));
}

int PjgContext::aavrg_context(int pos, int p_y, int p_x, int r_x) {
	int ctx_avr = 0; // AVERAGE context
	int w_ctx = 0; // accumulated weight of context
	int w_curr; // current weight of context

	// different cases due to edge treatment
	if (p_y >= 2) {
		w_curr = std::get<0>(weights);
		ctx_avr += quick_abs_coeffs_[0][pos] * w_curr;
		w_ctx += w_curr;
		w_curr = std::get<2>(weights);
		ctx_avr += quick_abs_coeffs_[2][pos] * w_curr;
		w_ctx += w_curr;
		if (p_x >= 2) {
			w_curr = std::get<1>(weights);
			ctx_avr += quick_abs_coeffs_[1][pos] * w_curr;
			w_ctx += w_curr;
			w_curr = std::get<4>(weights);
			ctx_avr += quick_abs_coeffs_[4][pos] * w_curr;
			w_ctx += w_curr;
			w_curr = std::get<5>(weights);
			ctx_avr += quick_abs_coeffs_[5][pos] * w_curr;
			w_ctx += w_curr;
		} else if (p_x == 1) {
			w_curr = std::get<1>(weights);
			ctx_avr += quick_abs_coeffs_[1][pos] * w_curr;
			w_ctx += w_curr;
			w_curr = std::get<5>(weights);
			ctx_avr += quick_abs_coeffs_[5][pos] * w_curr;
			w_ctx += w_curr;
		}
		if (r_x >= 1) {
			w_curr = std::get<3>(weights);
			ctx_avr += quick_abs_coeffs_[3][pos] * w_curr;
			w_ctx += w_curr;
		}
	} else if (p_y == 1) {
		w_curr = std::get<2>(weights);
		ctx_avr += quick_abs_coeffs_[2][pos] * w_curr;
		w_ctx += w_curr;
		if (p_x >= 2) {
			w_curr = std::get<1>(weights);
			ctx_avr += quick_abs_coeffs_[1][pos] * w_curr;
			w_ctx += w_curr;
			w_curr = std::get<4>(weights);
			ctx_avr += quick_abs_coeffs_[4][pos] * w_curr;
			w_ctx += w_curr;
			w_curr = std::get<5>(weights);
			ctx_avr += quick_abs_coeffs_[5][pos] * w_curr;
			w_ctx += w_curr;
		} else if (p_x == 1) {
			w_curr = std::get<1>(weights);
			ctx_avr += quick_abs_coeffs_[1][pos] * w_curr;
			w_ctx += w_curr;
			w_curr = std::get<5>(weights);
			ctx_avr += quick_abs_coeffs_[5][pos] * w_curr;
			w_ctx += w_curr;
		}
		if (r_x >= 1) {
			w_curr = std::get<3>(weights);
			ctx_avr += quick_abs_coeffs_[3][pos] * w_curr;
			w_ctx += w_curr;
		}
	} else {
		if (p_x >= 2) {
			w_curr = std::get<4>(weights);
			ctx_avr += quick_abs_coeffs_[4][pos] * w_curr;
			w_ctx += w_curr;
			w_curr = std::get<5>(weights);
			ctx_avr += quick_abs_coeffs_[5][pos] * w_curr;
			w_ctx += w_curr;
		} else if (p_x == 1) {
			w_curr = std::get<5>(weights);
			ctx_avr += quick_abs_coeffs_[5][pos] * w_curr;
			w_ctx += w_curr;
		}
	}

	// return average context
	return (w_ctx != 0) ? (ctx_avr + (w_ctx / 2)) / w_ctx : 0;
}

int PjgContext::lakh_context(const std::array<int16_t*, 8>& coeffs_x, const std::array<int16_t*, 8>& coeffs_a, const std::array<int, 8>& pred_cf, int pos) {
	int pred = 0;

	// calculate partial prediction
	pred -= (coeffs_x[1][pos] + coeffs_a[1][pos]) * pred_cf[1];
	pred -= (coeffs_x[2][pos] - coeffs_a[2][pos]) * pred_cf[2];
	pred -= (coeffs_x[3][pos] + coeffs_a[3][pos]) * pred_cf[3];
	pred -= (coeffs_x[4][pos] - coeffs_a[4][pos]) * pred_cf[4];
	pred -= (coeffs_x[5][pos] + coeffs_a[5][pos]) * pred_cf[5];
	pred -= (coeffs_x[6][pos] - coeffs_a[6][pos]) * pred_cf[6];
	pred -= (coeffs_x[7][pos] + coeffs_a[7][pos]) * pred_cf[7];
	// normalize / quantize partial prediction
	pred = ((pred > 0) ? (pred + (pred_cf[0] / 2)) : (pred - (pred_cf[0] / 2))) / pred_cf[0];
	// complete prediction
	pred += coeffs_a[0][pos];

	return pred;
}

std::pair<int, int> PjgContext::get_context_nnb(int pos, int w) {
	std::pair<int, int> coords;
	if (pos == 0) {
		coords = std::make_pair<int, int>(-1, -1);
	} else if ((pos % w) == 0) {
		if (pos >= (w << 1)) {
			coords = std::make_pair<int, int>(pos - (w << 1), pos - w);
		} else {
			coords = std::make_pair<int, int>(pos - w, pos - w);
		}
	} else if (pos < w) {
		if (pos >= 2) {
			coords = std::make_pair<int, int>(pos - 1, pos - 2);
		} else {
			coords = std::make_pair<int, int>(pos - 1, pos - 1);
		}
	} else {
		coords = std::make_pair<int, int>(pos - 1, pos - w);
	}
	return coords;
}
