#include "jpgencoder.h"

#include "bitops.h"
#include "jfif.h"
#include "marker.h"
#include "pjpgtbl.h"
#include "segmentparser.h"

JpgEncoder::JpgEncoder(const FrameInfo& frame_info, const std::vector<Segment>& segments, std::uint8_t padbit) :
	frame_info_(frame_info),
	segments_(segments) {
	huffman_writer_ = std::make_unique<BitWriter>(padbit);
}

std::tuple<std::vector<std::uint8_t>, std::vector<std::size_t>, std::vector<std::size_t>> JpgEncoder::encode(std::vector<Component>& components) {
	int restart_marker_count = 0;
	int restart_interval = 0;
	scan_pos_ = { 0 };

	for (const auto& segment : segments_) {
		switch (segment.get_type()) {
		case Marker::DHT:
			SegmentParser::parse_dht(segment, dc_tables_, ac_tables_);
			continue;
		case Marker::DRI:
			restart_interval = SegmentParser::parse_dri(segment);
			continue;
		case Marker::SOS:
			scan_info_ = SegmentParser::get_scan_info(segment, components);
			break;
		default:
			continue; // Ignore other segment types.
		}

		if (restart_interval > 0) {
			int tmp = restart_marker_count + (scan_info_.cmp.size() > 1 ?
				                                  frame_info_.mcu_count / restart_interval
				                                  : components[scan_info_.cmp[0]].bc / restart_interval);
			restart_marker_pos_.resize(tmp + 1);
		}

		encode_scan(components, restart_interval, restart_marker_count);
		scan_pos_.emplace_back(huffman_writer_->num_bytes_written()); // Store scan position.
	}
	
	// store last restart position
	if (!restart_marker_pos_.empty()) {
		restart_marker_pos_[restart_marker_count] = huffman_writer_->num_bytes_written();
	}

	return std::tuple<std::vector<std::uint8_t>, std::vector<std::size_t>, std::vector<std::size_t>>
		(huffman_writer_->get_data(), std::move(restart_marker_pos_), std::move(scan_pos_));
}

void JpgEncoder::encode_scan(const std::vector<Component>& components, int restart_interval, int& restart_markers) {
	// intial variables set for encoding
	int cmp_id = scan_info_.cmp[0];
	int csc = 0;
	int mcu = 0;
	int sub = 0;
	int dpos = 0;

	CodingStatus status = CodingStatus::OKAY;
	while (status != CodingStatus::DONE) {
		int restart_wait_counter = restart_interval;

		std::fill(std::begin(lastdc_), std::end(lastdc_), std::int16_t(0));
		if (scan_info_.cmp.size() > 1) {
			status = encode_interleaved(components, restart_interval, cmp_id, dpos, restart_wait_counter, csc, mcu, sub);
		}
		else {
			status = encode_noninterleaved(components[cmp_id], restart_interval, cmp_id, dpos, restart_wait_counter);
		}

		huffman_writer_->pad();

		if (status == CodingStatus::RESTART) {
			if (restart_interval > 0) {
				// store rstp & stay in the loop
				restart_marker_pos_[restart_markers] = huffman_writer_->num_bytes_written() - 1;
				restart_markers++;
			}
		}
	}
}

CodingStatus JpgEncoder::encode_noninterleaved(const Component& component, int rsti, int cmp_id, int& dpos, int& rstw) {
	if (frame_info_.coding_process == JpegType::SEQUENTIAL) {
		return this->encode_sequential_noninterleaved(component, cmp_id, rsti, dpos, rstw);
	} else if (scan_info_.to == 0) {
		return this->encode_progressive_noninterleaved_dc(component, cmp_id, rsti, dpos, rstw);
	} else {
		return this->encode_progressive_noninterleaved_ac(component, rsti, dpos, rstw);
	}
}

