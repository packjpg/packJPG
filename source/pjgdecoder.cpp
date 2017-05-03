#include "pjgdecoder.h"

#include <algorithm>
#include <string>

#include "jfifparse.h"
#include "programinfo.h"
#include "writer.h"

PjgDecoder::PjgDecoder(Reader& decoding_stream) {
	// check header codes ( maybe position in other function ? )
	while (true) {
		std::uint8_t hcode;
		try {
			hcode = decoding_stream.read_byte();
		} catch (const std::runtime_error&) {
			throw;
		}
		if (hcode >= 0x14) {
			// compare version number
			if (hcode != program_info::appversion) {
				throw std::runtime_error("incompatible file, use " + program_info::appname
					+ " v" + std::to_string(hcode / 10) + "." + std::to_string(hcode % 10));
			} else {
				break;
			}
		} else {
			throw std::runtime_error("unknown header code, use newer version of " + program_info::appname);
		}
	}


	// init arithmetic compression
	decoder_ = std::make_unique<ArithmeticDecoder>(decoding_stream);
}

void PjgDecoder::decode() {
	// decode JPG header
	segments_ = Segment::parse_segments(this->generic());
	// retrieve padbit from stream
	padbit_ = this->bit();
	// decode one bit that signals false /correct use of RST markers
	auto cb = this->bit();
	// decode # of false set RST markers per scan only if available
	if (cb == 1) {
		rst_err_ = this->generic();
	}

	// undo header optimizations
	for (auto& segment : segments_) {
		segment.undo_optimize();
	}
	// parse header for image-info
	try {
		frame_info_ = jfif::get_frame_info(segments_);
	}
	catch (const std::exception&) {
		throw;
	}

	// decode actual components data
	for (auto& component : frame_info_->components) {
		// decode frequency scan ('zero-sort-scan')
		component.freqscan = this->zstscan(); // set zero sort scan as freqscan
		// decode zero-distribution-lists for higher (7x7) ACs
		this->zdst_high(component);
		// decode coefficients for higher (7x7) ACs
		this->ac_high(component);
		// decode zero-distribution-lists for lower ACs
		this->zdst_low(component);
		// decode coefficients for first row / collumn ACs
		this->ac_low(component);
		// decode coefficients for DC
		this->dc(component);
	}

	// retrieve checkbit for garbage (0 if no garbage, 1 if garbage has to be coded)
	auto garbage_exists = this->bit() == 1;

	// decode garbage data only if available
	if (garbage_exists) {
		garbage_data_ = this->generic();
	}
}

std::unique_ptr<FrameInfo> PjgDecoder::get_frame_info() {
	return std::move(frame_info_);
}


std::vector<Segment> PjgDecoder::get_segments() {
	return segments_;
}


std::uint8_t PjgDecoder::get_padbit() {
	return padbit_;
}

std::vector<std::uint8_t> PjgDecoder::get_rst_err() {
	return rst_err_;
}

std::vector<std::uint8_t> PjgDecoder::get_garbage_data() {
	return garbage_data_;
}

std::array<std::uint8_t, 64> PjgDecoder::zstscan() {
	int tpos; // true position

	std::array<std::uint8_t, 64> zsrtscan;
	// set first position in zero sort scan
	zsrtscan[0] = 0;

	// preset freqlist
	std::array<std::uint8_t, 64> freqlist;
	std::copy(std::begin(pjg::stdscan), std::end(pjg::stdscan), std::begin(freqlist));

	// init model
	auto model = std::make_unique<UniversalModel>(64, 64, 1);

	// encode scanorder
	for (int i = 1; i < 64; i++) {
		// reduce range of model
		model->exclude_symbols(64 - i);

		// decode symbol
		int cpos = decoder_->decode(*model); // coded position	
		model->shift_context(cpos);

		if (cpos == 0) {
			// remaining list is identical to scan
			// fill the scan & make a quick exit				
			for (tpos = 0; i < 64; i++) {
				while (freqlist[++tpos] == 0);
				zsrtscan[i] = freqlist[tpos];
			}
			break;
		}

		// decode position from list
		for (tpos = 0; tpos < 64; tpos++) {
			if (freqlist[tpos] != 0)
				cpos--;
			if (cpos == 0)
				break;
		}

		// write decoded position to zero sort scan
		zsrtscan[i] = freqlist[tpos];
		// remove from list
		freqlist[tpos] = 0;
	}

	return zsrtscan;
}

