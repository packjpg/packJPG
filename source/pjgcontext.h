#ifndef PJGCONTEXT_H
#define PJGCONTEXT_H

#include <array>
#include <cstdint>
#include <tuple>

#include "component.h"

class PjgContext {
private:
	std::array<std::uint16_t*, 6> quick_abs_coeffs_{}; // quick access array for contexts

public:
	PjgContext(const Component& component);

	void reset_store();

	// Special average context used in coeff encoding.
	int aavrg_context(int pos, int p_y, int p_x, int r_x);

	// Lakhani ac context used in coeff encoding.
	static int lakh_context(const std::array<int16_t*, 8>& coeffs_x, const std::array<int16_t*, 8>& coeffs_a, const std::array<int, 8>& pred_cf, int pos);

	// Calculates coordinates for nearest neighbor (2D) context.
	static std::pair<int, int> get_context_nnb(int pos, int w);

	std::vector<std::uint16_t> abs_coeffs_; // absolute coefficients values storage
};


#endif
