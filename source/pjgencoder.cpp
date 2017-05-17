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
		component.freqscan = this->zstscan(component);
		this->zdst_high(component);
		this->ac_high(component);
		this->zdst_low(component);
		this->ac_low(component);
		this->dc(component);
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

std::array<std::uint8_t, 64> PjgEncoder::zstscan(const Component& component) {
	// calculate zero sort scan
	const auto zero_sorted_scan = this->get_zerosort_scan(component);

	// preset freqlist
	std::array<std::uint8_t, 64> freqlist;
	std::copy(std::begin(pjg::stdscan), std::end(pjg::stdscan), std::begin(freqlist));

	// init model
	auto model = std::make_unique<UniversalModel>(64, 64, 1);

	// encode scanorder
	for (int i = 1; i < 64; i++) {
		// reduce range of model
		model->exclude_symbols_above(64 - i);

		// compare remaining list to remainnig scan
		int tpos = 0; // True position.
		int c;
		for (c = i; c < 64; c++) {
			// search next val != 0 in list
			tpos++;
			while (freqlist[tpos] == 0 && tpos < freqlist.size()) {
				tpos++;
			}
			// get out if not a match
			if (freqlist[tpos] != zero_sorted_scan[c]) {
				break;
			}
		}
		if (c == 64) {
			// remaining list is in sorted scanorder
			// encode zero and make a quick exit
			encoder_->encode(*model, 0);
			break;
		}

		// list is not in sorted order -> next pos hat to be encoded
		int cpos = 1; // Coded position.
		// encode position
		for (tpos = 0; freqlist[tpos] != zero_sorted_scan[i]; tpos++) {
			if (freqlist[tpos] != 0) {
				cpos++;
			}
		}
		// remove from list
		freqlist[tpos] = 0;

		// encode coded position in list
		encoder_->encode(*model, cpos);
		model->shift_context(cpos);
	}

	return zero_sorted_scan;
}

void PjgEncoder::zdst_high(const Component& component) {
	auto model = std::make_unique<UniversalModel>(49 + 1, 25 + 1, 1);
	const auto& zdstls = component.zdstdata;
	const int w = component.bch;

	// arithmetic encode zero-distribution-list
	for (std::size_t dpos = 0; dpos < zdstls.size(); dpos++) {
		// context modelling - use average of above and left as context
		auto coords = PjgContext::get_context_nnb(dpos, w);
		coords.first = (coords.first >= 0) ? zdstls[coords.first] : 0;
		coords.second = (coords.second >= 0) ? zdstls[coords.second] : 0;
		// shift context
		model->shift_context((coords.first + coords.second + 2) / 4);
		// encode symbol
		encoder_->encode(*model, zdstls[dpos]);
	}
}

void PjgEncoder::zdst_low(const Component& component) {
	auto model = std::make_unique<UniversalModel>(8, 8, 2);
	const auto& zdstls_x = component.zdstxlow;
	const auto& zdstls_y = component.zdstylow;
	const auto& ctx_eobx = component.eobxhigh;
	const auto& ctx_eoby = component.eobyhigh;
	const auto& ctx_zdst = component.zdstdata;
	const int bc = component.bc;

	// arithmetic encode zero-distribution-list (first row)
	for (int dpos = 0; dpos < bc; dpos++) {
		model->shift_context((ctx_zdst[dpos] + 3) / 7); // shift context
		model->shift_context(ctx_eobx[dpos]); // shift context
		encoder_->encode(*model, zdstls_x[dpos]); // encode symbol
	}
	// arithmetic encode zero-distribution-list (first collumn)
	for (int dpos = 0; dpos < bc; dpos++) {
		model->shift_context((ctx_zdst[dpos] + 3) / 7); // shift context
		model->shift_context(ctx_eoby[dpos]); // shift context
		encoder_->encode(*model, zdstls_y[dpos]); // encode symbol
	}
}

