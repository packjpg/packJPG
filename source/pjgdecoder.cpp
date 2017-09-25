#include "pjgdecoder.h"

#include <algorithm>
#include <string>

#include "bitops.h"
#include "dct8x8.h"
#include "jfif.h"
#include "pjgcontext.h"
#include "pjpgtbl.h"
#include "programinfo.h"

PjgDecoder::PjgDecoder(Reader& decoding_stream) {
	const auto image_pjg_version = decoding_stream.read_byte();
	if (image_pjg_version >= 0x14) {
		if (image_pjg_version != program_info::appversion) {
			throw std::runtime_error("Incompatible file, use " + program_info::appname
				+ " v" + std::to_string(image_pjg_version / 10) + "." + std::to_string(image_pjg_version % 10));
		}
	} else {
		throw std::runtime_error("Unknown pjg version, use newer version of " + program_info::appname);
	}
	decoder_ = std::make_unique<ArithmeticDecoder>(decoding_stream);
}

void PjgDecoder::decode() {
	segments_ = Segment::parse_segments(this->generic());
	padbit_ = this->bit();
	const bool rst_err_used = this->bit() == 1;
	if (rst_err_used) {
		// Decode the number of false set RST markers per scan only if available:
		rst_err_ = this->generic();
	}

	for (auto& segment : segments_) {
		segment.undo_optimize();
	}
	frame_info_ = jfif::get_frame_info(segments_);

	for (auto& component : frame_info_->components) {
		const auto zero_sorted_scan = this->decode_zero_sorted_scan();
		const auto zero_dist_data = this->zdst_high(component);
		const auto eob_data = this->ac_high(component, zero_sorted_scan, std::vector<std::uint8_t>(zero_dist_data));
		auto lower_zero_dist_lists = this->zdst_low(component, zero_dist_data, eob_data.first, eob_data.second);
		this->ac_low(component, lower_zero_dist_lists.first, lower_zero_dist_lists.second);
		this->decode_dc(component, zero_dist_data);
	}

	const bool garbage_exists = this->bit() == 1;
	if (garbage_exists) {
		garbage_data_ = this->generic();
	}
}

std::unique_ptr<FrameInfo> PjgDecoder::get_frame_info() {
	return std::move(frame_info_);
}

std::vector<Segment> PjgDecoder::get_segments() const {
	return segments_;
}

std::uint8_t PjgDecoder::get_padbit() const {
	return padbit_;
}

std::vector<std::uint8_t> PjgDecoder::get_rst_err() const {
	return rst_err_;
}

std::vector<std::uint8_t> PjgDecoder::get_garbage_data() const {
	return garbage_data_;
}

std::array<std::uint8_t, 64> PjgDecoder::decode_zero_sorted_scan() {
	std::array<std::uint8_t, 64> zero_sorted_scan{};
	std::vector<std::uint8_t> standard_scan(std::begin(pjg::stdscan), std::end(pjg::stdscan));

	auto model = std::make_unique<UniversalModel>(64, 64, 1);

	// Decode the zero-sorted scan order:
	for (int i = 1; i < zero_sorted_scan.size(); i++) {
		model->exclude_symbols_above(64 - i);

		int coded_pos = decoder_->decode(*model);
		model->shift_context(coded_pos);

		if (coded_pos == 0) {
			// The remainder of the zero-sorted scan is identical to the standard scan:
			std::copy(std::begin(standard_scan) + 1, std::end(standard_scan), std::begin(zero_sorted_scan) + i);
			break;
		}
		zero_sorted_scan[i] = standard_scan[coded_pos];
		standard_scan.erase(std::begin(standard_scan) + coded_pos);
	}

	return zero_sorted_scan;
}

