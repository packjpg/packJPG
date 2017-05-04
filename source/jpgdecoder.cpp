#include "jpgdecoder.h"

#include <string>
#include "jpg.h"
#include "jfifparse.h"

JpgDecoder::JpgDecoder(FrameInfo& frame_info, const std::vector<Segment>& segments, const std::vector<std::uint8_t>& huffman_data) :
	frame_info_(frame_info), segments_(segments) {
	huffman_reader_ = std::make_unique<BitReader>(huffman_data);
}

void JpgDecoder::decode() {
	int scan = 0; // Count of scans.

	// JPEG decompression loop
	int restart_interval = 0; // restart interval

	for (const auto& segment : segments_) {
		// seek till start-of-scan, parse only DHT, DRI and SOS
		switch (segment.get_type()) {
		case Marker::kDHT:
			try {
				jfif::parse_dht(segment.get_data(), hcodes_);
			} catch (const std::range_error&) {
				throw;
			}
			build_trees();
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
			continue;
		}

		// check if huffman tables are available
		for (int csc = 0; csc < scan_info_.cmpc; csc++) {
			const auto& component = frame_info_.components[scan_info_.cmp[csc]];
			if ((scan_info_.sal == 0 && !htrees_[0][component.huffdc]) ||
				(scan_info_.sah > 0 && !htrees_[1][component.huffac])) {
				throw std::runtime_error("Huffman table missing in scan " + std::to_string(scan));
			}
		}


		// intial variables set for decoding
		int cmp = scan_info_.cmp[0];
		int csc = 0;
		int mcu = 0;
		int sub = 0;
		int dpos = 0;

		// JPEG imagedata decoding routines
		CodingStatus status = CodingStatus::OKAY;
		while (status != CodingStatus::DONE) {
			// set last DCs for diff coding
			std::fill(std::begin(lastdc_), std::end(lastdc_), 0);

			try {
				if (scan_info_.cmpc > 1) {
					status = decode_interleaved_data(restart_interval, cmp, dpos, mcu, csc, sub);
				} else {
					status = decode_noninterleaved_data(restart_interval, cmp, dpos);
				}
			} catch (const std::runtime_error&) {
				throw;
			}

			// unpad huffman reader / check padbit
			if (padbit_set_) {
				if (padbit_ != huffman_reader_->unpad(padbit_)) {
					throw std::runtime_error("Inconsistent use of padbits.");
				}
			} else {
				padbit_ = huffman_reader_->unpad(padbit_);
				padbit_set_ = padbit_ == 0 || padbit_ == 1;
			}
		}
		scan++;
	}

	if (huffman_reader_->overread()) {
		throw std::runtime_error("Coded image data shorter than expected.");
	}

	if (!huffman_reader_->eof()) {
		throw std::runtime_error("Coded image data longer than expected.");
	}
}

