#include "pjgencoder.h"

#include <numeric>
#include <algorithm>

#include "bitops.h"
#include "dct8x8.h"
#include "pjgcontext.h"
#include "pjpgtbl.h"
#include "programinfo.h"

PjgEncoder::PjgEncoder(Writer& encoding_output) {
	// PJG-Header
	encoding_output.write(program_info::pjg_magic);

	// store version number
	encoding_output.write_byte(program_info::appversion);

	// init arithmetic compression
	encoder_ = std::make_unique<ArithmeticEncoder>(encoding_output);
}

void PjgEncoder::encode(std::uint8_t padbit, std::vector<Component>& components, std::vector<Segment>& segments, const std::vector<std::uint8_t>& rst_err, const std::vector<std::uint8_t>& garbage_data) {
	// Set the padbit to 1 if it has not been set:
	if (padbit == -1) {
		padbit = 1;
	}

	// Optimize the header segments for compression:
	for (auto& segment : segments) {
		segment.optimize();
	}
	// Encode the header segments:
	this->generic(segments);

	// Store the padbit (as it can't be retrieved from the header during decompression):
	this->bit(padbit);

	// Encode a bit indicating whether RST markers are used:
	std::uint8_t rst_err_used = rst_err.empty() ? 0 : 1;
	this->bit(rst_err_used);
	// Encode the number of false set RST markers per scan:
	if (rst_err_used) {
		this->generic(rst_err);
	}

	// Encode component data:
	for (auto& component : components) {
		auto zero_dist_lists = component.calc_zdst_lists();
		component.freqscan = this->get_zero_sorted_scan(component);
		this->encode_zero_sorted_scan(component.freqscan);
		this->zdst_high(component, std::get<0>(zero_dist_lists));
		const auto eob_data = this->ac_high(component, std::vector<std::uint8_t>(std::get<0>(zero_dist_lists)));
		this->zdst_low(component, std::get<0>(zero_dist_lists), std::get<1>(zero_dist_lists), std::get<2>(zero_dist_lists), eob_data.first, eob_data.second);
		this->ac_low(component, std::get<1>(zero_dist_lists), std::get<2>(zero_dist_lists));
		this->dc(component, std::get<0>(zero_dist_lists));
	}

	// Encode a bit indicating whether there is garbage data:
	std::uint8_t garbage_exists = garbage_data.empty() ? 0 : 1;
	this->bit(garbage_exists);
	// Encode the garbage data:
	if (garbage_exists) {
		this->generic(garbage_data);
	}

	encoder_->finalize();

	if (encoder_->error()) {
		throw std::runtime_error("Error occurred while writing, drive is possibly full.");
	}
}

void PjgEncoder::encode_zero_sorted_scan(const std::array<std::uint8_t, 64>& zero_sorted_scan) {
	// Skip the first (DC) element, since it is always the first element in the zero-sorted scan order.
	std::vector<std::uint8_t> standard_scan(std::begin(pjg::stdscan) + 1, std::end(pjg::stdscan));
	auto model = std::make_unique<UniversalModel>(64, 64, 1);

	// Encode scanorder:
	for (int i = 1; i < zero_sorted_scan.size(); i++) {
		model->exclude_symbols_above(64 - i);

		bool remainder_sorted = std::equal(std::begin(standard_scan),
		                                   std::end(standard_scan),
		                                   std::begin(zero_sorted_scan) + i,
		                                   std::end(zero_sorted_scan));
		if (remainder_sorted) {
			// The remainder of the standard is in zero-sorted order.
			encoder_->encode(*model, 0);
			break;
		}
		// The list is not in zero-sorted order: encode the next position:
		const auto pos = std::find(std::begin(standard_scan), std::end(standard_scan), zero_sorted_scan[i]);
		const int coded_pos = 1 + std::distance(std::begin(standard_scan), pos);
		standard_scan.erase(pos);

		encoder_->encode(*model, coded_pos);
		model->shift_context(coded_pos);
	}
}