std::vector<std::uint8_t> PjgDecoder::zdst_high(const Component& component) {
	auto model = std::make_unique<UniversalModel>(49 + 1, 25 + 1, 1);
	std::vector<std::uint8_t> zero_dist_list(component.bc);
	const int band_width = component.bch;

	// Decode the zero-distribution-list:
	for (int pos = 0; pos < zero_dist_list.size(); pos++) {
		// Context modeling: use the average of above and left as context:	
		auto neighbors = PjgContext::get_context_nnb(pos, band_width);
		neighbors.first = (neighbors.first >= 0) ? zero_dist_list[neighbors.first] : 0;
		neighbors.second = (neighbors.second >= 0) ? zero_dist_list[neighbors.second] : 0;
		model->shift_context((neighbors.first + neighbors.second + 2) / 4);

		zero_dist_list[pos] = decoder_->decode(*model);
	}
	return zero_dist_list;
}

std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>> PjgDecoder::zdst_low(const Component& component, const std::vector<std::uint8_t>& zero_dist_context, const std::vector<std::uint8_t>& eob_x, const std::vector<std::uint8_t>& eob_y) {
	auto model = std::make_unique<UniversalModel>(8, 8, 2);

	auto decode_zero_dist_list = [&](const auto& eob_context, auto& zero_dist_list) {
		for (int dpos = 0; dpos < zero_dist_list.size(); dpos++) {
			model->shift_model((zero_dist_context[dpos] + 3) / 7, eob_context[dpos]);
			zero_dist_list[dpos] = decoder_->decode(*model);
		}
	};

	std::vector<std::uint8_t> first_row_zero_dist_list(component.bc);
	decode_zero_dist_list(eob_x, first_row_zero_dist_list);
	std::vector<std::uint8_t> first_col_zero_dist_list(component.bc);
	decode_zero_dist_list(eob_y, first_col_zero_dist_list);

	return std::make_pair(std::move(first_row_zero_dist_list), std::move(first_col_zero_dist_list));
}

void PjgDecoder::decode_dc(Component& component, const std::vector<std::uint8_t>& zero_dist_list) {
	const int max_val = component.max_v(0);
	const int max_bitlen = pjg::bitlen1024p(max_val);

	auto bitlen_model = std::make_unique<UniversalModel>(max_bitlen + 1, std::max(component.segm_cnt, max_bitlen + 1), 2);
	auto residual_model = std::make_unique<BinaryModel>(std::max(component.segm_cnt, 16), 2);
	auto sign_model = std::make_unique<BinaryModel>(1, 0);

	const auto& segmentation_set = pjg::segm_tables[component.segm_cnt - 1];
	PjgContext context(component);

	auto& dc_coeffs = component.colldata[0];
	for (int pos = 0; pos < dc_coeffs.size(); pos++) {
		const int segment_num = segmentation_set[zero_dist_list[pos]];
		const int average_context = context.aavrg_context(pos, component.bch);
		const int bitlen_context = pjg::bitlen1024p(average_context);
		// Do context modeling (segmentation is done per context):
		bitlen_model->shift_model(bitlen_context, segment_num);

		const int coeff_bitlen = decoder_->decode(*bitlen_model);
		if (coeff_bitlen != 0) {
			// The highest nonzero bit of the residual is one, so we start at bitlen - 2:
			const int coeff_residual = this->decode_residual(*residual_model, coeff_bitlen - 2, segment_num);
			const bool coeff_is_positive = decoder_->decode(*sign_model) == 0;
			dc_coeffs[pos] = coeff_is_positive ? coeff_residual : -coeff_residual;
			context.abs_coeffs_[pos] = coeff_residual;
		}
	}
}