CodingStatus JpgEncoder::encode_interleaved(const std::vector<Component>& components, int rsti, int& cmp_id, int& dpos, int& rstw, int& csc, int& mcu, int& sub) {
	if (frame_info_.coding_process == JpegType::SEQUENTIAL) {
		return encode_sequential_interleaved(components, rsti, cmp_id, dpos, rstw, csc, mcu, sub);
	} else {
		return encode_progressive_interleaved_dc(components, rsti, cmp_id, dpos, rstw, csc, mcu, sub);
	}
}

CodingStatus JpgEncoder::encode_sequential_noninterleaved(const Component& component, int cmp, int rsti, int& dpos, int& rstw) {
	auto status = CodingStatus::OKAY;
	while (status == CodingStatus::OKAY) {
		encode_sequential(component, cmp, dpos);

		status = component.next_mcuposn(rsti, dpos, rstw);
	}
	return status;
}

CodingStatus JpgEncoder::encode_progressive_noninterleaved_dc(const Component& component, int cmp, int rsti, int& dpos, int& rstw) {
	auto status = CodingStatus::OKAY;
	if (scan_info_.sah == 0) {
		while (status == CodingStatus::OKAY) {
			this->dc_succ_approx_first_stage(component, cmp, dpos);
			status = component.next_mcuposn(rsti, dpos, rstw);
		}
	} else {
		while (status == CodingStatus::OKAY) {
			dc_succ_approx_later_stage(component, dpos);
			status = component.next_mcuposn(rsti, dpos, rstw);
		}
	}
	return status;
}

CodingStatus JpgEncoder::encode_progressive_noninterleaved_ac(const Component& component, int rsti, int& dpos, int& rstw) {
	int eobrun = 0; // run of eobs
	auto status = CodingStatus::OKAY;
	auto ac_table = *ac_tables_[component.huffac];
	if (scan_info_.sah == 0) {
		// successive approximation first stage
		while (status == CodingStatus::OKAY) {
			copy_colldata_to_block_in_scan(component, dpos);

			// encode block
			this->ac_prg_fs(ac_table, eobrun);

			status = component.next_mcuposn(rsti, dpos, rstw);
		}

		// encode remaining eobrun
		this->encode_eobrun(ac_table, eobrun);
	} else {
		// successive approximation later stage
		while (status == CodingStatus::OKAY) {
			copy_colldata_to_block_in_scan(component, dpos);

			// encode block
			this->ac_prg_sa(ac_table, eobrun);

			status = component.next_mcuposn(rsti, dpos, rstw);
		}

		// encode remaining eobrun
		this->encode_eobrun(ac_table, eobrun);

		// encode remaining correction bits
		this->write_correction_bits();
	}
	return status;
}

CodingStatus JpgEncoder::encode_progressive_interleaved_dc(const std::vector<Component>& components, int rsti, int& cmp_id, int& dpos, int& rstw, int& csc, int& mcu, int& sub) {
	auto status = CodingStatus::OKAY;
	if (scan_info_.sah == 0) {
		while (status == CodingStatus::OKAY) {
			dc_succ_approx_first_stage(components[cmp_id], cmp_id, dpos);

			status = jfif::increment_counts(frame_info_, scan_info_, components[cmp_id], rsti, mcu, cmp_id, csc, sub, rstw);
			dpos = components[cmp_id].next_mcupos(frame_info_, mcu, sub);
		}
	} else {
		while (status == CodingStatus::OKAY) {
			dc_succ_approx_later_stage(components[cmp_id], dpos);

			status = jfif::increment_counts(frame_info_, scan_info_, components[cmp_id], rsti, mcu, cmp_id, csc, sub, rstw);
			dpos = components[cmp_id].next_mcupos(frame_info_, mcu, sub);
		}
	}
	return status;
}