std::array<std::uint8_t, 64> PjgEncoder::get_zero_sorted_scan(const Component& component) const {
	// Preset the unsorted scan index:
	std::array<std::uint8_t, 64> index;
	std::iota(std::begin(index), std::end(index), std::uint8_t(0));

	// Count the number of zeroes for each frequency:
	std::array<std::size_t, 64> zero_dist; // Distribution of zeroes per band.
	std::transform(std::begin(component.colldata),
	               std::end(component.colldata),
	               std::begin(zero_dist),
	               [](const auto& freq) {
		               return std::count(std::begin(freq), std::end(freq), static_cast<int16_t>(0));
	               });

	// Sort in ascending order according to the number of zeroes per band:
	std::stable_sort(std::begin(index) + 1, // Skip the first (DC) element, since it should always be first.
	                 std::end(index),
	                 [&zero_dist](const auto& a, const auto& b) {
		                 return zero_dist[a] < zero_dist[b];
	                 }
	);
	return index;
}

void PjgEncoder::zdst_high(const Component& component, const std::vector<std::uint8_t>& zero_dist_list) {
	auto model = std::make_unique<UniversalModel>(49 + 1, 25 + 1, 1);
	const int w = component.bch;

	// Encode the zero-distribution-list:
	for (std::size_t dpos = 0; dpos < zero_dist_list.size(); dpos++) {
		// context modeling - use the average of above and left as context:
		auto coords = PjgContext::get_context_nnb(dpos, w);
		coords.first = (coords.first >= 0) ? zero_dist_list[coords.first] : 0;
		coords.second = (coords.second >= 0) ? zero_dist_list[coords.second] : 0;
		model->shift_context((coords.first + coords.second + 2) / 4);

		encoder_->encode(*model, zero_dist_list[dpos]);
	}
}

void PjgEncoder::zdst_low(const Component& component, const std::vector<std::uint8_t>& zero_dist_context, const std::vector<std::uint8_t>& zdstxlow, const std::vector<std::uint8_t>& zdstylow, const std::vector<std::uint8_t>& eob_x, const std::vector<std::uint8_t>& eob_y) {
	auto model = std::make_unique<UniversalModel>(8, 8, 2);
	auto encode_zero_dist = [&](const auto& zero_dist_list, const auto& eob_context) {
		for (std::size_t dpos = 0; dpos < zero_dist_list.size(); dpos++) {
			model->shift_model((zero_dist_context[dpos] + 3) / 7, eob_context[dpos]);
			encoder_->encode(*model, zero_dist_list[dpos]);
		}
	};
	
	// Encode the first row zero-distribution-list:
	encode_zero_dist(zdstxlow, eob_x);

	// Encode the first column zero-distribution-list:
	encode_zero_dist(zdstylow, eob_y);
}

void PjgEncoder::dc(const Component& component, const std::vector<std::uint8_t>& zero_dist_list) {
	const auto& segmentation_set = pjg::segm_tables[component.segm_cnt - 1];

	const int max_val = component.max_v(0);
	const int max_bitlen = pjg::bitlen1024p(max_val);

	auto bitlen_model = std::make_unique<UniversalModel>(max_bitlen + 1, std::max(component.segm_cnt, max_bitlen + 1), 2);
	auto residual_model = std::make_unique<BinaryModel>(std::max(component.segm_cnt, 16), 2);
	auto sign_model = std::make_unique<BinaryModel>(1, 0);

	// set width/height of each band
	const int bc = component.bc;
	const int band_width = component.bch;

	PjgContext context(component);

	const auto& coeffs = component.colldata[0];

	// arithmetic compression loop
	for (int dpos = 0; dpos < bc; dpos++) {
		const int segment_number = segmentation_set[zero_dist_list[dpos]];
		const int average_context = context.aavrg_context(dpos, band_width);
		const int bitlen_context = pjg::bitlen1024p(average_context);

		// shift context / do context modelling (segmentation is done per context)
		bitlen_model->shift_model(bitlen_context, segment_number);

		if (coeffs[dpos] == 0) {
			// Simple treatment if the coefficient is zero:
			// Encode the bitlength (i.e. 0) of the coefficient:	
			encoder_->encode(*bitlen_model, 0);
		} else {
			// Encode the bitlength of the current coefficient:
			const int coeff_abs = std::abs(coeffs[dpos]);
			const int coeff_bitlen = pjg::bitlen1024p(coeff_abs);
			encoder_->encode(*bitlen_model, coeff_bitlen);

			// Encode the residual of the current coefficient:
			// The highest-nonzero must be 1, so we start at bitlen - 2:
			this->encode_residual(*residual_model, coeff_abs, coeff_bitlen - 2, segment_number);

			// Encode the sign of the current coefficient:
			const int coeff_sign = (coeffs[dpos] > 0) ? 0 : 1;
			encoder_->encode(*sign_model, coeff_sign);

			context.abs_coeffs_[dpos] = coeff_abs;
		}
	}
}