CodingStatus JpgDecoder::decode_noninterleaved_data(int rsti, int cmp, int& dpos) {
	int eobrun = 0; // run of eobs
	int peobrun = 0; // previous eobrun
	int rstw = rsti; // restart wait counter

	CodingStatus status = CodingStatus::OKAY;
	auto& component = frame_info_.components[cmp];
	if (frame_info_.coding_process == JpegType::SEQUENTIAL) {
		// ---> sequential non interleaved decoding <---
		while (status == CodingStatus::OKAY) {
			this->decode_sequential_block(component, cmp, dpos);

			status = jpg::next_mcuposn(component, rsti, dpos, rstw);
		}
	} else if (scan_info_.to == 0) {
		if (scan_info_.sah == 0) {
			// ---> progressive non interleaved DC decoding <---
			// ---> succesive approximation first stage <---
			while (status == CodingStatus::OKAY) {
				decode_successive_approx_first_stage(component, cmp, dpos);

				status = jpg::next_mcuposn(component, rsti, dpos, rstw);
			}
		} else {
			// ---> progressive non interleaved DC decoding <---
			// ---> succesive approximation later stage <---
			while (status == CodingStatus::OKAY) {
				decode_success_approx_later_stage(component, dpos);

				// increment dpos
				status = jpg::next_mcuposn(component, rsti, dpos, rstw);
			}
		}
	} else {
		if (scan_info_.sah == 0) {
			// ---> progressive non interleaved AC decoding <---
			// ---> succesive approximation first stage <---
			while (status == CodingStatus::OKAY) {
				if (eobrun == 0) {
					// decode block
					int eob;
					try {
						eob = this->ac_prg_fs(*htrees_[1][component.huffac], eobrun);
					} catch (const std::runtime_error&) {
						throw;
					}

					if (eobrun > 0) {
						// check for non optimal coding
						if ((eob == scan_info_.from) && (peobrun > 0) &&
							(peobrun < hcodes_[1][component.huffac]->max_eobrun - 1)) {
							throw std::runtime_error("reconstruction of inefficient coding not supported");
						}
						peobrun = eobrun;
						eobrun--;
					} else {
						peobrun = 0;
					}

					// copy to colldata
					for (int bpos = scan_info_.from; bpos < eob; bpos++) {
						component.colldata[bpos][dpos] = block_[bpos] << scan_info_.sal;
					}
				} else {
					eobrun--;
				}

				try {
					status = this->skip_eobrun(component, rsti, dpos, rstw, eobrun);
				} catch (const std::runtime_error&) {
					throw;
				}

				if (status == CodingStatus::OKAY) {
					status = jpg::next_mcuposn(component, rsti, dpos, rstw);
				}
			}
		} else {
			// ---> progressive non interleaved AC decoding <---
			// ---> succesive approximation later stage <---
			while (status == CodingStatus::OKAY) {
				// copy from colldata
				for (int bpos = scan_info_.from; bpos <= scan_info_.to; bpos++) {
					block_[bpos] = component.colldata[bpos][dpos];
				}

				if (eobrun == 0) {
					// decode block (long routine)
					int eob;
					try {
						eob = this->ac_prg_sa(*htrees_[1][component.huffac], eobrun);
					} catch (std::runtime_error&) {
						throw;
					}

					if (eobrun > 0) {
						// check for non optimal coding
						if ((eob == scan_info_.from) && (peobrun > 0) &&
							(peobrun < hcodes_[1][component.huffac]->max_eobrun - 1)) {
							throw std::runtime_error("reconstruction of inefficient coding not supported");
						}

						// store eobrun
						peobrun = eobrun;
						eobrun--;
					} else {
						peobrun = 0;
					}
				} else {
					// decode block (short routine)
					this->eobrun_sa();
					eobrun--;
				}

				// copy back to colldata
				for (int bpos = scan_info_.from; bpos <= scan_info_.to; bpos++) {
					component.colldata[bpos][dpos] += block_[bpos] << scan_info_.sal;
				}

				status = jpg::next_mcuposn(component, rsti, dpos, rstw);
			}
		}
	}
	return status;
}

CodingStatus JpgDecoder::decode_interleaved_data(int rsti, int& cmp, int& dpos, int& mcu, int& csc, int& sub) {
	int rstw = rsti; // restart wait counter
	auto& components = frame_info_.components;
	CodingStatus status = CodingStatus::OKAY;
	if (frame_info_.coding_process == JpegType::SEQUENTIAL) {
		// ---> sequential interleaved decoding <---
		while (status == CodingStatus::OKAY) {
			this->decode_sequential_block(components[cmp], cmp, dpos);

			status = jpg::increment_counts(frame_info_, scan_info_, rsti, mcu, cmp, csc, sub, rstw);
			dpos = jpg::next_mcupos(frame_info_, mcu, cmp, sub);
		}
	} else if (scan_info_.sah == 0) {
		// ---> progressive interleaved DC decoding <---
		// ---> succesive approximation first stage <---
		while (status == CodingStatus::OKAY) {
			decode_successive_approx_first_stage(components[cmp], cmp, dpos);

			status = jpg::increment_counts(frame_info_, scan_info_, rsti, mcu, cmp, csc, sub, rstw);
			dpos = jpg::next_mcupos(frame_info_, mcu, cmp, sub);
		}
	} else {
		// ---> progressive interleaved DC decoding <---
		// ---> succesive approximation later stage <---					
		while (status == CodingStatus::OKAY) {
			decode_success_approx_later_stage(components[cmp], dpos);

			status = jpg::increment_counts(frame_info_, scan_info_, rsti, mcu, cmp, csc, sub, rstw);
			dpos = jpg::next_mcupos(frame_info_, mcu, cmp, sub);
		}
	}
	return status;
}

void JpgDecoder::decode_successive_approx_first_stage(Component& component, int cmp, int dpos) {
	try {
		this->dc_prg_fs(*htrees_[0][component.huffdc]);
	} catch (const std::runtime_error&) {
		throw;
	}

	// fix dc for diff coding
	component.colldata[0][dpos] = block_[0] + lastdc_[cmp];
	lastdc_[cmp] = component.colldata[0][dpos];

	// bitshift for succesive approximation
	component.colldata[0][dpos] <<= scan_info_.sal;
}

