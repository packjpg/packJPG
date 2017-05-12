#include "pjgencoder.h"

#include <numeric>
#include <algorithm>

#include "bitops.h"
#include "dct8x8.h"
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

	// set padbit to 1 if previously unset:
	if (padbit == -1) {
		padbit = 1;
	}

	// optimize header for compression
	for (auto& segment : segments) {
		segment.optimize();
	}
	// encode JPG header
	this->generic(segments);

	// store padbit (padbit can't be retrieved from the header)
	this->bit(padbit);

	// also encode one bit to signal false/correct use of RST markers
	this->bit(rst_err.empty() ? 0 : 1);
	// encode # of false set RST markers per scan
	if (!rst_err.empty()) {
		this->generic(rst_err);
	}

	// encode actual components data
	for (auto& component : components) {
		// encode frequency scan ('zero-sort-scan')
		component.freqscan = this->zstscan(component); // set zero sort scan as freqscan
		// encode zero-distribution-lists for higher (7x7) ACs
		this->zdst_high(component);
		// encode coefficients for higher (7x7) ACs
		this->ac_high(component);
		// encode zero-distribution-lists for lower ACs
		this->zdst_low(component);
		// encode coefficients for first row / collumn ACs
		this->ac_low(component);
		// encode coefficients for DC
		this->dc(component);
	}

	// encode checkbit for garbage (0 if no garbage, 1 if garbage has to be coded)
	this->bit(!garbage_data.empty() ? 1 : 0);
	// encode garbage data only if needed
	if (!garbage_data.empty()) {
		this->generic(garbage_data);
	}

	encoder_->finalize();

	// errormessage if write error
	if (encoder_->error()) {
		throw std::runtime_error("write error, possibly drive is full");
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
		model->exclude_symbols(64 - i);

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
	// init model, constants
	auto model = std::make_unique<UniversalModel>(49 + 1, 25 + 1, 1);
	const auto& zdstls = component.zdstdata;
	const int w = component.bch;

	// arithmetic encode zero-distribution-list
	for (std::size_t dpos = 0; dpos < zdstls.size(); dpos++) {
		// context modelling - use average of above and left as context
		auto coords = context_.get_context_nnb(dpos, w);
		coords.first = (coords.first >= 0) ? zdstls[coords.first] : 0;
		coords.second = (coords.second >= 0) ? zdstls[coords.second] : 0;
		// shift context
		model->shift_context((coords.first + coords.second + 2) / 4);
		// encode symbol
		encoder_->encode(*model, zdstls[dpos]);
	}
}