std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>> PjgEncoder::ac_high(Component& component, std::vector<std::uint8_t>& zero_dist_list) {
	const auto& segm_tab = pjg::segm_tables[component.segm_cnt - 1];

	auto bitlen_model = std::make_unique<UniversalModel>(11, std::max(11, component.segm_cnt), 2);
	auto residual_model = std::make_unique<BinaryModel>(std::max(component.segm_cnt, 16), 2);
	auto sign_model = std::make_unique<BinaryModel>(9, 1);

	// set width/height of each band
	const int bc = component.bc;
	const int band_width = component.bch;

	std::vector<std::uint8_t> signs(bc); // sign storage for context	
	
	std::vector<std::uint8_t> eob_x(bc);
	std::vector<std::uint8_t> eob_y(bc);

	PjgContext context(component);

	// work through lower 7x7 bands in order of frequency
	for (int i = 1; i < 64; i++) {
		// work through blocks in order of frequency scan
		const int block = static_cast<int>(component.freqscan[i]);
		const int b_x = pjg::unzigzag[block] % 8;
		const int b_y = pjg::unzigzag[block] / 8;

		if (b_x == 0 || b_y == 0) {
			continue; // Only processing the lower 7x7 bands here: process the others elsewhere.
		}

		context.reset_store();
		std::fill(std::begin(signs), std::end(signs), static_cast<std::uint8_t>(0));

		const auto& coeffs = component.colldata[block];

		const int max_val = component.max_v(block);
		const int max_bitlen = pjg::bitlen1024p(max_val);

		for (int dpos = 0; dpos < coeffs.size(); dpos++) {
			// skip if beyound eob
			if (zero_dist_list[dpos] == 0) {
				continue;
			}

			// get segment-number from zero distribution list and segmentation set
			const int segment_number = segm_tab[zero_dist_list[dpos]];
			// calculate contexts (for bit length)
			const int average_context = context.aavrg_context(dpos, band_width);
			const int bitlen_context = pjg::bitlen1024p(average_context);
			// shift context / do context modelling (segmentation is done per context)
			bitlen_model->shift_model(bitlen_context, segment_number);
			bitlen_model->exclude_symbols_above(max_bitlen);

			if (coeffs[dpos] == 0) {
				// Simple treatment if the coefficient is zero:
				// Encode the bitlength (i.e. 0) of the coefficient:
				encoder_->encode(*bitlen_model, 0);
			} else {
				const int coeff_abs = std::abs(coeffs[dpos]);
				const int coeff_bitlen = pjg::bitlen1024p(coeff_abs);
				const int coeff_sign = (coeffs[dpos] > 0) ? 0 : 1;

				// Encode the bitlength of the current coefficient:			
				encoder_->encode(*bitlen_model, coeff_bitlen);

				// Encoding of residual:
				// The highest-nonzero must be 1, so we start at bitlen - 2:
				this->encode_residual(*residual_model, coeff_abs, coeff_bitlen - 2, segment_number);

				// Encode the sign of the current coefficient:
				const int p_y = dpos / band_width;
				const int p_x = dpos % band_width;
				int sign_context = (p_x > 0) ? signs[dpos - 1] : 0;
				if (p_y > 0) {
					sign_context += 3 * signs[dpos - band_width]; // IMPROVE !!!!!!!!!!!
				}
				sign_model->shift_context(sign_context);
				encoder_->encode(*sign_model, coeff_sign);

				// store absolute value/sign, decrement zdst
				context.abs_coeffs_[dpos] = coeff_abs;
				signs[dpos] = coeff_sign + 1;
				zero_dist_list[dpos]--;

				// recalculate x/y eob:
				eob_x[dpos] = std::max(static_cast<std::uint8_t>(b_x), eob_x[dpos]);
				eob_y[dpos] = std::max(static_cast<std::uint8_t>(b_y), eob_y[dpos]);
			}
		}
		bitlen_model->flush_model();
		residual_model->flush_model();
		sign_model->flush_model();
	}
	return std::make_pair(std::move(eob_x), std::move(eob_y));
}

