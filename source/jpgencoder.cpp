#include "jpgencoder.h"

#include "jpg.h"
#include "jfifparse.h"
#include "pjpgtbl.h"

JpgEncoder::JpgEncoder(Writer& jpg_output_writer, FrameInfo& frame_info, const std::vector<Segment>& segments, std::uint8_t padbit) :
	output_writer_(jpg_output_writer),
	frame_info_(frame_info),
	segments_(segments) {

	// open huffman coded image data in abitwriter
	huffw_ = std::make_unique<BitWriter>(padbit); // bitwise writer for image data
	
	// init storage writer
	storw_ = std::make_unique<MemoryWriter>(); // bytewise writer for storage of correction bits
}

void JpgEncoder::copy_colldata_to_block_in_scan(const Component& component, int dpos) {
	// copy from colldata
	for (int bpos = scan_info_.from; bpos <= scan_info_.to; bpos++) {
		block_[bpos] = fdiv2(component.colldata[bpos][dpos], scan_info_.sal);
	}
}

void JpgEncoder::dc_successive_first_stage(const Component& component, int cmp, int dpos) {
	// diff coding & bitshifting for dc 
	const int tmp = component.colldata[0][dpos] >> scan_info_.sal;
	block_[0] = tmp - lastdc_[cmp];
	lastdc_[cmp] = tmp;

	// encode dc
	this->dc_prg_fs(*dc_tables_[component.huffdc]);
}

void JpgEncoder::dc_successive_later_stage(const Component& component, int dpos) {
	// fetch bit from current bitplane
	block_[0] = bitops::bitn(component.colldata[0][dpos], scan_info_.sal);

	// encode dc correction bit
	this->dc_prg_sa();
}

void JpgEncoder::sequential_interleaved(const Component& component, int cmp, int dpos) {
	for (std::size_t bpos = 0; bpos < block_.size(); bpos++) {
		block_[bpos] = component.colldata[bpos][dpos];
	}

	// diff coding for dc
	block_[0] -= lastdc_[cmp];
	lastdc_[cmp] = component.colldata[0][dpos];

	// encode block
	this->block_seq(*dc_tables_[component.huffac], *ac_tables_[component.huffac]);
}

CodingStatus JpgEncoder::encode_noninterleaved(int rsti, int cmp, int& dpos, int& rstw) {
	int eobrun = 0; // run of eobs

	CodingStatus status = CodingStatus::OKAY;
	const auto& component = frame_info_.components[cmp];
	if (frame_info_.coding_process == JpegType::SEQUENTIAL) {
		// ---> sequential non interleaved encoding <---
		while (status == CodingStatus::OKAY) {
			sequential_interleaved(component, cmp, dpos);

			status = jpg::next_mcuposn(component, rsti, dpos, rstw);
		}
	} else if (scan_info_.to == 0) {
		if (scan_info_.sah == 0) {
			// ---> progressive non interleaved DC encoding <---
			// ---> succesive approximation first stage <---
			while (status == CodingStatus::OKAY) {
				dc_successive_first_stage(component, cmp, dpos);

				// Increment dpos
				status = jpg::next_mcuposn(component, rsti, dpos, rstw);
			}
		} else {
			// ---> progressive non interleaved DC encoding <---
			// ---> succesive approximation later stage <---
			while (status == CodingStatus::OKAY) {
				dc_successive_later_stage(component, dpos);

				// next mcupos
				status = jpg::next_mcuposn(component, rsti, dpos, rstw);
			}
		}
	} else {
		if (scan_info_.sah == 0) {
			// ---> progressive non interleaved AC encoding <---
			// ---> succesive approximation first stage <---
			while (status == CodingStatus::OKAY) {
				copy_colldata_to_block_in_scan(component, dpos);

				// encode block
				this->ac_prg_fs(*ac_tables_[component.huffac], eobrun);

				status = jpg::next_mcuposn(component, rsti, dpos, rstw);
			}

			// encode remaining eobrun
			this->eobrun(*ac_tables_[component.huffac], eobrun);
		} else {
			// ---> progressive non interleaved AC encoding <---
			// ---> succesive approximation later stage <---
			while (status == CodingStatus::OKAY) {
				copy_colldata_to_block_in_scan(component, dpos);

				// encode block
				this->ac_prg_sa(*ac_tables_[component.huffac], eobrun);

				status = jpg::next_mcuposn(component, rsti, dpos, rstw);
			}

			// encode remaining eobrun
			this->eobrun(*ac_tables_[component.huffac], eobrun);

			// encode remaining correction bits
			this->crbits();
		}
	}
	return status;
}