void PjgEncoder::zdst_low(const Component& component) {
	// init model, constants
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
	std::array<std::uint16_t*, 6> c_absc{nullptr}; // quick access array for contexts

	// decide segmentation setting
	const auto& segm_tab = pjg::segm_tables[component.segm_cnt - 1];

	// get max absolute value/bit length
	const int max_val = component.max_v(0); // Max value.
	const int max_len = pjg::bitlen1024p(max_val); // Max bitlength.

	// init models for bitlenghts and -patterns	
	auto mod_len = std::make_unique<UniversalModel>(max_len + 1, std::max(component.segm_cnt, max_len + 1), 2);
	auto mod_res = std::make_unique<BinaryModel>(std::max(component.segm_cnt, 16), 2);
	auto mod_sgn = std::make_unique<BinaryModel>(1, 0);

	// set width/height of each band
	const int bc = component.bc;
	const int w = component.bch;

	// allocate memory for absolute values storage
	std::vector<std::uint16_t> absv_store(bc); // absolute coefficients values storage

	// set up context quick access array
	context_.aavrg_prepare(c_absc, absv_store.data(), component);

	// locally store pointer to coefficients and zero distribution list
	const auto& coeffs = component.colldata[0]; // Pointer to current coefficent data.
	const auto& zdstls = component.zdstdata; // Pointer to zero distribution list.

	// arithmetic compression loop
	for (int dpos = 0; dpos < bc; dpos++) {
		//calculate x/y positions in band
		const int p_y = dpos / w;
		// r_y = h - ( p_y + 1 );
		const int p_x = dpos % w;
		const int r_x = w - (p_x + 1);

		// get segment-number from zero distribution list and segmentation set
		const int snum = segm_tab[zdstls[dpos]];
		// calculate contexts (for bit length)
		const int ctx_avr = context_.aavrg_context(c_absc, dpos, p_y, p_x, r_x); // Average context
		const int ctx_len = pjg::bitlen1024p(ctx_avr); // Bitlength context.
		// shift context / do context modelling (segmentation is done per context)
		mod_len->shift_model(ctx_len, snum);

		// simple treatment if coefficient is zero
		if (coeffs[dpos] == 0) {
			// encode bit length (0) of current coefficient			
			encoder_->encode(*mod_len, 0);
		} else {
			// get absolute val, sign & bit length for current coefficient
			const int absv = std::abs(coeffs[dpos]);
			const int clen = pjg::bitlen1024p(absv);
			const int sgn = (coeffs[dpos] > 0) ? 0 : 1;
			// encode bit length of current coefficient
			encoder_->encode(*mod_len, clen);
			// encoding of residual
			// first set bit must be 1, so we start at clen - 2
			for (int bp = clen - 2; bp >= 0; bp--) {
				mod_res->shift_model(snum, bp); // shift in 2 contexts
				// encode/get bit
				const int bt = bitops::bitn(absv, bp);
				encoder_->encode(*mod_res, bt);
			}
			// encode sign
			encoder_->encode(*mod_sgn, sgn);
			// store absolute value
			absv_store[dpos] = absv;
		}
	}
}