std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>> PjgDecoder::ac_high(Component& component, const std::array<std::uint8_t, 64>& zero_sorted_scan, std::vector<std::uint8_t>&& zero_dist_list) {
	const auto& segmentation_set = pjg::segm_tables[component.segm_cnt - 1];

	auto bitlen_model = std::make_unique<UniversalModel>(11, std::max(component.segm_cnt, 11), 2);
	auto residual_model = std::make_unique<BinaryModel>(std::max(component.segm_cnt, 16), 2);
	auto sign_model = std::make_unique<BinaryModel>(9, 1);

	const int bc = component.bc;
	const int band_width = component.bch;

	std::vector<std::uint8_t> signs(bc);

	std::vector<std::uint8_t> eob_x(bc);
	std::vector<std::uint8_t> eob_y(bc);

	PjgContext context(component);

	// work through lower 7x7 bands in zero-sorted scan order:
	for (int i = 1; i < 64; i++) {
		// work through blocks in order of frequency scan
		const int bpos = static_cast<int>(zero_sorted_scan[i]);
		const int b_x = pjg::unzigzag[bpos] % 8;
		const int b_y = pjg::unzigzag[bpos] / 8;

		if ((b_x == 0) || (b_y == 0)) {
			continue; // process remaining coefficients elsewhere
		}

		// preset absolute values/sign storage
		context.reset_store();
		std::fill(std::begin(signs), std::end(signs), static_cast<std::uint8_t>(0));

		auto& coeffs = component.colldata[bpos];

		const int max_val = component.max_v(bpos);
		const int max_bitlen = pjg::bitlen1024p(max_val);

		for (int pos = 0; pos < coeffs.size(); pos++) {
			if (zero_dist_list[pos] == 0) {
				continue; // skip if beyound eob
			}

			const int segment_num = segmentation_set[zero_dist_list[pos]];

			const int average_context = context.aavrg_context(pos, band_width);
			const int bitlen_context = pjg::bitlen1024p(average_context);
			// shift context / do context modelling (segmentation is done per context)
			bitlen_model->shift_model(bitlen_context, segment_num);
			bitlen_model->exclude_symbols_above(max_bitlen);

			const int coeff_bitlen = decoder_->decode(*bitlen_model);

			if (coeff_bitlen != 0) {
				// The highest nonzero bit of the residual is one, so we start at bitlen - 2:
				const int coeff_residual = this->decode_residual(*residual_model, coeff_bitlen - 2, segment_num);

				// Decode the sign of the coefficient:
				const int p_y = pos / band_width;
				const int p_x = pos % band_width;
				int sign_context = p_x > 0 ? signs[pos - 1] : 0;
				if (p_y > 0) {
					sign_context += 3 * signs[pos - band_width]; // IMPROVE! !!!!!!!!!!!
				}
				sign_model->shift_context(sign_context);
				const int coeff_sign = decoder_->decode(*sign_model);

				coeffs[pos] = (coeff_sign == 0) ? coeff_residual : -coeff_residual;
				context.abs_coeffs_[pos] = coeff_residual;
				signs[pos] = coeff_sign + 1;
				zero_dist_list[pos]--;

				eob_x[pos] = std::max(eob_x[pos], static_cast<std::uint8_t>(b_x));
				eob_y[pos] = std::max(eob_y[pos], static_cast<std::uint8_t>(b_y));
			}
		}
		bitlen_model->flush_model();
		residual_model->flush_model();
		sign_model->flush_model();
	}

	return std::make_pair(std::move(eob_x), std::move(eob_y));
}

