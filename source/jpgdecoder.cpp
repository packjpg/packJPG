#include "jpgdecoder.h"

#include <string>
#include "jpg.h"
#include "jfifparse.h"

void JpgDecoder::decode(FrameInfo& frame_info, const std::vector<Segment>& segments, const std::vector<std::uint8_t>& huffdata) {
	std::array<std::int16_t, 64> block; // store block for coeffs
	int scan_count = 0; // Count of scans.
	// open huffman coded image data for input in abitreader
	huffr = std::make_unique<BitReader>(huffdata); // bitwise reader for image data
	auto& components = frame_info.components;

	// JPEG decompression loop
	int rsti = 0; // restart interval
	std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2> hcodes; // huffman codes
	std::array<std::array<std::unique_ptr<HuffTree>, 4>, 2> htrees; // huffman decoding trees

	for (const auto& segment : segments) {
		// seek till start-of-scan, parse only DHT, DRI and SOS
		ScanInfo scan_info;
		const Marker type = segment.get_type();
		if (type == Marker::kDHT) {
			try {
				jfif::parse_dht(segment.get_data(), hcodes);
			} catch (const std::range_error&) {
				throw;
			}
			build_trees(hcodes, htrees);
		} else if (type == Marker::kDRI) {
			rsti = jfif::parse_dri(segment.get_data());
		} else if (type == Marker::kSOS) {
			try {
				scan_info = jfif::get_scan_info(frame_info, segment.get_data());
			} catch (std::runtime_error&) {
				throw;
			}
		} else {
			continue;
		}

		// get out if last marker segment type was not SOS
		if (type != Marker::kSOS) {
			continue;
		}

		// check if huffman tables are available
		for (int csc = 0; csc < scan_info.cmpc; csc++) {
			auto& component = components[scan_info.cmp[csc]];
			if ((scan_info.sal == 0 && !htrees[0][component.huffdc]) ||
				(scan_info.sah > 0 && !htrees[1][component.huffac])) {
				throw std::runtime_error("huffman table missing in scan%i" + std::to_string(scan_count));
			}
		}


		// intial variables set for decoding
		int cmp = scan_info.cmp[0];
		int csc = 0;
		int mcu = 0;
		int sub = 0;
		int dpos = 0;

		// JPEG imagedata decoding routines
		CodingStatus status = CodingStatus::OKAY;
		while (status != CodingStatus::DONE) {
			// (re)set status
			int eob;
			status = CodingStatus::OKAY;

			// (re)set last DCs for diff coding
			std::array<int, 4> lastdc{}; // last dc for each component

			// (re)set eobrun
			int eobrun = 0; // run of eobs
			int peobrun = 0; // previous eobrun

			// (re)set rst wait counter
			int rstw = rsti; // restart wait counter

			// decoding for interleaved data
			if (scan_info.cmpc > 1) {
				if (frame_info.coding_process == JpegType::SEQUENTIAL) {
					// ---> sequential interleaved decoding <---
					while (status == CodingStatus::OKAY) {
						// decode block
						try {
							eob = this->block_seq(*htrees[0][components[cmp].huffdc], *htrees[1][components[cmp].huffdc], block);
						} catch (const std::runtime_error&) {
							throw;
						}

						// check for non optimal coding
						if ((eob > 1) && (block[eob - 1] == 0)) {
							throw std::runtime_error("reconstruction of inefficient coding not supported");
						}

						// fix dc
						block[0] += lastdc[cmp];
						lastdc[cmp] = block[0];

						// copy to colldata
						for (int bpos = 0; bpos < eob; bpos++)
							components[cmp].colldata[bpos][dpos] = block[bpos];

						// check for errors, proceed if no error encountered
						status = jpg::increment_counts(frame_info, scan_info, rsti, mcu, cmp, csc, sub, rstw);
						dpos = jpg::next_mcupos(frame_info, mcu, cmp, sub);
					}
				} else if (scan_info.sah == 0) {
					// ---> progressive interleaved DC decoding <---
					// ---> succesive approximation first stage <---
					while (status == CodingStatus::OKAY) {
						try {
							this->dc_prg_fs(*htrees[0][components[cmp].huffdc], block);
						} catch (const std::runtime_error&) {
							throw;
						}

						// fix dc for diff coding
						components[cmp].colldata[0][dpos] = block[0] + lastdc[cmp];
						lastdc[cmp] = components[cmp].colldata[0][dpos];

						// bitshift for succesive approximation
						components[cmp].colldata[0][dpos] <<= scan_info.sal;

						// next mcupos if no error happened
						status = jpg::increment_counts(frame_info, scan_info, rsti, mcu, cmp, csc, sub, rstw);
						dpos = jpg::next_mcupos(frame_info, mcu, cmp, sub);
					}
				} else {
					// ---> progressive interleaved DC decoding <---
					// ---> succesive approximation later stage <---					
					while (status == CodingStatus::OKAY) {
						// decode next bit
						this->dc_prg_sa(block);

						// shift in next bit
						components[cmp].colldata[0][dpos] += block[0] << scan_info.sal;

						status = jpg::increment_counts(frame_info, scan_info, rsti, mcu, cmp, csc, sub, rstw);
						dpos = jpg::next_mcupos(frame_info, mcu, cmp, sub);
					}
				}
			} else { // decoding for non interleaved data
				if (frame_info.coding_process == JpegType::SEQUENTIAL) {
					// ---> sequential non interleaved decoding <---
					while (status == CodingStatus::OKAY) {
						// decode block
						try {
							eob = this->block_seq(*htrees[0][components[cmp].huffdc], *htrees[1][components[cmp].huffdc], block);
						} catch (const std::runtime_error&) {
							throw;
						}

						// check for non optimal coding
						if ((eob > 1) && (block[eob - 1] == 0)) {
							throw std::runtime_error("reconstruction of inefficient coding not supported");
						}

						// fix dc
						block[0] += lastdc[cmp];
						lastdc[cmp] = block[0];

						// copy to colldata
						for (int bpos = 0; bpos < eob; bpos++)
							components[cmp].colldata[bpos][dpos] = block[bpos];

						// check for errors, proceed if no error encountered
						status = jpg::next_mcuposn(components[cmp], rsti, dpos, rstw);
					}
				} else if (scan_info.to == 0) {
					if (scan_info.sah == 0) {
						// ---> progressive non interleaved DC decoding <---
						// ---> succesive approximation first stage <---
						while (status == CodingStatus::OKAY) {
							try {
								this->dc_prg_fs(*htrees[0][components[cmp].huffdc], block);
							} catch (const std::runtime_error&) {
								throw;
							}

							// fix dc for diff coding
							components[cmp].colldata[0][dpos] = block[0] + lastdc[cmp];
							lastdc[cmp] = components[cmp].colldata[0][dpos];

							// bitshift for succesive approximation
							components[cmp].colldata[0][dpos] <<= scan_info.sal;

							// check for errors, increment dpos otherwise
							status = jpg::next_mcuposn(components[cmp], rsti, dpos, rstw);
						}
					} else {
						// ---> progressive non interleaved DC decoding <---
						// ---> succesive approximation later stage <---
						while (status == CodingStatus::OKAY) {
							// decode next bit
							this->dc_prg_sa(block);

							// shift in next bit
							components[cmp].colldata[0][dpos] += block[0] << scan_info.sal;

							// increment dpos
							status = jpg::next_mcuposn(components[cmp], rsti, dpos, rstw);
						}
					}
				} else {
					if (scan_info.sah == 0) {
						// ---> progressive non interleaved AC decoding <---
						// ---> succesive approximation first stage <---
						while (status == CodingStatus::OKAY) {
							if (eobrun == 0) {
								// decode block
								try {
									eob = this->ac_prg_fs(*htrees[1][components[cmp].huffac], scan_info, block, eobrun);
								} catch (const std::runtime_error&) {
									throw;
								}

								if (eobrun > 0) {
									// check for non optimal coding
									if ((eob == scan_info.from) && (peobrun > 0) &&
										(peobrun < hcodes[1][components[cmp].huffac]->max_eobrun - 1)) {
										throw std::runtime_error("reconstruction of inefficient coding not supported");
									}
									peobrun = eobrun;
									eobrun--;
								} else
									peobrun = 0;

								// copy to colldata
								for (int bpos = scan_info.from; bpos < eob; bpos++)
									components[cmp].colldata[bpos][dpos] = block[bpos] << scan_info.sal;
							} else
								eobrun--;

							// check for errors
							try {
								status = this->skip_eobrun(components[cmp], rsti, dpos, rstw, eobrun);
							} catch (const std::runtime_error&) {
								throw;
							}

							// proceed only if no error encountered
							if (status == CodingStatus::OKAY) {
								status = jpg::next_mcuposn(components[cmp], rsti, dpos, rstw);
							}
						}
					} else {
						// ---> progressive non interleaved AC decoding <---
						// ---> succesive approximation later stage <---
						while (status == CodingStatus::OKAY) {
							// copy from colldata
							for (int bpos = scan_info.from; bpos <= scan_info.to; bpos++)
								block[bpos] = components[cmp].colldata[bpos][dpos];

							if (eobrun == 0) {
								// decode block (long routine)
								try {
									eob = this->ac_prg_sa(*htrees[1][components[cmp].huffac], scan_info, block, eobrun);
								} catch (std::runtime_error&) {
									throw;
								}

								if (eobrun > 0) {
									// check for non optimal coding
									if ((eob == scan_info.from) && (peobrun > 0) &&
										(peobrun < hcodes[1][components[cmp].huffac]->max_eobrun - 1)) {
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
								this->eobrun_sa(scan_info, block);
								eobrun--;
							}

							// copy back to colldata
							for (int bpos = scan_info.from; bpos <= scan_info.to; bpos++)
								components[cmp].colldata[bpos][dpos] += block[bpos] << scan_info.sal;

							// proceed only if no error encountered
							status = jpg::next_mcuposn(components[cmp], rsti, dpos, rstw);
						}
					}
				}
			}

			// unpad huffman reader / check padbit
			if (padbit_set) {
				if (padbit != huffr->unpad(padbit)) {
					throw std::runtime_error("inconsistent use of padbits");
					//jpg::padbit = 1;
				}
			} else {
				padbit = huffr->unpad(padbit);
				padbit_set = padbit == 0 || padbit == 1;
			}
		}
		scan_count++;
	}

	// check for missing data
	if (huffr->overread()) {
		throw std::runtime_error("coded image data truncated / too short");
	}

	// check for surplus data
	if (!huffr->eof()) {
		throw std::runtime_error("surplus data found after coded image data");
	}

	huffr = nullptr;
}


void JpgDecoder::check_value_range(const std::vector<Component>& components) {
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

std::uint8_t JpgDecoder::get_padbit() {
	return padbit;
}

void JpgDecoder::build_trees(const std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2>& hcodes, std::array<std::array<std::unique_ptr<HuffTree>, 4>, 2>& htrees) {
	for (std::size_t i = 0; i < hcodes.size(); i++) {
		for (std::size_t j = 0; j < hcodes[i].size(); j++) {
			if (hcodes[i][j] && !htrees[i][j]) {
				htrees[i][j] = std::make_unique<HuffTree>(*hcodes[i][j]);
			}
		}
	}
}


int JpgDecoder::block_seq(const HuffTree& dctree, const HuffTree& actree, std::array<std::int16_t, 64>& block) {
	int eob = 64;
	// decode dc
	try {
		this->dc_prg_fs(dctree, block);
	} catch (const std::runtime_error&) {
		throw;
	}

	// decode ac
	for (std::size_t bpos = 1; bpos < block.size(); bpos++) {
		// decode next
		std::uint8_t hc;
		try {
			hc = actree.next_huffcode(*huffr);
		} catch (const std::runtime_error&) {
			throw;
		}
		// analyse code
		if (hc > 0) {
			std::uint8_t z = bitops::left_nibble(hc);
			std::uint8_t s = bitops::right_nibble(hc);
			std::uint16_t n = huffr->read_u16(s);
			if ((z + bpos) >= block.size()) {
				throw std::runtime_error("Run is too long.");
			}
			std::fill_n(std::begin(block) + bpos, z, std::int16_t(0));
			bpos += z;
			block[bpos] = static_cast<std::int16_t>(devli(s, n)); // decode cvli
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

void JpgDecoder::dc_prg_fs(const HuffTree& dctree, std::array<std::int16_t, 64>& block) {
	// decode dc
	std::uint8_t hc;
	try {
		hc = dctree.next_huffcode(*huffr);
	} catch (const std::runtime_error&) {
		throw;
	}
	int s = hc;
	std::uint16_t n = huffr->read_u16(s);
	block[0] = static_cast<std::int16_t>(devli(s, n));
}

int JpgDecoder::ac_prg_fs(const HuffTree& actree, const ScanInfo& scan_info, std::array<std::int16_t, 64>& block, int& eobrun) {
	int eob = scan_info.to + 1;
	// decode ac
	for (int bpos = scan_info.from; bpos <= scan_info.to;) {
		// decode next
		std::uint8_t hc;
		try {
			hc = actree.next_huffcode(*huffr);
		} catch (const std::runtime_error&) {
			throw;
		}
		auto l = bitops::left_nibble(hc);
		auto r = bitops::right_nibble(hc);
		// analyse code
		if ((l == 15) || (r > 0)) { // decode run/level combination
			std::uint8_t z = l;
			std::uint8_t s = r;
			std::uint16_t n = huffr->read_u16(s);
			if ((z + bpos) > scan_info.to) {
				throw std::runtime_error("Run is too long.");
			}
			std::fill_n(std::begin(block) + bpos, z, std::int16_t(0));
			bpos += z;
			block[bpos] = static_cast<std::int16_t>(devli(s, n)); // decode cvli
			bpos++;
		} else { // decode eobrun
			eob = bpos;
			std::uint8_t s = l;
			std::uint16_t n = huffr->read_u16(s);
			eobrun = e_devli(s, n);
			// while( bpos <= to ) // fill remaining block with zeroes
			//	block[ bpos++ ] = 0;
			break;
		}
	}


	// return position of eob
	return eob;
}

void JpgDecoder::dc_prg_sa(std::array<std::int16_t, 64>& block) {
	// decode next bit of dc coefficient
	block[0] = huffr->read_bit();
}

int JpgDecoder::ac_prg_sa(const HuffTree& actree, const ScanInfo& scan_info, std::array<std::int16_t, 64>& block, int& eobrun) {
	signed char v;
	int bpos = scan_info.from;
	int eob = scan_info.to;
	// decode AC succesive approximation bits
	if (eobrun == 0) {
		while (bpos <= scan_info.to) {
			// decode next
			std::uint8_t hc;
			try {
				hc = actree.next_huffcode(*huffr);
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
					std::uint8_t n = huffr->read_bit();
					v = (n == 0) ? -1 : 1; // fast decode vli
				} else {
					throw std::runtime_error("Decoding error in JpgDecoder::ac_prg_sa");
				}
				// write zeroes / write correction bits
				while (true) {
					if (block[bpos] == 0) { // skip zeroes / write value
						if (z > 0) {
							z--;
						} else {
							block[bpos++] = v;
							break;
						}
					} else { // read correction bit
						std::int16_t n = huffr->read_bit();
						block[bpos] = (block[bpos] > 0) ? n : -n;
					}
					if (bpos++ >= scan_info.to) {
						throw std::runtime_error("bpos larger than to.");
					}
				}
			} else { // decode eobrun
				eob = bpos;
				std::uint8_t s = l;
				std::uint16_t n = huffr->read_u16(s);
				eobrun = e_devli(s, n);
				break;
			}
		}
	}

	// read after eob correction bits
	if (eobrun > 0) {
		for (; bpos <= scan_info.to; bpos++) {
			if (block[bpos] != 0) {
				std::int16_t n = huffr->read_bit();
				block[bpos] = (block[bpos] > 0) ? n : -n;
			}
		}
	}

	// return eob
	return eob;
}

void JpgDecoder::eobrun_sa(const ScanInfo& scan_info, std::array<std::int16_t, 64>& block) {
	// fast eobrun decoding routine for succesive approximation
	for (int bpos = scan_info.from; bpos <= scan_info.to; bpos++) {
		if (block[bpos] != 0) {
			std::int16_t n = huffr->read_bit();
			block[bpos] = (block[bpos] > 0) ? n : -n;
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
