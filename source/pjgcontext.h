#ifndef PJGCONTEXT_H
#define PJGCONTEXT_H

#include <array>
#include <cstdint>

#include "component.h"
#include "pjpgtbl.h"

class PjgContext {
private:
	// Context weighting factors:
	static constexpr std::array<int, 6> weights {
		pjg::abs_ctx_weights_lum[0][0][2], // top-top
		pjg::abs_ctx_weights_lum[0][1][1], // top-left
		pjg::abs_ctx_weights_lum[0][1][2], // top
		pjg::abs_ctx_weights_lum[0][1][3], // top-right
		pjg::abs_ctx_weights_lum[0][2][0], // left-left
		pjg::abs_ctx_weights_lum[0][2][1] // left
	};

public:

	// Preparations for special average context.
	static void aavrg_prepare(std::array<std::uint16_t*, 6>& abs_coeffs, std::uint16_t* abs_store, const Component& component);

	// Special average context used in coeff encoding.
	static int aavrg_context(const std::array<std::uint16_t*, 6>& abs_coeffs, int pos, int p_y, int p_x, int r_x);

	// Lakhani ac context used in coeff encoding.
	static int lakh_context(const std::array<int16_t*, 8>& coeffs_x, const std::array<int16_t*, 8>& coeffs_a, const std::array<int, 8>& pred_cf, int pos);

	// Calculates coordinates for nearest neighbor (2D) context.
	static std::pair<int, int> get_context_nnb(int pos, int w);
};


#endif