void PjgDecoder::zdst_high(Component& component) {
	// init model, constants
	auto model = std::make_unique<UniversalModel>(49 + 1, 25 + 1, 1);
	auto& zdstls = component.zdstdata;
	const int w = component.bch;
	const int bc = component.bc;

	// arithmetic decode zero-distribution-list
	for (int dpos = 0; dpos < bc; dpos++) {
		// context modelling - use average of above and left as context		
		auto coords = context_.get_context_nnb(dpos, w);
		coords.first = (coords.first >= 0) ? zdstls[coords.first] : 0;
		coords.second = (coords.second >= 0) ? zdstls[coords.second] : 0;
		// shift context
		model->shift_context((coords.first + coords.second + 2) / 4);
		// decode symbol
		zdstls[dpos] = decoder_->decode(*model);
	}
}

void PjgDecoder::zdst_low(Component& component) {
	// init model, constants
	auto model = std::make_unique<UniversalModel>(8, 8, 2);

	auto& zdstls_x = component.zdstxlow;
	auto& zdstls_y = component.zdstylow;

	const auto& ctx_eobx = component.eobxhigh;
	const auto& ctx_eoby = component.eobyhigh;
	const auto& ctx_zdst = component.zdstdata;
	const int bc = component.bc;

	// arithmetic encode zero-distribution-list (first row)
	for (int dpos = 0; dpos < bc; dpos++) {
		model->shift_context((ctx_zdst[dpos] + 3) / 7); // shift context
		model->shift_context(ctx_eobx[dpos]); // shift context
		zdstls_x[dpos] = decoder_->decode(*model); // decode symbol
	}
	// arithmetic encode zero-distribution-list (first collumn)
	for (int dpos = 0; dpos < bc; dpos++) {
		model->shift_context((ctx_zdst[dpos] + 3) / 7); // shift context
		model->shift_context(ctx_eoby[dpos]); // shift context
		zdstls_y[dpos] = decoder_->decode(*model); // decode symbol
	}
}

void PjgDecoder::dc(Component& component) {
	std::array<std::uint16_t*, 6> c_absc{nullptr}; // quick access array for contexts
	const auto c_weight = context_.get_weights(); // weighting for contexts

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
	auto& coeffs = component.colldata[0]; // Pointer to current coefficent data.
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
		const int ctx_avr = context_.aavrg_context(c_absc, c_weight, dpos, p_y, p_x, r_x); // Average context
		const int ctx_len = pjg::bitlen1024p(ctx_avr); // Bitlength context				
		// shift context / do context modelling (segmentation is done per context)
		mod_len->shift_model(ctx_len, snum);
		// decode bit length of current coefficient
		const int clen = decoder_->decode(*mod_len);

		// simple treatment if coefficient is zero
		if (clen == 0) {
			// coeffs[ dpos ] = 0;
		} else {
			// decoding of residual
			int absv = 1;
			// first set bit must be 1, so we start at clen - 2
			for (int bp = clen - 2; bp >= 0; bp--) {
				mod_res->shift_model(snum, bp); // shift in 2 contexts
				// decode bit
				const int bt = decoder_->decode(*mod_res);
				// update absv
				absv = absv << 1;
				if (bt)
					absv |= 1;
			}
			// decode sign
			const int sgn = decoder_->decode(*mod_sgn);
			// copy to colldata
			coeffs[dpos] = (sgn == 0) ? absv : -absv;
			// store absolute value/sign
			absv_store[dpos] = absv;
		}
	}
}