CodingStatus JpgEncoder::encode_interleaved(int rsti, int& cmp, int& dpos, int& rstw, int& csc, int& mcu, int& sub) {
	CodingStatus status = CodingStatus::OKAY;
	if (frame_info_.coding_process == JpegType::SEQUENTIAL) {
		// ---> sequential interleaved encoding <---
		while (status == CodingStatus::OKAY) {
			sequential_interleaved(frame_info_.components[cmp], cmp, dpos);

			status = jpg::increment_counts(frame_info_, scan_info_, rsti, mcu, cmp, csc, sub, rstw);
			dpos = jpg::next_mcupos(frame_info_, mcu, cmp, sub);

		}
	} else if (scan_info_.sah == 0) {
		// ---> progressive interleaved DC encoding <---
		// ---> succesive approximation first stage <---
		while (status == CodingStatus::OKAY) {
			dc_successive_first_stage(frame_info_.components[cmp], cmp, dpos);

			// next mcupos
			status = jpg::increment_counts(frame_info_, scan_info_, rsti, mcu, cmp, csc, sub, rstw);
			dpos = jpg::next_mcupos(frame_info_, mcu, cmp, sub);
		}
	} else {
		// ---> progressive interleaved DC encoding <---
		// ---> succesive approximation later stage <---
		while (status == CodingStatus::OKAY) {
			dc_successive_later_stage(frame_info_.components[cmp], dpos);

			status = jpg::increment_counts(frame_info_, scan_info_, rsti, mcu, cmp, csc, sub, rstw);
			dpos = jpg::next_mcupos(frame_info_, mcu, cmp, sub);
		}
	}
	return status;
}

void JpgEncoder::recode() {
	int scan_count = 0;
	int restart_markers = 0; // Count of restart markers.
	int restart_interval = 0; // Restart interval.

	for (const auto& segment : segments_) {
		switch (segment.get_type()) {
		case Marker::kDHT:
			try {
				jfif::parse_dht(segment.get_data(), dc_tables_, ac_tables_);
			} catch (const std::range_error&) {
				throw;
			}
			continue;
		case Marker::kDRI:
			try {
				restart_interval = jfif::parse_dri(segment.get_data());
			} catch (const std::runtime_error&) {
				throw;
			}
			continue;
		case Marker::kSOS:
			try {
				scan_info_ = jfif::get_scan_info(frame_info_, segment.get_data());
			} catch (std::runtime_error&) {
				throw;
			}
			break;
		default:
			continue; // Ignore other segment types.
		}

		// (re)alloc scan positons array
		scnp_.resize(scan_count + 2);

		// (re)alloc restart marker positons array if needed
		if (restart_interval > 0) {
			int tmp = restart_markers + ((scan_info_.cmpc > 1) ?
				                  (frame_info_.mcu_count / restart_interval) : (frame_info_.components[scan_info_.cmp[0]].bc / restart_interval));
			rstp_.resize(tmp + 1);
		}

		// intial variables set for encoding
		int cmp = scan_info_.cmp[0];
		int csc = 0;
		int mcu = 0;
		int sub = 0;
		int dpos = 0;

		// store scan position
		scnp_[scan_count] = huffw_->getpos();

		// JPEG imagedata encoding routines
		CodingStatus status = CodingStatus::OKAY;
		while (status != CodingStatus::DONE) {
			// set rst wait counter
			int rstw = restart_interval; // restart wait counter

			std::fill(std::begin(lastdc_), std::end(lastdc_), 0);
			if (scan_info_.cmpc > 1) {
				status = encode_interleaved(restart_interval, cmp, dpos, rstw, csc, mcu, sub);
			} else {
				status = encode_noninterleaved(restart_interval, cmp, dpos, rstw);
			}

			// pad huffman writer
			huffw_->pad();

			if (status == CodingStatus::RESTART) {
				if (restart_interval > 0) {
					// store rstp & stay in the loop
					rstp_[restart_markers] = huffw_->getpos() - 1;
					restart_markers++;
				}
			}
		}
		scan_count++;
	}

	huffman_data_ = huffw_->get_data();

	// store last scan & restart positions
	scnp_[scan_count] = huffman_data_.size();
	if (!rstp_.empty()) {
		rstp_[restart_markers] = huffman_data_.size();
	}
}

void JpgEncoder::write_scan_huffman_data(int scan, int& restart_pos, std::vector<std::uint8_t>& rst_err) {
	int corrected_pos = 0; // in-scan corrected rst marker position

	// write & expand huffman coded image data
	// ipos is the current position in image data.
	for (auto ipos = scnp_[scan]; ipos < scnp_[scan + 1]; ipos++) {
		// write current byte
		output_writer_.write_byte(huffman_data_[ipos]);
		// check current byte, stuff if needed
		if (huffman_data_[ipos] == 0xFF) {
			output_writer_.write_byte(std::uint8_t(0)); // 0xFF stuff value
		}
		// insert restart markers if needed
		if (!rstp_.empty()) {
			if (ipos == rstp_[restart_pos]) {
				const std::uint8_t restart_marker = 0xD0 + (corrected_pos % 8);
				constexpr std::uint8_t mrk = 0xFF; // marker start
				output_writer_.write_byte(mrk);
				output_writer_.write_byte(restart_marker);
				restart_pos++;
				corrected_pos++;
			}
		}
	}
	// insert false rst markers at end if needed
	if (!rst_err.empty()) {
		while (rst_err[scan] > 0) {
			const std::uint8_t rst = 0xD0 + (corrected_pos % 8); // Restart marker
			constexpr std::uint8_t mrk = 0xFF; // marker start
			output_writer_.write_byte(mrk);
			output_writer_.write_byte(rst);
			corrected_pos++;
			rst_err[scan]--;
		}
	}
}

