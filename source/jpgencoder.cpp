#include "jpgencoder.h"

#include "jpg.h"
#include "jfifparse.h"
#include "pjpgtbl.h"

JpgEncoder::JpgEncoder(FrameInfo& frame_info, const std::vector<Segment>& segments, std::uint8_t padbit) :
	frame_info_(frame_info),
	segments_(segments) {

	huffman_writer_ = std::make_unique<BitWriter>(padbit);
}

std::vector<std::uint8_t> JpgEncoder::get_huffman_data() const {
	return huffman_data_;
}

std::vector<std::size_t> JpgEncoder::get_restart_marker_pos() const {
	return restart_marker_pos_;
}

std::vector<std::size_t> JpgEncoder::get_scan_pos() const {
	return scan_pos_;
}

void JpgEncoder::encode() {
	int restart_marker_count = 0;
	int restart_interval = 0;
	scan_pos_ = { 0 };

	for (const auto& segment : segments_) {
		switch (segment.get_type()) {
		case Marker::DHT:
			jfif::parse_dht(segment, dc_tables_, ac_tables_);
			continue;
		case Marker::DRI:
			restart_interval = jfif::parse_dri(segment);
			continue;
		case Marker::SOS:
			scan_info_ = jfif::get_scan_info(frame_info_, segment);
			break;
		default:
			continue; // Ignore other segment types.
		}

		if (restart_interval > 0) {
			int tmp = restart_marker_count + (scan_info_.cmpc > 1 ?
				                                  frame_info_.mcu_count / restart_interval
				                                  : frame_info_.components[scan_info_.cmp[0]].bc / restart_interval);
			restart_marker_pos_.resize(tmp + 1);
		}

		encode_scan(restart_interval, restart_marker_count);
		scan_pos_.emplace_back(huffman_writer_->num_bytes_written()); // Store scan position.
	}

	huffman_data_ = huffman_writer_->get_data();

	// store last restart position
	if (!restart_marker_pos_.empty()) {
		restart_marker_pos_[restart_marker_count] = huffman_data_.size();
	}
}