void PjgDecoder::ac_high(Component& component) {
	std::array<std::uint16_t*, 6> c_absc{nullptr}; // quick access array for contexts
	const auto c_weight = context_.get_weights(); // weighting for contexts

	// decide segmentation setting
	const auto& segm_tab = pjg::segm_tables[component.segm_cnt - 1];

	// init models for bitlenghts and -patterns
	auto mod_len = std::make_unique<UniversalModel>(11, std::max(component.segm_cnt, 11), 2);
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
		auto& coeffs = component.colldata[bpos]; // Pointer to current coefficent data.

		// get max bit length
		const int max_val = component.max_v(bpos); // Max value.
		const int max_len = pjg::bitlen1024p(max_val); // Max bitlength.

		// arithmetic compression loop
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
			const int ctx_avr = context_.aavrg_context(c_absc, c_weight, dpos, p_y, p_x, r_x); // Average context.
			const int ctx_len = pjg::bitlen1024p(ctx_avr); // Bitlength context.
			// shift context / do context modelling (segmentation is done per context)
			mod_len->shift_model(ctx_len, snum);
			mod_len->exclude_symbols(max_len);

			// decode bit length of current coefficient
			const int clen = decoder_->decode(*mod_len);
			// simple treatment if coefficient is zero
			if (clen == 0) {
				// coeffs[ dpos ] = 0;
			} else {
				// decoding of residual
				int absv = 1;
				// first set bit must be 1, so we start at clen - 2
				for (int bp = clen - 2; bp >= 0; bp--) {
					mod_res->shift_model(snum, bp); // shift in 2 contexts
					// decode bit
					const int bt = decoder_->decode(*mod_res);
					// update absv
					absv = absv << 1;
					if (bt)
						absv |= 1;
				}
				// decode sign
				int ctx_sgn = (p_x > 0) ? sgn_nbh[dpos] : 0; // Sign context.
				if (p_y > 0)
					ctx_sgn += 3 * sgn_nbv[dpos]; // IMPROVE! !!!!!!!!!!!
				mod_sgn->shift_context(ctx_sgn);
				const int sgn = decoder_->decode(*mod_sgn);
				// copy to colldata
				coeffs[dpos] = (sgn == 0) ? absv : -absv;
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

void PjgDecoder::ac_low(Component& component) {
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
		auto& coeffs = component.colldata[bpos]; // Pointer to current coefficent data.
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
		const int max_len = pjg::bitlen1024p(max_valp); // Max bitlength.
		const int thrs_bp = std::max(0, max_len - component.nois_trs); // Residual threshold bitplane.

		// arithmetic compression loop
		for (int dpos = 0; dpos < bc; dpos++) {
			// skip if beyound eob
			if (zdstls[dpos] == 0)
				continue;

			//calculate x/y positions in band
			p_y = dpos / w;
			p_x = dpos % w;

			// edge treatment / calculate LAKHANI context
			int ctx_lak; // Lakhani context.
			if ((*edge_c) > 0)
				ctx_lak = context_.lakh_context(coeffs_x, coeffs_a, pred_cf, dpos);
			else
				ctx_lak = 0;
			ctx_lak = clamp(ctx_lak, max_valn, max_valp);
			const int ctx_len = pjg::bitlen2048n(ctx_lak); // Bitlength context.				
			// shift context / do context modelling (segmentation is done per context)
			mod_len->shift_model(ctx_len, zdstls[dpos]);
			mod_len->exclude_symbols(max_len);

			// decode bit length of current coefficient
			const int clen = decoder_->decode(*mod_len);
			// simple treatment if coefficients == 0
			if (clen == 0) {
				// coeffs[ dpos ] = 0;
			} else {
				// decoding of residual
				int bp = clen - 2; // first set bit must be 1, so we start at clen - 2
				int ctx_res = (bp >= thrs_bp) ? 1 : 0; // Bit plane context for residual.
				const int ctx_abs = std::abs(ctx_lak); // Absolute context.
				const int ctx_sgn = (ctx_lak == 0) ? 0 : (ctx_lak > 0) ? 1 : 2; // Context for sign.
				for (; bp >= thrs_bp; bp--) {
					mod_top->shift_model(ctx_abs >> thrs_bp, ctx_res, clen - thrs_bp); // shift in 3 contexts
					// decode bit
					const int bt = decoder_->decode(*mod_top);
					// update context
					ctx_res = ctx_res << 1;
					if (bt)
						ctx_res |= 1;
				}
				int absv = (ctx_res == 0) ? 1 : ctx_res; // !!!!
				for (; bp >= 0; bp--) {
					mod_res->shift_model(zdstls[dpos], bp); // shift in 2 contexts
					// decode bit
					const int bt = decoder_->decode(*mod_res);
					// update absv
					absv = absv << 1;
					if (bt)
						absv |= 1;
				}
				// decode sign
				mod_sgn->shift_model(zdstls[dpos], ctx_sgn);
				const int sgn = decoder_->decode(*mod_sgn);
				// copy to colldata
				coeffs[dpos] = (sgn == 0) ? absv : -absv;
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

std::vector<std::uint8_t> PjgDecoder::generic() {
	auto bwrt = std::make_unique<MemoryWriter>();
	auto model = std::make_unique<UniversalModel>(256 + 1, 256, 1);
	while (true) {
		int c = decoder_->decode(*model);
		if (c == 256) {
			break;
		}
		bwrt->write_byte(static_cast<std::uint8_t>(c));
		model->shift_context(c);
	}

	return bwrt->get_data();
}

std::uint8_t PjgDecoder::bit() {
	auto model = std::make_unique<BinaryModel>(1, -1);
	std::uint8_t bit = decoder_->decode(*model); // This conversion is okay since there are only 2 symbols in the model.
	return bit;
}