void JpgEncoder::merge(const std::vector<std::uint8_t>& garbage_data, std::vector<std::uint8_t>& rst_err) {
	int restart_pos = 0; // current restart marker position
	int scan = 0; // number of current scan	

	constexpr std::array<std::uint8_t, 2> SOI{0xFF, 0xD8};
	output_writer_.write(SOI);

	// JPEG writing loop
	for (const auto& segment : segments_) {
		output_writer_.write(segment.get_data());

		if (segment.get_type() != Marker::kSOS) {
			continue;
		}

		write_scan_huffman_data(scan, restart_pos, rst_err);

		scan++;
	}

	constexpr std::array<std::uint8_t, 2> EOI{0xFF, 0xD9};
	output_writer_.write(EOI);

	if (!garbage_data.empty()) {
		output_writer_.write(garbage_data);
	}

	if (output_writer_.error()) {
		throw std::runtime_error("write error, possibly drive is full");
	}
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
				huffw_->write_u16(ac_table.cval[0xF0], ac_table.clen[0xF0]);
				z -= 16;
			}
			// vli encode
			int s = pjg::bitlen2048n(block_[bpos]);
			std::uint16_t n = envli(s, block_[bpos]);
			int hc = ((z << 4) + s);
			// write to huffman writer
			huffw_->write_u16(ac_table.cval[hc], ac_table.clen[hc]);
			huffw_->write_u16(n, s);
			// reset zeroes
			z = 0;
		} else { // increment zero counter
			z++;
		}
	}
	// write eob if needed
	if (z > 0) {
		huffw_->write_u16(ac_table.cval[0], ac_table.clen[0]);
	}
}

void JpgEncoder::dc_prg_fs(const HuffCodes& dc_table) {
	// encode DC	
	int s = pjg::bitlen2048n(block_[0]);
	std::uint16_t n = envli(s, block_[0]);
	huffw_->write_u16(dc_table.cval[s], dc_table.clen[s]);
	huffw_->write_u16(n, s);
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
				huffw_->write_u16(ac_table.cval[0xF0], ac_table.clen[0xF0]);
				z -= 16;
			}
			// vli encode
			std::uint8_t s = pjg::bitlen2048n(block_[bpos]);
			std::uint16_t n = envli(s, block_[bpos]);
			int hc = ((z << 4) + s);
			// write to huffman writer
			huffw_->write_u16(ac_table.cval[hc], ac_table.clen[hc]);
			huffw_->write_u16(n, s);
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

void JpgEncoder::dc_prg_sa() {
	// enocode next bit of dc coefficient
	huffw_->write_bit(block_[0]);
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
		this->crbits();
	}

	// encode AC
	std::uint8_t z = 0;
	for (bpos = scan_info_.from; bpos < eob; bpos++) {
		// if zero is encountered
		if (block_[bpos] == 0) {
			z++; // increment zero counter
			if (z == 16) { // write zeroes if needed
				huffw_->write_u16(ac_table.cval[0xF0], ac_table.clen[0xF0]);
				this->crbits();
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
			huffw_->write_u16(ac_table.cval[hc], ac_table.clen[hc]);
			huffw_->write_u16(n, s);
			// write correction bits
			this->crbits();
			// reset zeroes
			z = 0;
		} else { // store correction bits
			std::uint8_t n = block_[bpos] & 0x1;
			storw_->write_byte(n);
		}
	}

	// fast processing after eob
	for (; bpos <= scan_info_.to; bpos++) {
		if (block_[bpos] != 0) { // store correction bits
			std::uint8_t n = block_[bpos] & 0x1;
			storw_->write_byte(n);
		}
	}

	// check eob, increment eobrun if needed
	if (eob <= scan_info_.to) {
		eobrun++;
		// check eobrun, encode if needed
		if (eobrun == ac_table.max_eobrun) {
			this->eobrun(ac_table, eobrun);
			this->crbits();
		}
	}
}

void JpgEncoder::eobrun(const HuffCodes& ac_table, int& eobrun) {
	if (eobrun > 0) {
		while (eobrun > ac_table.max_eobrun) {
			huffw_->write_u16(ac_table.cval[0xE0], ac_table.clen[0xE0]);
			huffw_->write_u16(std::uint16_t(e_envli(14, 32767)), 14);
			eobrun -= ac_table.max_eobrun;
		}
		std::uint8_t s = bitlen(eobrun);
		s--;
		std::uint16_t n = e_envli(s, eobrun);
		int hc = s << 4;
		huffw_->write_u16(ac_table.cval[hc], ac_table.clen[hc]);
		huffw_->write_u16(n, s);
		eobrun = 0;
	}
}

void JpgEncoder::crbits() {
	const auto& data = storw_->get_data();

	// write bits to huffwriter
	for (std::uint8_t bit : data) {
		huffw_->write_bit(bit);
	}

	// reset writer, discard data
	storw_->rewind();
}