void PjgDecoder::ac_low(Component& component, std::vector<std::uint8_t>& zdstxlow, std::vector<std::uint8_t>& zdstylow) {
	std::array<int16_t*, 8> coeffs_x{nullptr}; // prediction coeffs - current block
	std::array<int16_t*, 8> coeffs_a{nullptr}; // prediction coeffs - neighboring block
	std::array<int, 8> pred_cf{}; // prediction multipliers

	auto bitlen_model = std::make_unique<UniversalModel>(11, std::max(component.segm_cnt, 11), 2);
	auto residual_model = std::make_unique<BinaryModel>(16, 2);
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

		auto& coeffs = component.colldata[bpos];
		// store pointers to prediction coefficients
		int p_x, p_y;
		int* edge_c; // edge criteria
		auto& zero_dist_list = b_x == 0 ? zdstylow : zdstxlow; // Reference to row/col # of non-zeroes.
		if (b_x == 0) {
			for (; b_x < 8; b_x++) {
				coeffs_x[b_x] = component.colldata[pjg::zigzag[b_x + (8 * b_y)]].data();
				coeffs_a[b_x] = component.colldata[pjg::zigzag[b_x + (8 * b_y)]].data() - 1;
				pred_cf[b_x] = dct::icos_base_8x8[b_x * 8] * component.quant(pjg::zigzag[b_x + (8 * b_y)]);
			}
			edge_c = &p_x;
		} else { // if ( b_y == 0 )
			for (; b_y < 8; b_y++) {
				coeffs_x[b_y] = component.colldata[pjg::zigzag[b_x + (8 * b_y)]].data();
				coeffs_a[b_y] = component.colldata[pjg::zigzag[b_x + (8 * b_y)]].data() - w;
				pred_cf[b_y] = dct::icos_base_8x8[b_y * 8] * component.quant(pjg::zigzag[b_x + (8 * b_y)]);
			}
			edge_c = &p_y;
		}

		const int max_val = component.max_v(bpos);
		const int max_bitlen = pjg::bitlen1024p(max_val);
		const int thrs_bp = std::max(0, max_bitlen - component.nois_trs); // Residual threshold bitplane.

		for (int pos = 0; pos < bc; pos++) {
			// skip if beyound eob
			if (zero_dist_list[pos] == 0) {
				continue;
			}

			//calculate x/y positions in band
			p_y = pos / w;
			p_x = pos % w;

			int lakhani_context;
			if ((*edge_c) > 0) {
				lakhani_context = PjgContext::lakh_context(coeffs_x, coeffs_a, pred_cf, pos);
			} else {
				lakhani_context = 0;
			}
			lakhani_context = bitops::clamp(lakhani_context, -max_val, max_val);

			// shift context / do context modelling (segmentation is done per context)
			const int bitlen_context = pjg::bitlen2048n(lakhani_context);
			bitlen_model->shift_model(bitlen_context, zero_dist_list[pos]);
			bitlen_model->exclude_symbols_above(max_bitlen);

			const int coeff_bitlen = decoder_->decode(*bitlen_model);
			if (coeff_bitlen != 0) {
				// Decode the residual of the coefficient:
				int bp = coeff_bitlen - 2; // first set bit must be 1, so we start at clen - 2
				int residual_context = (bp >= thrs_bp) ? 1 : 0; // Bit plane context for residual.
				const int absolute_context = std::abs(lakhani_context);
				for (; bp >= thrs_bp; bp--) {
					top_model->shift_model(absolute_context >> thrs_bp, residual_context, coeff_bitlen - thrs_bp);
					const int bit = decoder_->decode(*top_model);
					residual_context = residual_context << 1;
					if (bit) {
						residual_context |= 1;
					}
				}
				int initial_coeff_residual = (residual_context == 0) ? 1 : residual_context; // !!!!
				const int coeff_residual = this->decode_residual(*residual_model, bp, zero_dist_list[pos], initial_coeff_residual);

				// Decode the sign of the coefficient:
				const int sign_context = (lakhani_context == 0) ? 0 : (lakhani_context > 0) ? 1 : 2;
				sign_model->shift_model(zero_dist_list[pos], sign_context);
				const int sign = decoder_->decode(*sign_model);

				coeffs[pos] = (sign == 0) ? coeff_residual : -coeff_residual;

				zero_dist_list[pos]--;
			}
		}
		bitlen_model->flush_model();
		residual_model->flush_model();
		top_model->flush_model();
		sign_model->flush_model();
	}
}

std::vector<std::uint8_t> PjgDecoder::generic() {
	std::vector<std::uint8_t> generic_data;
	auto model = std::make_unique<UniversalModel>(256 + 1, 256, 1);
	for (int c = decoder_->decode(*model); c != 256; c = decoder_->decode(*model)) {
		generic_data.emplace_back(static_cast<std::uint8_t>(c));
		model->shift_context(c);
	}

	return generic_data;
}

std::uint8_t PjgDecoder::bit() {
	auto model = std::make_unique<BinaryModel>(1, -1);
	std::uint8_t bit = decoder_->decode(*model); // This conversion is okay since there are only 2 symbols in the model.
	return bit;
}

int PjgDecoder::decode_residual(BinaryModel& residual_model, int starting_bit, int context, int initial_residual) {
	int residual = initial_residual;
	for (int bit_plane = starting_bit; bit_plane >= 0; bit_plane--) {
		residual_model.shift_model(context, bit_plane);
		const int bit = decoder_->decode(residual_model);
		residual <<= 1;
		if (bit) {
			residual |= 1;
		}
	}
	return residual;
}