void PjgEncoder::dc(const Component& component) {
	const auto& segmentation_set = pjg::segm_tables[component.segm_cnt - 1];

	const int max_val = component.max_v(0);
	const int max_bitlen = pjg::bitlen1024p(max_val);

	auto bitlen_model = std::make_unique<UniversalModel>(max_bitlen + 1, std::max(component.segm_cnt, max_bitlen + 1), 2);
	auto residual_model = std::make_unique<BinaryModel>(std::max(component.segm_cnt, 16), 2);
	auto sign_model = std::make_unique<BinaryModel>(1, 0);

	// set width/height of each band
	const int bc = component.bc;
	const int w = component.bch;

	PjgContext context(component);

	const auto& coeffs = component.colldata[0];
	const auto& zero_dist_list = component.zdstdata;

	// arithmetic compression loop
	for (int dpos = 0; dpos < bc; dpos++) {
		//calculate x/y positions in band
		const int p_y = dpos / w;
		const int p_x = dpos % w;
		const int r_x = w - (p_x + 1);

		const int segment_number = segmentation_set[zero_dist_list[dpos]];
		
		const int average_context = context.aavrg_context(dpos, p_y, p_x, r_x);
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

void PjgEncoder::ac_high(Component& component) {
	const auto& segm_tab = pjg::segm_tables[component.segm_cnt - 1];

	auto bitlen_model = std::make_unique<UniversalModel>(11, std::max(11, component.segm_cnt), 2);
	auto residual_model = std::make_unique<BinaryModel>(std::max(component.segm_cnt, 16), 2);
	auto sign_model = std::make_unique<BinaryModel>(9, 1);

	// set width/height of each band
	const int bc = component.bc;
	const int w = component.bch;

	std::vector<std::uint8_t> signs(bc); // sign storage for context	
	auto zero_dist_list = component.zdstdata; // copy of zero distribution list
	
	auto& eob_x = component.eobxhigh;
	auto& eob_y = component.eobyhigh;

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

		for (int dpos = 0; dpos < bc; dpos++) {
			// skip if beyound eob
			if (zero_dist_list[dpos] == 0) {
				continue;
			}

			//calculate x/y positions in band
			const int p_y = dpos / w;
			const int p_x = dpos % w;
			const int r_x = w - (p_x + 1);

			// get segment-number from zero distribution list and segmentation set
			const int segment_number = segm_tab[zero_dist_list[dpos]];
			// calculate contexts (for bit length)
			const int average_context = context.aavrg_context(dpos, p_y, p_x, r_x);
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
				int sign_context = (p_x > 0) ? signs[dpos - 1] : 0;
				if (p_y > 0) {
					sign_context += 3 * signs[dpos - w]; // IMPROVE !!!!!!!!!!!
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
}

void PjgEncoder::ac_low(Component& component) {
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
		auto& zero_dist_list = b_x == 0 ? component.zdstylow : component.zdstxlow; // Reference to row/col # of non-zeroes.
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

std::array<std::uint8_t, 64> PjgEncoder::get_zerosort_scan(const Component& component) {
	// Preset the unsorted scan index:
	std::array<std::uint8_t, 64> index;
	std::iota(std::begin(index), std::end(index), std::uint8_t(0)); // Initialize the unsorted scan with indices 0, 1, ..., 63.

																	// Count the number of zeroes for each frequency:
	std::array<std::size_t, 64> zeroDist; // Distribution of zeroes per band.
	std::transform(std::begin(component.colldata),
		std::end(component.colldata),
		std::begin(zeroDist),
		[&](const auto& freq) {
		return std::count(std::begin(freq), std::end(freq), static_cast<int16_t>(0));
	});

	// Sort in ascending order according to the number of zeroes per band:
	std::stable_sort(std::begin(index) + 1, // Skip the first element.
		std::end(index),
		[&](const auto& a, const auto& b) {
		return zeroDist[a] < zeroDist[b];
	}
	);
	return index;
}