void JpgEncoder::encode_scan(int restart_interval, int& restart_markers) {
	// intial variables set for encoding
	int cmp = scan_info_.cmp[0];
	int csc = 0;
	int mcu = 0;
	int sub = 0;
	int dpos = 0;

	CodingStatus status = CodingStatus::OKAY;
	while (status != CodingStatus::DONE) {
		int restart_wait_counter = restart_interval;

		std::fill(std::begin(lastdc_), std::end(lastdc_), 0);
		if (scan_info_.cmpc > 1) {
			status = encode_interleaved(restart_interval, cmp, dpos, restart_wait_counter, csc, mcu, sub);
		}
		else {
			status = encode_noninterleaved(restart_interval, cmp, dpos, restart_wait_counter);
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

CodingStatus JpgEncoder::encode_noninterleaved(int rsti, int cmp, int& dpos, int& rstw) {
	const auto& component = frame_info_.components[cmp];
	if (frame_info_.coding_process == JpegType::SEQUENTIAL) {
		return this->encode_sequential_noninterleaved(component, cmp, rsti, dpos, rstw);
	} else if (scan_info_.to == 0) {
		return this->encode_progressive_noninterleaved_dc(component, cmp, rsti, dpos, rstw);
	} else {
		return this->encode_progressive_noninterleaved_ac(component, rsti, dpos, rstw);
	}
}

CodingStatus JpgEncoder::encode_interleaved(int rsti, int& cmp, int& dpos, int& rstw, int& csc, int& mcu, int& sub) {
	if (frame_info_.coding_process == JpegType::SEQUENTIAL) {
		return encode_sequential_interleaved(rsti, cmp, dpos, rstw, csc, mcu, sub);
	} else {
		return encode_progressive_interleaved_dc(rsti, cmp, dpos, rstw, csc, mcu, sub);
	}
}

CodingStatus JpgEncoder::encode_sequential_noninterleaved(const Component& component, int cmp, int rsti, int& dpos, int& rstw) {
	auto status = CodingStatus::OKAY;
	while (status == CodingStatus::OKAY) {
		encode_sequential(component, cmp, dpos);

		status = jpg::next_mcuposn(component, rsti, dpos, rstw);
	}
	return status;
}

CodingStatus JpgEncoder::encode_progressive_noninterleaved_dc(const Component& component, int cmp, int rsti, int& dpos, int& rstw) {
	auto status = CodingStatus::OKAY;
	if (scan_info_.sah == 0) {
		while (status == CodingStatus::OKAY) {
			this->dc_succ_approx_first_stage(component, cmp, dpos);
			status = jpg::next_mcuposn(component, rsti, dpos, rstw);
		}
	} else {
		while (status == CodingStatus::OKAY) {
			dc_succ_approx_later_stage(component, dpos);
			status = jpg::next_mcuposn(component, rsti, dpos, rstw);
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

			status = jpg::next_mcuposn(component, rsti, dpos, rstw);
		}

		// encode remaining eobrun
		this->eobrun(ac_table, eobrun);
	} else {
		// successive approximation later stage
		while (status == CodingStatus::OKAY) {
			copy_colldata_to_block_in_scan(component, dpos);

			// encode block
			this->ac_prg_sa(ac_table, eobrun);

			status = jpg::next_mcuposn(component, rsti, dpos, rstw);
		}

		// encode remaining eobrun
		this->eobrun(ac_table, eobrun);

		// encode remaining correction bits
		this->write_correction_bits();
	}
	return status;
}

CodingStatus JpgEncoder::encode_progressive_interleaved_dc(int rsti, int& cmp, int& dpos, int& rstw, int& csc, int& mcu, int& sub) {
	auto status = CodingStatus::OKAY;
	if (scan_info_.sah == 0) {
		while (status == CodingStatus::OKAY) {
			dc_succ_approx_first_stage(frame_info_.components[cmp], cmp, dpos);

			status = jpg::increment_counts(frame_info_, scan_info_, rsti, mcu, cmp, csc, sub, rstw);
			dpos = jpg::next_mcupos(frame_info_, mcu, cmp, sub);
		}
	} else {
		while (status == CodingStatus::OKAY) {
			dc_succ_approx_later_stage(frame_info_.components[cmp], dpos);

			status = jpg::increment_counts(frame_info_, scan_info_, rsti, mcu, cmp, csc, sub, rstw);
			dpos = jpg::next_mcupos(frame_info_, mcu, cmp, sub);
		}
	}
	return status;
}

CodingStatus JpgEncoder::encode_sequential_interleaved(int rsti, int& cmp, int& dpos, int& rstw, int& csc, int& mcu, int& sub) {
	auto status = CodingStatus::OKAY;
	while (status == CodingStatus::OKAY) {
		encode_sequential(frame_info_.components[cmp], cmp, dpos);

		status = jpg::increment_counts(frame_info_, scan_info_, rsti, mcu,  cmp, csc, sub, rstw);
		dpos = jpg::next_mcupos(frame_info_, mcu, cmp, sub);
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
	this->block_seq(*dc_tables_[component.huffac], *ac_tables_[component.huffac]);
}

void JpgEncoder::block_seq(const HuffCodes& dc_table, const HuffCodes& ac_table) {
	// encode DC
	this->dc_prg_fs(dc_table);

	// encode AC
	int z = 0;
	for (std::size_t bpos = 1; bpos < block_.size(); bpos++) {
		// if nonzero is encountered
		if (block_[bpos] != 0) {
			// write remaining zeroes
			while (z >= 16) {
				huffman_writer_->write_u16(ac_table.cval[0xF0], ac_table.clen[0xF0]);
				z -= 16;
			}
			// vli encode
			int s = pjg::bitlen2048n(block_[bpos]);
			std::uint16_t n = envli(s, block_[bpos]);
			int hc = ((z << 4) + s);
			// write to huffman writer
			huffman_writer_->write_u16(ac_table.cval[hc], ac_table.clen[hc]);
			huffman_writer_->write_u16(n, s);
			// reset zeroes
			z = 0;
		} else { // increment zero counter
			z++;
		}
	}
	// write eob if needed
	if (z > 0) {
		huffman_writer_->write_u16(ac_table.cval[0], ac_table.clen[0]);
	}
}

void JpgEncoder::dc_succ_approx_first_stage(const Component& component, int cmp, int dpos) {
	// diff coding & bitshifting for dc 
	const int tmp = component.colldata[0][dpos] >> scan_info_.sal;
	block_[0] = tmp - lastdc_[cmp];
	lastdc_[cmp] = tmp;

	// encode dc
	this->dc_prg_fs(*dc_tables_[component.huffdc]);
}

void JpgEncoder::dc_prg_fs(const HuffCodes& dc_table) {
	// encode DC	
	int s = pjg::bitlen2048n(block_[0]);
	std::uint16_t n = envli(s, block_[0]);
	huffman_writer_->write_u16(dc_table.cval[s], dc_table.clen[s]);
	huffman_writer_->write_u16(n, s);
}

void JpgEncoder::dc_succ_approx_later_stage(const Component& component, int dpos) {
	// fetch bit from current bitplane
	block_[0] = bitops::bitn(component.colldata[0][dpos], scan_info_.sal);

	// encode dc correction bit
	this->dc_prg_sa();
}

void JpgEncoder::dc_prg_sa() {
	// enocode next bit of dc coefficient
	huffman_writer_->write_bit(block_[0]);
}

void JpgEncoder::ac_prg_fs(const HuffCodes& ac_table, int& eobrun) {
	// encode AC
	std::uint8_t z = 0;
	for (int bpos = scan_info_.from; bpos <= scan_info_.to; bpos++) {
		// if nonzero is encountered
		if (block_[bpos] != 0) {
			// encode eobrun
			this->eobrun(ac_table, eobrun);
			// write remaining zeroes
			while (z >= 16) {
				huffman_writer_->write_u16(ac_table.cval[0xF0], ac_table.clen[0xF0]);
				z -= 16;
			}
			// vli encode
			std::uint8_t s = pjg::bitlen2048n(block_[bpos]);
			std::uint16_t n = envli(s, block_[bpos]);
			int hc = ((z << 4) + s);
			// write to huffman writer
			huffman_writer_->write_u16(ac_table.cval[hc], ac_table.clen[hc]);
			huffman_writer_->write_u16(n, s);
			// reset zeroes
			z = 0;
		} else { // increment zero counter
			z++;
		}
	}

	// check eob, increment eobrun if needed
	if (z > 0) {
		eobrun++;
		// check eobrun, encode if needed
		if (eobrun == ac_table.max_eobrun) {
			this->eobrun(ac_table, eobrun);
		}
	}
}

void JpgEncoder::ac_prg_sa(const HuffCodes& ac_table, int& eobrun) {
	int eob = scan_info_.from;
	int bpos;

	// check if block contains any newly nonzero coefficients and find out position of eob
	for (bpos = scan_info_.to; bpos >= scan_info_.from; bpos--) {
		if ((block_[bpos] == 1) || (block_[bpos] == -1)) {
			eob = bpos + 1;
			break;
		}
	}

	// encode eobrun if needed
	if ((eob > scan_info_.from) && (eobrun > 0)) {
		this->eobrun(ac_table, eobrun);
		this->write_correction_bits();
	}

	// encode AC
	std::uint8_t z = 0;
	for (bpos = scan_info_.from; bpos < eob; bpos++) {
		// if zero is encountered
		if (block_[bpos] == 0) {
			z++; // increment zero counter
			if (z == 16) { // write zeroes if needed
				huffman_writer_->write_u16(ac_table.cval[0xF0], ac_table.clen[0xF0]);
				this->write_correction_bits();
				z = 0;
			}
		}
		// if nonzero is encountered
		else if ((block_[bpos] == 1) || (block_[bpos] == -1)) {
			// vli encode			
			std::uint8_t s = pjg::bitlen2048n(block_[bpos]);
			std::uint16_t n = envli(s, block_[bpos]);
			int hc = (z << 4) + s;
			// write to huffman writer
			huffman_writer_->write_u16(ac_table.cval[hc], ac_table.clen[hc]);
			huffman_writer_->write_u16(n, s);
			// write correction bits
			this->write_correction_bits();
			// reset zeroes
			z = 0;
		} else { // store correction bits
			std::uint8_t bit = block_[bpos] & 0x1;
			correction_bits_.emplace_back(bit);
		}
	}

	// fast processing after eob
	for (; bpos <= scan_info_.to; bpos++) {
		if (block_[bpos] != 0) { // store correction bits
			std::uint8_t bit = block_[bpos] & 0x1;
			correction_bits_.emplace_back(bit);
		}
	}

	// check eob, increment eobrun if needed
	if (eob <= scan_info_.to) {
		eobrun++;
		// check eobrun, encode if needed
		if (eobrun == ac_table.max_eobrun) {
			this->eobrun(ac_table, eobrun);
			this->write_correction_bits();
		}
	}
}

void JpgEncoder::eobrun(const HuffCodes& ac_table, int& eobrun) {
	if (eobrun > 0) {
		while (eobrun > ac_table.max_eobrun) {
			huffman_writer_->write_u16(ac_table.cval[0xE0], ac_table.clen[0xE0]);
			huffman_writer_->write_u16(std::uint16_t(e_envli(14, 32767)), 14);
			eobrun -= ac_table.max_eobrun;
		}
		std::uint8_t s = bitlen(eobrun);
		s--;
		std::uint16_t n = e_envli(s, eobrun);
		int hc = s << 4;
		huffman_writer_->write_u16(ac_table.cval[hc], ac_table.clen[hc]);
		huffman_writer_->write_u16(n, s);
		eobrun = 0;
	}
}

void JpgEncoder::write_correction_bits() {
	for (std::uint8_t bit : correction_bits_) {
		huffman_writer_->write_bit(bit);
	}

	correction_bits_.clear();
}