void PjgEncoder::ac_high(Component& component) {
	std::array<std::uint16_t*, 6> c_absc{nullptr}; // quick access array for contexts

	// decide segmentation setting
	const auto& segm_tab = pjg::segm_tables[component.segm_cnt - 1];

	// init models for bitlenghts and -patterns
	auto mod_len = std::make_unique<UniversalModel>(11, std::max(11, component.segm_cnt), 2);
	auto mod_res = std::make_unique<BinaryModel>(std::max(component.segm_cnt, 16), 2);
	auto mod_sgn = std::make_unique<BinaryModel>(9, 1);

	// set width/height of each band
	const int bc = component.bc;
	const int w = component.bch;

	// allocate memory for absolute values & signs storage
	std::vector<std::uint16_t> absv_store(bc); // absolute coefficients values storage
	std::vector<std::uint8_t> sgn_store(bc); // sign storage for context	
	auto zdstls = component.zdstdata; // copy of zero distribution list

	// set up quick access arrays for signs context
	std::uint8_t* sgn_nbh = sgn_store.data() - 1; // Left signs neighbor.
	std::uint8_t* sgn_nbv = sgn_store.data() - w; // Upper signs neighbor.

	// locally store pointer to eob x / eob y
	auto& eob_x = component.eobxhigh; // Pointer to x eobs.
	auto& eob_y = component.eobyhigh; // Pointer to y eobs.

	// set up average context quick access arrays
	context_.aavrg_prepare(c_absc, absv_store.data(), component);

	// work through lower 7x7 bands in order of pjg::freqscan
	for (int i = 1; i < 64; i++) {
		// work through blocks in order of frequency scan
		const int bpos = static_cast<int>(component.freqscan[i]);
		const int b_x = pjg::unzigzag[bpos] % 8;
		const int b_y = pjg::unzigzag[bpos] / 8;

		if ((b_x == 0) || (b_y == 0))
			continue; // process remaining coefficients elsewhere

		// preset absolute values/sign storage
		std::fill(std::begin(absv_store), std::end(absv_store), static_cast<std::uint16_t>(0));
		std::fill(std::begin(sgn_store), std::end(sgn_store), static_cast<std::uint8_t>(0));

		// locally store pointer to coefficients
		const auto& coeffs = component.colldata[bpos]; // Pointer to current coefficent data.

		// get max bit length
		const int max_val = component.max_v(bpos); // Max value.
		const int max_len = pjg::bitlen1024p(max_val); // Max bitlength.

		// arithmetic compression loo
		for (int dpos = 0; dpos < bc; dpos++) {
			// skip if beyound eob
			if (zdstls[dpos] == 0)
				continue;

			//calculate x/y positions in band
			const int p_y = dpos / w;
			const int p_x = dpos % w;
			const int r_x = w - (p_x + 1);

			// get segment-number from zero distribution list and segmentation set
			const int snum = segm_tab[zdstls[dpos]];
			// calculate contexts (for bit length)
			const int ctx_avr = context_.aavrg_context(c_absc, dpos, p_y, p_x, r_x); // Average context.
			const int ctx_len = pjg::bitlen1024p(ctx_avr); // Bitlength context.
			// shift context / do context modelling (segmentation is done per context)
			mod_len->shift_model(ctx_len, snum);
			mod_len->exclude_symbols(max_len);

			// simple treatment if coefficient is zero
			if (coeffs[dpos] == 0) {
				// encode bit length (0) of current coefficien
				encoder_->encode(*mod_len, 0);
			} else {
				// get absolute val, sign & bit length for current coefficient
				const int absv = std::abs(coeffs[dpos]);
				const int clen = pjg::bitlen1024p(absv);
				const int sgn = (coeffs[dpos] > 0) ? 0 : 1;
				// encode bit length of current coefficient				
				encoder_->encode(*mod_len, clen);
				// encoding of residual
				// first set bit must be 1, so we start at clen - 2
				for (int bp = clen - 2; bp >= 0; bp--) {
					mod_res->shift_model(snum, bp); // shift in 2 contexts
					// encode/get bit
					const int bt = bitops::bitn(absv, bp);
					encoder_->encode(*mod_res, bt);
				}
				// encode sign				
				int ctx_sgn = (p_x > 0) ? sgn_nbh[dpos] : 0; // Sign context.
				if (p_y > 0)
					ctx_sgn += 3 * sgn_nbv[dpos]; // IMPROVE !!!!!!!!!!!
				mod_sgn->shift_context(ctx_sgn);
				encoder_->encode(*mod_sgn, sgn);
				// store absolute value/sign, decrement zdst
				absv_store[dpos] = absv;
				sgn_store[dpos] = sgn + 1;
				zdstls[dpos]--;
				// recalculate x/y eob				
				if (b_x > eob_x[dpos])
					eob_x[dpos] = b_x;
				if (b_y > eob_y[dpos])
					eob_y[dpos] = b_y;
			}
		}
		// flush models
		mod_len->flush_model();
		mod_res->flush_model();
		mod_sgn->flush_model();
	}
}