void JpgDecoder::decode_success_approx_later_stage(Component& component, int dpos) {
	// decode next bit
	this->dc_prg_sa();

	// shift in next bit
	component.colldata[0][dpos] += block_[0] << scan_info_.sal;
}

void JpgDecoder::decode_sequential_block(Component& component, int cmp, int dpos) {
	// decode block
	int eob;
	try {
		eob = this->block_seq(*htrees_[0][component.huffdc], *htrees_[1][component.huffdc]);
	} catch (const std::runtime_error&) {
		throw;
	}

	// check for non optimal coding
	if ((eob > 1) && (block_[eob - 1] == 0)) {
		throw std::runtime_error("reconstruction of inefficient coding not supported");
	}

	// fix dc
	block_[0] += lastdc_[cmp];
	lastdc_[cmp] = block_[0];

	// copy to colldata
	for (int bpos = 0; bpos < eob; bpos++) {
		component.colldata[bpos][dpos] = block_[bpos];
	}
}


void JpgDecoder::check_value_range(const std::vector<Component>& components) const {
	// out of range should never happen with unmodified JPEGs
	for (const auto& component : components) {
		for (std::size_t freq = 0; freq < component.colldata.size(); freq++) {
			const auto& coefficients = component.colldata[freq];
			const auto absmax = component.max_v(freq);
			for (auto value : coefficients) {
				if (std::abs(value) > absmax) {
					throw std::range_error("value out of range error: cmp id: " + std::to_string(component.jid)
						+ ", frq " + std::to_string(freq)
						+ ", val " + std::to_string(value)
						+ ", max " + std::to_string(absmax));
				}
			}
		}
	}
}

std::uint8_t JpgDecoder::get_padbit() const {
	return padbit_;
}

void JpgDecoder::build_trees() {
	for (std::size_t i = 0; i < hcodes_.size(); i++) {
		for (std::size_t j = 0; j < hcodes_[i].size(); j++) {
			if (hcodes_[i][j]) {
				htrees_[i][j] = std::make_unique<HuffTree>(*hcodes_[i][j]);
			}
		}
	}
}


int JpgDecoder::block_seq(const HuffTree& dctree, const HuffTree& actree) {
	int eob = 64;
	// decode dc
	try {
		this->dc_prg_fs(dctree);
	} catch (const std::runtime_error&) {
		throw;
	}

	// decode ac
	for (std::size_t bpos = 1; bpos < block_.size(); bpos++) {
		// decode next
		std::uint8_t hc;
		try {
			hc = actree.next_huffcode(*huffman_reader_);
		} catch (const std::runtime_error&) {
			throw;
		}
		// analyse code
		if (hc > 0) {
			std::uint8_t z = bitops::left_nibble(hc);
			std::uint8_t s = bitops::right_nibble(hc);
			std::uint16_t n = huffman_reader_->read_u16(s);
			if ((z + bpos) >= block_.size()) {
				throw std::runtime_error("Run is too long.");
			}
			std::fill_n(std::begin(block_) + bpos, z, std::int16_t(0));
			bpos += z;
			block_[bpos] = static_cast<std::int16_t>(devli(s, n)); // decode cvli
		} else if (hc == 0) { // EOB
			eob = bpos;
			// while( bpos < 64 ) // fill remaining block with zeroes
			//	block[ bpos++ ] = 0;
			break;
		}
	}


	// return position of eob
	return eob;
}

void JpgDecoder::dc_prg_fs(const HuffTree& dctree) {
	// decode dc
	std::uint8_t hc;
	try {
		hc = dctree.next_huffcode(*huffman_reader_);
	} catch (const std::runtime_error&) {
		throw;
	}
	int s = hc;
	std::uint16_t n = huffman_reader_->read_u16(s);
	block_[0] = static_cast<std::int16_t>(devli(s, n));
}

int JpgDecoder::ac_prg_fs(const HuffTree& actree, int& eobrun) {
	int eob = scan_info_.to + 1;
	// decode ac
	for (int bpos = scan_info_.from; bpos <= scan_info_.to;) {
		// decode next
		std::uint8_t hc;
		try {
			hc = actree.next_huffcode(*huffman_reader_);
		} catch (const std::runtime_error&) {
			throw;
		}
		auto l = bitops::left_nibble(hc);
		auto r = bitops::right_nibble(hc);
		// analyse code
		if ((l == 15) || (r > 0)) { // decode run/level combination
			std::uint8_t z = l;
			std::uint8_t s = r;
			std::uint16_t n = huffman_reader_->read_u16(s);
			if ((z + bpos) > scan_info_.to) {
				throw std::runtime_error("Run is too long.");
			}
			std::fill_n(std::begin(block_) + bpos, z, std::int16_t(0));
			bpos += z;
			block_[bpos] = static_cast<std::int16_t>(devli(s, n)); // decode cvli
			bpos++;
		} else { // decode eobrun
			eob = bpos;
			std::uint8_t s = l;
			std::uint16_t n = huffman_reader_->read_u16(s);
			eobrun = e_devli(s, n);
			// while( bpos <= to ) // fill remaining block with zeroes
			//	block[ bpos++ ] = 0;
			break;
		}
	}


	// return position of eob
	return eob;
}