void PjgEncoder::ac_low(Component& component, std::vector<std::uint8_t>& zdstxlow, std::vector<std::uint8_t>& zdstylow) {
	std::array<int16_t*, 8> coeffs_x{nullptr}; // prediction coeffs - current block
	std::array<int16_t*, 8> coeffs_a{nullptr}; // prediction coeffs - neighboring block
	std::array<int, 8> pred_cf{}; // prediction multipliers

	auto bitlen_model = std::make_unique<UniversalModel>(11, std::max(component.segm_cnt, 11), 2);
	auto residual_model = std::make_unique<BinaryModel>(1 << 4, 2);
	auto top_model = std::make_unique<BinaryModel>(1 << std::max(4, component.nois_trs), 3);
	auto sign_model = std::make_unique<BinaryModel>(11, 1);

	// set width/height of each band
	const int bc = component.bc;
	const int w = component.bch;

	// work through each first row / first column band
	for (int i = 2; i < 16; i++) {
		// alternate between first row and first column
		int b_x = (i % 2 == 0) ? i / 2 : 0;
		int b_y = (i % 2 == 1) ? i / 2 : 0;
		const int bpos = static_cast<int>(pjg::zigzag[b_x + (8 * b_y)]);

		const auto& coeffs = component.colldata[bpos]; // Current coefficent data.
		// store pointers to prediction coefficients
		int p_x, p_y;
		const int& edge_criterion = b_x == 0 ? p_x : p_y;
		auto& zero_dist_list = b_x == 0 ? zdstylow : zdstxlow; // Reference to row/col # of non-zeroes.
		if (b_x == 0) {
			for (; b_x < 8; b_x++) {
				const auto block = pjg::zigzag[b_x + (8 * b_y)];
				coeffs_x[b_x] = component.colldata[block].data();
				coeffs_a[b_x] = component.colldata[block].data() - 1;
				pred_cf[b_x] = dct::icos_base_8x8[b_x * 8] * component.quant(block);
			}
		} else { // if ( b_y == 0 )
			for (; b_y < 8; b_y++) {
				const auto block = pjg::zigzag[b_x + (8 * b_y)];
				coeffs_x[b_y] = component.colldata[block].data();
				coeffs_a[b_y] = component.colldata[block].data() - w;
				pred_cf[b_y] = dct::icos_base_8x8[b_y * 8] * component.quant(block);
			}
		}

		const int max_val = component.max_v(bpos);
		const int max_bitlen = pjg::bitlen1024p(max_val);
		const int bp_threshold = std::max(0, max_bitlen - component.nois_trs); // residual threshold bitplane	

		for (int dpos = 0; dpos < bc; dpos++) {
			// skip if beyound eob
			if (zero_dist_list[dpos] == 0) {
				continue;
			}

			// calculate x/y positions in band
			p_y = dpos / w;
			p_x = dpos % w;

			// edge treatment / calculate LAKHANI context
			int lakhani_context;
			if (edge_criterion > 0) {
				lakhani_context = PjgContext::lakh_context(coeffs_x, coeffs_a, pred_cf, dpos);
			} else {
				lakhani_context = 0;
			}
			lakhani_context = bitops::clamp(lakhani_context, -max_val, max_val);
			const int bitlen_context = pjg::bitlen2048n(lakhani_context);

			// shift context / do context modelling (segmentation is done per context)
			bitlen_model->shift_model(bitlen_context, zero_dist_list[dpos]);
			bitlen_model->exclude_symbols_above(max_bitlen);

			// simple treatment if coefficient is zero
			if (coeffs[dpos] == 0) {
				// encode bit length (0) of current coefficient
				encoder_->encode(*bitlen_model, 0);
			} else {
				const int coeff_abs = std::abs(coeffs[dpos]);
				const int coeff_bitlen = pjg::bitlen2048n(coeff_abs);
				const int coeff_sign = (coeffs[dpos] > 0) ? 0 : 1;

				// Encode the bitlength of the current coefficient:
				encoder_->encode(*bitlen_model, coeff_bitlen);

				// Encoding of residual:
				int bp = coeff_bitlen - 2; // The highest nonzero bit must be one, so we start at bitlen - 2
				int residual_context = (bp >= bp_threshold) ? 1 : 0; // Bitplane context for residual.
				const int absolute_context = std::abs(lakhani_context);
				for (; bp >= bp_threshold; bp--) {
					top_model->shift_model(absolute_context >> bp_threshold, residual_context, coeff_bitlen - bp_threshold);

					// Encode the next bit:
					const int bit = bitops::bitn(coeff_abs, bp);
					encoder_->encode(*top_model, bit);

					// Update context:
					residual_context = residual_context << 1;
					if (bit) {
						residual_context |= 1;
					}
				}
				encode_residual(*residual_model, coeff_abs, bp, zero_dist_list[dpos]);

				// Encode the sign of the current coefficient:
				const int sign_context = (lakhani_context == 0) ? 0 : (lakhani_context > 0) ? 1 : 2;
				sign_model->shift_model(bitlen_context, sign_context);
				encoder_->encode(*sign_model, coeff_sign);

				zero_dist_list[dpos]--;
			}
		}
		bitlen_model->flush_model();
		residual_model->flush_model();
		top_model->flush_model();
		sign_model->flush_model();
	}
}