void PjgEncoder::ac_low(Component& component) {
	std::array<int16_t*, 8> coeffs_x{nullptr}; // prediction coeffs - current block
	std::array<int16_t*, 8> coeffs_a{nullptr}; // prediction coeffs - neighboring block
	std::array<int, 8> pred_cf{}; // prediction multipliers

	// init models for bitlenghts and -patterns
	auto mod_len = std::make_unique<UniversalModel>(11, std::max(component.segm_cnt, 11), 2);
	auto mod_res = std::make_unique<BinaryModel>(1 << 4, 2);
	auto mod_top = std::make_unique<BinaryModel>(1 << std::max(4, component.nois_trs), 3);
	auto mod_sgn = std::make_unique<BinaryModel>(11, 1);

	// set width/height of each band
	const int bc = component.bc;
	const int w = component.bch;

	// work through each first row / first collumn band
	for (int i = 2; i < 16; i++) {
		// alternate between first row and first collumn
		int b_x = (i % 2 == 0) ? i / 2 : 0;
		int b_y = (i % 2 == 1) ? i / 2 : 0;
		const int bpos = static_cast<int>(pjg::zigzag[b_x + (8 * b_y)]);

		// locally store pointer to band coefficients
		const auto& coeffs = component.colldata[bpos]; // Pointer to current coefficent data.
		// store pointers to prediction coefficients
		int p_x, p_y;
		int* edge_c; // edge criteria
		auto& zdstls = b_x == 0 ? component.zdstylow : component.zdstxlow; // Pointer to row/col # of non-zeroes.
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

		// get max bit length / other info
		const int max_valp = component.max_v(bpos); // Max value (positive).
		const int max_valn = -max_valp; // Max value (negative).
		const int max_len = pjg::bitlen1024p(max_valp); // Max bitlength
		const int thrs_bp = std::max(0, max_len - component.nois_trs); // residual threshold bitplane	

		// arithmetic compression loop
		for (int dpos = 0; dpos < bc; dpos++) {
			// skip if beyound eob
			if (zdstls[dpos] == 0) {
				continue;
			}

			// calculate x/y positions in band
			p_y = dpos / w;
			p_x = dpos % w;

			// edge treatment / calculate LAKHANI context
			int ctx_lak; // lakhani context
			if ((*edge_c) > 0) {
				ctx_lak = context_.lakh_context(coeffs_x, coeffs_a, pred_cf, dpos);
			} else {
				ctx_lak = 0;
			}
			ctx_lak = bitops::clamp(ctx_lak, max_valn, max_valp);
			const int ctx_len = pjg::bitlen2048n(ctx_lak); // Context for bitlength.

			// shift context / do context modelling (segmentation is done per context)
			mod_len->shift_model(ctx_len, zdstls[dpos]);
			mod_len->exclude_symbols(max_len);

			// simple treatment if coefficient is zero
			if (coeffs[dpos] == 0) {
				// encode bit length (0) of current coefficient
				encoder_->encode(*mod_len, 0);
			} else {
				// get absolute val, sign & bit length for current coefficient
				const int absv = std::abs(coeffs[dpos]);
				const int clen = pjg::bitlen2048n(absv);
				const int sgn = (coeffs[dpos] > 0) ? 0 : 1;
				// encode bit length of current coefficient
				encoder_->encode(*mod_len, clen);
				// encoding of residual
				int bp = clen - 2; // first set bit must be 1, so we start at clen - 2
				int ctx_res = (bp >= thrs_bp) ? 1 : 0; // Bitplane context for residual.
				const int ctx_abs = std::abs(ctx_lak); // Absolute context.
				const int ctx_sgn = (ctx_lak == 0) ? 0 : (ctx_lak > 0) ? 1 : 2; // Context for sign.
				for (; bp >= thrs_bp; bp--) {
					mod_top->shift_model(ctx_abs >> thrs_bp, ctx_res, clen - thrs_bp); // shift in 3 contexts
					// encode/get bit
					const int bt = bitops::bitn(absv, bp);
					encoder_->encode(*mod_top, bt);
					// update context
					ctx_res = ctx_res << 1;
					if (bt)
						ctx_res |= 1;
				}
				for (; bp >= 0; bp--) {
					mod_res->shift_model(zdstls[dpos], bp); // shift in 2 contexts
					// encode/get bit
					const int bt = bitops::bitn(absv, bp);
					encoder_->encode(*mod_res, bt);
				}
				// encode sign
				mod_sgn->shift_model(ctx_len, ctx_sgn);
				encoder_->encode(*mod_sgn, sgn);
				// decrement # of non zeroes
				zdstls[dpos]--;
			}
		}
		// flush models
		mod_len->flush_model();
		mod_res->flush_model();
		mod_top->flush_model();
		mod_sgn->flush_model();
	}
}

void PjgEncoder::generic(const std::vector<Segment>& segments) {
	// arithmetic encode data
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
	// arithmetic encode data
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