void JpgDecoder::dc_prg_sa() {
	// decode next bit of dc coefficient
	block_[0] = huffman_reader_->read_bit();
}

int JpgDecoder::ac_prg_sa(const HuffTree& actree, int& eobrun) {
	signed char v;
	int bpos = scan_info_.from;
	int eob = scan_info_.to;
	// decode AC succesive approximation bits
	if (eobrun == 0) {
		while (bpos <= scan_info_.to) {
			// decode next
			std::uint8_t hc;
			try {
				hc = actree.next_huffcode(*huffman_reader_);
			} catch (const std::runtime_error&) {
				throw;
			}
			auto l = bitops::left_nibble(hc);
			auto r = bitops::right_nibble(hc);
			// analyse code
			if ((l == 15) || (r > 0)) { // decode run/level combination
				signed char z = l;
				std::uint8_t s = r;
				if (s == 0) {
					v = 0;
				} else if (s == 1) {
					std::uint8_t n = huffman_reader_->read_bit();
					v = (n == 0) ? -1 : 1; // fast decode vli
				} else {
					throw std::runtime_error("Decoding error in JpgDecoder::ac_prg_sa");
				}
				// write zeroes / write correction bits
				while (true) {
					if (block_[bpos] == 0) { // skip zeroes / write value
						if (z > 0) {
							z--;
						} else {
							block_[bpos++] = v;
							break;
						}
					} else { // read correction bit
						std::int16_t n = huffman_reader_->read_bit();
						block_[bpos] = (block_[bpos] > 0) ? n : -n;
					}
					if (bpos++ >= scan_info_.to) {
						throw std::runtime_error("bpos larger than to.");
					}
				}
			} else { // decode eobrun
				eob = bpos;
				std::uint8_t s = l;
				std::uint16_t n = huffman_reader_->read_u16(s);
				eobrun = e_devli(s, n);
				break;
			}
		}
	}

	// read after eob correction bits
	if (eobrun > 0) {
		for (; bpos <= scan_info_.to; bpos++) {
			if (block_[bpos] != 0) {
				std::int16_t n = huffman_reader_->read_bit();
				block_[bpos] = (block_[bpos] > 0) ? n : -n;
			}
		}
	}

	// return eob
	return eob;
}

void JpgDecoder::eobrun_sa() {
	// fast eobrun decoding routine for succesive approximation
	for (int bpos = scan_info_.from; bpos <= scan_info_.to; bpos++) {
		if (block_[bpos] != 0) {
			std::int16_t n = huffman_reader_->read_bit();
			block_[bpos] = (block_[bpos] > 0) ? n : -n;
		}
	}
}

CodingStatus JpgDecoder::skip_eobrun(const Component& component, int rsti, int& dpos, int& rstw, int& eobrun) {
	if (eobrun > 0) {// error check for eobrun
		// compare rst wait counter if needed
		if (rsti > 0) {
			if (eobrun > rstw) {
				throw std::runtime_error("eobrun greater than rstw");
			} else {
				rstw -= eobrun;
			}
		}

		// fix for non interleaved mcu - horizontal
		if (component.bch != component.nch) {
			dpos += (((dpos % component.bch) + eobrun) /
				component.nch) * (component.bch - component.nch);
		}

		// fix for non interleaved mcu - vertical
		if (component.bcv != component.ncv) {
			if (dpos / component.bch >= component.ncv) {
				dpos += (component.bcv - component.ncv) * component.bch;
			}
		}

		// skip blocks 
		dpos += eobrun;

		// reset eobrun
		eobrun = 0;

		// check position
		if (dpos == component.bc) {
			return CodingStatus::DONE;
		} else if (dpos > component.bc) {
			throw std::runtime_error("dpos greater than block count");
		} else if (rsti > 0) {
			if (rstw == 0) {
				return CodingStatus::RESTART;
			}
		}
	}

	return CodingStatus::OKAY;
}