CodingStatus JpgEncoder::encode_sequential_interleaved(const std::vector<Component>& components, int rsti, int& cmp_id, int& dpos, int& rstw, int& csc, int& mcu, int& sub) {
	auto status = CodingStatus::OKAY;
	while (status == CodingStatus::OKAY) {
		encode_sequential(components[cmp_id], cmp_id, dpos);

		status = jfif::increment_counts(frame_info_, scan_info_, components[cmp_id], rsti, mcu,  cmp_id, csc, sub, rstw);
		dpos = components[cmp_id].next_mcupos(frame_info_, mcu, sub);
	}
	return status;
}

void JpgEncoder::copy_colldata_to_block_in_scan(const Component& component, int dpos) {
	// copy from colldata
	for (int bpos = scan_info_.from; bpos <= scan_info_.to; bpos++) {
		block_[bpos] = fdiv2(component.colldata[bpos][dpos], scan_info_.sal);
	}
}

void JpgEncoder::encode_sequential(const Component& component, int cmp, int dpos) {
	for (std::size_t bpos = 0; bpos < block_.size(); bpos++) {
		block_[bpos] = component.colldata[bpos][dpos];
	}

	// diff coding for dc
	block_[0] -= lastdc_[cmp];
	lastdc_[cmp] = component.colldata[0][dpos];

	// encode block
	this->block_seq(*dc_tables_[component.huffdc], *ac_tables_[component.huffac]);
}

void JpgEncoder::vli_encode(const HuffCodes& ac_table, std::int16_t kth_elem, std::int32_t zero_run) {
	const auto size = pjg::bitlen2048n(kth_elem);
	const std::uint16_t value = std::uint16_t(envli(size, kth_elem)); // This cast is fine since kth_elem will be -2048 <= 0 <= 2048.
	const std::int32_t symbol = (zero_run << 4) + size;
	// write to huffman writer
	huffman_encode(ac_table, symbol);
	huffman_writer_->write_u16(value, size);
}

void JpgEncoder::block_seq(const HuffCodes& dc_table, const HuffCodes& ac_table) {
	// encode DC
	this->dc_prg_fs(dc_table);

	// encode AC
	std::int32_t zero_run = 0;
	for (std::size_t k = 1; k < block_.size(); k++) {
		// if nonzero is encountered
		if (block_[k] != 0) {
			// write remaining zeroes
			while (zero_run >= 16) {
				huffman_encode(ac_table, 0xF0);
				zero_run -= 16;
			}
			vli_encode(ac_table, block_[k], zero_run);
			// reset zeroes
			zero_run = 0;
		} else {
			zero_run++;
		}
	}
	// write eob if needed
	if (zero_run > 0) {
		huffman_encode(ac_table, 0);
	}
}

void JpgEncoder::dc_succ_approx_first_stage(const Component& component, int cmp, int dpos) {
	// diff coding & bitshifting for dc 
	const std::int16_t tmp = component.colldata[0][dpos] >> scan_info_.sal;
	block_[0] = tmp - lastdc_[cmp];
	lastdc_[cmp] = tmp;

	// encode dc
	this->dc_prg_fs(*dc_tables_[component.huffdc]);
}

void JpgEncoder::dc_prg_fs(const HuffCodes& dc_table) {
	// encode DC	
	const auto size = pjg::bitlen2048n(block_[0]);
	const std::uint16_t value = std::uint16_t(envli(size, block_[0])); // This cast is fine since block[0] will be -2048 <= 0 <= 2048.
	huffman_encode(dc_table, size);
	huffman_writer_->write_u16(value, size);
}

void JpgEncoder::dc_succ_approx_later_stage(const Component& component, int dpos) {
	// fetch bit from current bitplane
	block_[0] = bitops::bitn(component.colldata[0][dpos], scan_info_.sal);

	// encode dc correction bit
	this->dc_prg_sa();
}

void JpgEncoder::dc_prg_sa() {
	// encode next bit of dc coefficient
	huffman_writer_->write_bit(std::uint8_t(block_[0]));
}

void JpgEncoder::huffman_encode(const HuffCodes& table, int symbol) {
	const auto size = table.clen[symbol];
	const auto code = table.cval[symbol];
	huffman_writer_->write_u16(code, size);
}