void PjgEncoder::encode_residual(BinaryModel& model, int val_abs, int max_bit_pos, int val_context) {
	for (int bit_plane = max_bit_pos; bit_plane >= 0; bit_plane--) {
		model.shift_model(val_context, bit_plane);
		const int bit = bitops::bitn(val_abs, bit_plane);
		encoder_->encode(model, bit);
	}
}

void PjgEncoder::generic(const std::vector<Segment>& segments) {
	auto model = std::make_unique<UniversalModel>(256 + 1, 256, 1);

	for (const auto& segment : segments) {
		for (std::uint8_t byte : segment.get_data()) {
			encoder_->encode(*model, byte);
			model->shift_context(byte);
		}
	}
	// encode end-of-data symbol (256)
	encoder_->encode(*model, 256);
}

void PjgEncoder::generic(const std::vector<std::uint8_t>& data) {
	auto model = std::make_unique<UniversalModel>(256 + 1, 256, 1);

	for (std::uint8_t byte : data) {
		encoder_->encode(*model, byte);
		model->shift_context(byte);
	}
	// encode end-of-data symbol (256)
	encoder_->encode(*model, 256);
}

void PjgEncoder::bit(std::uint8_t bit) {
	// encode one bit
	auto model = std::make_unique<BinaryModel>(1, -1);
	encoder_->encode(*model, bit);
}