void JpgEncoder::ac_prg_fs(const HuffCodes& ac_table, int& eobrun) {
	// encode AC
	std::int32_t zero_run = 0;
	for (int k = scan_info_.from; k <= scan_info_.to; k++) {
		// if nonzero is encountered
		if (block_[k] != 0) {
			// encode eobrun
			encode_eobrun(ac_table, eobrun);
			// write remaining zeroes
			while (zero_run >= 16) {
				huffman_encode(ac_table, 0xF0);
				zero_run -= 16;
			}
			vli_encode(ac_table, block_[k], zero_run);
			// reset zeroes
			zero_run = 0;
		} else { // increment zero counter
			zero_run++;
		}
	}

	// check eob, increment eobrun if needed
	if (zero_run > 0) {
		eobrun++;
		// check eobrun, encode if needed
		if (eobrun == ac_table.max_eobrun) {
			this->encode_eobrun(ac_table, eobrun);
		}
	}
}

void JpgEncoder::ac_prg_sa(const HuffCodes& ac_table, int& eobrun) {
	int eob = scan_info_.from;
	int k;

	// check if block contains any newly nonzero coefficients and find out position of eob
	for (k = scan_info_.to; k >= scan_info_.from; k--) {
		if ((block_[k] == 1) || (block_[k] == -1)) {
			eob = k + 1;
			break;
		}
	}

	// encode eobrun if needed
	if ((eob > scan_info_.from) && (eobrun > 0)) {
		this->encode_eobrun(ac_table, eobrun);
		this->write_correction_bits();
	}

	// encode AC
	std::int32_t zero_run = 0;
	for (k = scan_info_.from; k < eob; k++) {
		// if zero is encountered
		if (block_[k] == 0) {
			zero_run++; // increment zero counter
			if (zero_run == 16) { // write zeroes if needed
				huffman_encode(ac_table, 0xF0);
				this->write_correction_bits();
				zero_run = 0;
			}
		}
		// if nonzero is encountered
		else if ((block_[k] == 1) || (block_[k] == -1)) {
			vli_encode(ac_table, block_[k], zero_run);
			// write correction bits
			this->write_correction_bits();
			// reset zeroes
			zero_run = 0;
		} else { // store correction bits
			std::uint8_t bit = block_[k] & 0x1;
			correction_bits_.emplace_back(bit);
		}
	}

	// fast processing after eob
	for (; k <= scan_info_.to; k++) {
		if (block_[k] != 0) { // store correction bits
			std::uint8_t bit = block_[k] & 0x1;
			correction_bits_.emplace_back(bit);
		}
	}

	// check eob, increment eobrun if needed
	if (eob <= scan_info_.to) {
		eobrun++;
		// check eobrun, encode if needed
		if (eobrun == ac_table.max_eobrun) {
			this->encode_eobrun(ac_table, eobrun);
			this->write_correction_bits();
		}
	}
}

void JpgEncoder::encode_eobrun(const HuffCodes& ac_table, int& eobrun) {
	if (eobrun > 0) {
		while (eobrun > ac_table.max_eobrun) {
			huffman_encode(ac_table, 0xE0);
			constexpr std::size_t size = 14;
			constexpr std::uint16_t value = e_envli(size, 32767);
			huffman_writer_->write_u16(value, size);
			eobrun -= ac_table.max_eobrun;
		}
		std::int32_t size = bitlen(eobrun);
		size--;
		const std::uint16_t value = std::uint16_t(e_envli(size, eobrun));
		const std::int32_t symbol = size << 4;
		huffman_encode(ac_table, symbol);
		huffman_writer_->write_u16(value, size);
		eobrun = 0;
	}
}

void JpgEncoder::write_correction_bits() {
	for (std::uint8_t bit : correction_bits_) {
		huffman_writer_->write_bit(bit);
	}

	correction_bits_.clear();
}
