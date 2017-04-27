#include "jpgencoder.h"

#include "pjpgtbl.h"
#include "codingstatus.h"
#include <string>
#include "jpg.h"
#include "jfifparse.h"
#include <algorithm>


JpgEncoder::JpgEncoder(Writer& jpg_output_writer, const std::vector<Segment>& segments) :
	jpg_output_writer_(jpg_output_writer),
	segments(segments) {}

void JpgEncoder::recode(FrameInfo& frame_info, std::uint8_t padbit) {
	std::array<std::int16_t, 64> block; // store block for coeffs

	// open huffman coded image data in abitwriter
	auto huffw = std::make_unique<BitWriter>(0); // bitwise writer for image data
	huffw->set_fillbit(padbit);

	// init storage writer
	auto storw = std::make_unique<MemoryWriter>(); // bytewise writer for storage of correction bits

	// preset count of scans and restarts
	int scan_count = 0;
	int rstc = 0; // count of restart markers

	// JPEG decompression loop
	int rsti = 0; // Restart interval.
	std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2> hcodes; // huffman codes
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

		// (re)alloc scan positons array
		scnp.resize(scan_count + 2);

		// (re)alloc restart marker positons array if needed
		if (rsti > 0) {
			int tmp = rstc + ((scan_info.cmpc > 1) ?
				                  (frame_info.mcu_count / rsti) : (frame_info.components[scan_info.cmp[0]].bc / rsti));
			rstp.resize(tmp + 1);
		}

		// intial variables set for encoding
		int cmp = scan_info.cmp[0];
		int csc = 0;
		int mcu = 0;
		int sub = 0;
		int dpos = 0;

		// store scan position
		scnp[scan_count] = huffw->getpos();

		// JPEG imagedata encoding routines
		while (true) {
			// (re)set last DCs for diff coding
			std::array<int, 4> lastdc{}; // last dc for each component

			// (re)set status
			CodingStatus status = CodingStatus::OKAY;

			// (re)set eobrun
			int eobrun = 0; // run of eobs

			// (re)set rst wait counter
			int rstw = rsti; // restart wait counter

			// encoding for interleaved data
			if (scan_info.cmpc > 1) {
				if (frame_info.coding_process == JpegType::SEQUENTIAL) {
					// ---> sequential interleaved encoding <---
					while (status == CodingStatus::OKAY) {
						// copy from colldata
						for (int bpos = 0; bpos < 64; bpos++)
							block[bpos] = frame_info.components[cmp].colldata[bpos][dpos];

						// diff coding for dc
						block[0] -= lastdc[cmp];
						lastdc[cmp] = frame_info.components[cmp].colldata[0][dpos];

						// encode block
						int eob = this->block_seq(*huffw,
							*hcodes[0][frame_info.components[cmp].huffac],
							*hcodes[1][frame_info.components[cmp].huffac],
							block);

						status = jpg::increment_counts(frame_info, scan_info, rsti, mcu, cmp, csc, sub, rstw);
						dpos = jpg::next_mcupos(frame_info, mcu, cmp, sub);

					}
				} else if (scan_info.sah == 0) {
					// ---> progressive interleaved DC encoding <---
					// ---> succesive approximation first stage <---
					while (status == CodingStatus::OKAY) {
						// diff coding & bitshifting for dc 
						int tmp = frame_info.components[cmp].colldata[0][dpos] >> scan_info.sal;
						block[0] = tmp - lastdc[cmp];
						lastdc[cmp] = tmp;

						// encode dc
						this->dc_prg_fs(*huffw,
						                *hcodes[0][frame_info.components[cmp].huffdc],
						                block);

						// next mcupos
						status = jpg::increment_counts(frame_info, scan_info, rsti, mcu, cmp, csc, sub, rstw);
						dpos = jpg::next_mcupos(frame_info, mcu, cmp, sub);
					}
				} else {
					// ---> progressive interleaved DC encoding <---
					// ---> succesive approximation later stage <---
					while (status == CodingStatus::OKAY) {
						// fetch bit from current bitplane
						block[0] = bitops::BITN(frame_info.components[cmp].colldata[0][dpos], scan_info.sal);

						// encode dc correction bit
						this->dc_prg_sa(*huffw, block);

						status = jpg::increment_counts(frame_info, scan_info, rsti, mcu, cmp, csc, sub, rstw);
						dpos = jpg::next_mcupos(frame_info, mcu, cmp, sub);
					}
				}
			} else // encoding for non interleaved data
			{
				if (frame_info.coding_process == JpegType::SEQUENTIAL) {
					// ---> sequential non interleaved encoding <---
					while (status == CodingStatus::OKAY) {
						// copy from colldata
						for (int bpos = 0; bpos < 64; bpos++)
							block[bpos] = frame_info.components[cmp].colldata[bpos][dpos];

						// diff coding for dc
						block[0] -= lastdc[cmp];
						lastdc[cmp] = frame_info.components[cmp].colldata[0][dpos];

						// encode block
						int eob = this->block_seq(*huffw,
						                          *hcodes[0][frame_info.components[cmp].huffac],
						                          *hcodes[1][frame_info.components[cmp].huffac],
						                          block);

						status = jpg::next_mcuposn(frame_info.components[cmp], rsti, dpos, rstw);
					}
				} else if (scan_info.to == 0) {
					if (scan_info.sah == 0) {
						// ---> progressive non interleaved DC encoding <---
						// ---> succesive approximation first stage <---
						while (status == CodingStatus::OKAY) {
							// diff coding & bitshifting for dc 
							int tmp = frame_info.components[cmp].colldata[0][dpos] >> scan_info.sal;
							block[0] = tmp - lastdc[cmp];
							lastdc[cmp] = tmp;

							// encode dc
							this->dc_prg_fs(*huffw,
							                *hcodes[0][frame_info.components[cmp].huffdc],
							                block);

							// ncrement dpos
							status = jpg::next_mcuposn(frame_info.components[cmp], rsti, dpos, rstw);
						}
					} else {
						// ---> progressive non interleaved DC encoding <---
						// ---> succesive approximation later stage <---
						while (status == CodingStatus::OKAY) {
							// fetch bit from current bitplane
							block[0] = bitops::BITN(frame_info.components[cmp].colldata[0][dpos], scan_info.sal);

							// encode dc correction bit
							this->dc_prg_sa(*huffw, block);

							// next mcupos
							status = jpg::next_mcuposn(frame_info.components[cmp], rsti, dpos, rstw);
						}
					}
				} else {
					if (scan_info.sah == 0) {
						// ---> progressive non interleaved AC encoding <---
						// ---> succesive approximation first stage <---
						while (status == CodingStatus::OKAY) {
							// copy from colldata
							for (int bpos = scan_info.from; bpos <= scan_info.to; bpos++)
								block[bpos] =
									fdiv2(frame_info.components[cmp].colldata[bpos][dpos], scan_info.sal);

							// encode block
							int eob = this->ac_prg_fs(*huffw,
							                          *hcodes[1][frame_info.components[cmp].huffac],
							                          block, eobrun, scan_info.from, scan_info.to);

							status = jpg::next_mcuposn(frame_info.components[cmp], rsti, dpos, rstw);
						}

						// encode remaining eobrun
						this->eobrun(*huffw, *hcodes[1][frame_info.components[cmp].huffac], eobrun);
					} else {
						// ---> progressive non interleaved AC encoding <---
						// ---> succesive approximation later stage <---
						while (status == CodingStatus::OKAY) {
							// copy from colldata
							for (int bpos = scan_info.from; bpos <= scan_info.to; bpos++)
								block[bpos] =
									fdiv2(frame_info.components[cmp].colldata[bpos][dpos], scan_info.sal);

							// encode block
							int eob = this->ac_prg_sa(*huffw, *storw,
							                          *hcodes[1][frame_info.components[cmp].huffac],
							                          block, eobrun, scan_info.from, scan_info.to);

							status = jpg::next_mcuposn(frame_info.components[cmp], rsti, dpos, rstw);
						}

						// encode remaining eobrun
						this->eobrun(*huffw, *hcodes[1][frame_info.components[cmp].huffac], eobrun);

						// encode remaining correction bits
						this->crbits(*huffw, *storw);
					}
				}
			}

			// pad huffman writer
			huffw->pad();

			// evaluate status
			if (status == CodingStatus::DONE) {
				scan_count++; // increment scan counter
				break; // leave decoding loop, everything is done here
			} else if (status == CodingStatus::RESTART) {
				if (rsti > 0) // store rstp & stay in the loop
					rstp[rstc++] = huffw->getpos() - 1;
			}
		}
	}

	// get data into huffdata
	huffman_data = huffw->get_data();

	// store last scan & restart positions
	scnp[scan_count] = huffman_data.size();
	if (!rstp.empty()) {
		rstp[rstc] = huffman_data.size();
	}
}


void JpgEncoder::merge(const std::vector<std::uint8_t>& garbage_data, std::vector<std::uint8_t>& rst_err) {
	int rpos = 0; // current restart marker position
	int scan = 1; // number of current scan	

	// write SOI
	constexpr std::array<std::uint8_t, 2> SOI{0xFF, 0xD8};
	jpg_output_writer_.write(SOI);

	// JPEG writing loop
	for (auto& segment : segments) {
		// write segment data to file
		jpg_output_writer_.write(segment.get_data());

		// get out if last marker segment type was not SOS
		if (segment.get_type() != Marker::kSOS) {
			continue;
		}

		// (re)set corrected rst pos
		std::uint32_t cpos = 0; // in scan corrected rst marker position

		// write & expand huffman coded image data
		// ipos is the current position in image data.
		for (auto ipos = scnp[scan - 1]; ipos < scnp[scan]; ipos++) {
			// write current byte
			jpg_output_writer_.write_byte(huffman_data[ipos]);
			// check current byte, stuff if needed
			if (huffman_data[ipos] == 0xFF) {
				jpg_output_writer_.write_byte(std::uint8_t(0)); // 0xFF stuff value
			}
			// insert restart markers if needed
			if (!rstp.empty()) {
				if (ipos == rstp[rpos]) {
					const std::uint8_t rst = 0xD0 + (cpos % 8); // Restart marker
					constexpr std::uint8_t mrk = 0xFF; // marker start
					jpg_output_writer_.write_byte(mrk);
					jpg_output_writer_.write_byte(rst);
					rpos++;
					cpos++;
				}
			}
		}
		// insert false rst markers at end if needed
		if (!rst_err.empty()) {
			while (rst_err[scan - 1] > 0) {
				const std::uint8_t rst = 0xD0 + (cpos % 8); // Restart marker
				constexpr std::uint8_t mrk = 0xFF; // marker start
				jpg_output_writer_.write_byte(mrk);
				jpg_output_writer_.write_byte(rst);
				cpos++;
				rst_err[scan - 1]--;
			}
		}

		// proceed with next scan
		scan++;
	}

	// write EOI
	constexpr std::array<std::uint8_t, 2> EOI{0xFF, 0xD9}; // EOI segment
	jpg_output_writer_.write(EOI);

	// write garbage if needed
	if (!garbage_data.empty()) {
		jpg_output_writer_.write(garbage_data);
	}

	// errormessage if write error
	if (jpg_output_writer_.error()) {
		throw std::runtime_error("write error, possibly drive is full");
	}
}

int JpgEncoder::block_seq(BitWriter& huffw, const HuffCodes& dctbl, const HuffCodes& actbl, const std::array<std::int16_t, 64>& block) {
	// encode DC
	this->dc_prg_fs(huffw, dctbl, block);

	// encode AC
	int z = 0;
	for (int bpos = 1; bpos < 64; bpos++) {
		// if nonzero is encountered
		if (block[bpos] != 0) {
			// write remaining zeroes
			while (z >= 16) {
				huffw.write(actbl.cval[0xF0], actbl.clen[0xF0]);
				z -= 16;
			}
			// vli encode
			int s = pjg::bitlen2048n(block[bpos]);
			std::uint16_t n = envli(s, block[bpos]);
			int hc = ((z << 4) + s);
			// write to huffman writer
			huffw.write(actbl.cval[hc], actbl.clen[hc]);
			huffw.write(n, s);
			// reset zeroes
			z = 0;
		} else { // increment zero counter
			z++;
		}
	}
	// write eob if needed
	if (z > 0) {
		huffw.write(actbl.cval[0x00], actbl.clen[0x00]);
	}

	return 64 - z;
}

void JpgEncoder::dc_prg_fs(BitWriter& huffw, const HuffCodes& dctbl, const std::array<std::int16_t, 64>& block) {
	// encode DC	
	int s = pjg::bitlen2048n(block[0]);
	std::uint16_t n = envli(s, block[0]);
	huffw.write(dctbl.cval[s], dctbl.clen[s]);
	huffw.write(n, s);
}

int JpgEncoder::ac_prg_fs(BitWriter& huffw, const HuffCodes& actbl, const std::array<std::int16_t, 64>& block, int& eobrun, int from, int to) {
	// encode AC
	std::uint8_t z = 0;
	for (int bpos = from; bpos <= to; bpos++) {
		// if nonzero is encountered
		if (block[bpos] != 0) {
			// encode eobrun
			this->eobrun(huffw, actbl, eobrun);
			// write remaining zeroes
			while (z >= 16) {
				huffw.write(actbl.cval[0xF0], actbl.clen[0xF0]);
				z -= 16;
			}
			// vli encode
			std::uint8_t s = pjg::bitlen2048n(block[bpos]);
			std::uint16_t n = envli(s, block[bpos]);
			int hc = ((z << 4) + s);
			// write to huffman writer
			huffw.write(actbl.cval[hc], actbl.clen[hc]);
			huffw.write(n, s);
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
		if (eobrun == actbl.max_eobrun)
			this->eobrun(huffw, actbl, eobrun);
		return 1 + to - z;
	} else {
		return 1 + to;
	}
}

void JpgEncoder::dc_prg_sa(BitWriter& huffw, const std::array<std::int16_t, 64>& block) {
	// enocode next bit of dc coefficient
	huffw.write_bit(block[0]);
}


int JpgEncoder::ac_prg_sa(BitWriter& huffw, ::Writer& storw, const HuffCodes& actbl, const std::array<std::int16_t, 64>& block, int& eobrun, int from, int to) {
	int eob = from;
	int bpos;

	// check if block contains any newly nonzero coefficients and find out position of eob
	for (bpos = to; bpos >= from; bpos--) {
		if ((block[bpos] == 1) || (block[bpos] == -1)) {
			eob = bpos + 1;
			break;
		}
	}

	// encode eobrun if needed
	if ((eob > from) && (eobrun > 0)) {
		this->eobrun(huffw, actbl, eobrun);
		this->crbits(huffw, storw);
	}

	// encode AC
	std::uint8_t z = 0;
	for (bpos = from; bpos < eob; bpos++) {
		// if zero is encountered
		if (block[bpos] == 0) {
			z++; // increment zero counter
			if (z == 16) { // write zeroes if needed
				huffw.write(actbl.cval[0xF0], actbl.clen[0xF0]);
				this->crbits(huffw, storw);
				z = 0;
			}
		}
		// if nonzero is encountered
		else if ((block[bpos] == 1) || (block[bpos] == -1)) {
			// vli encode			
			std::uint8_t s = pjg::bitlen2048n(block[bpos]);
			std::uint16_t n = envli(s, block[bpos]);
			int hc = (z << 4) + s;
			// write to huffman writer
			huffw.write(actbl.cval[hc], actbl.clen[hc]);
			huffw.write(n, s);
			// write correction bits
			this->crbits(huffw, storw);
			// reset zeroes
			z = 0;
		} else { // store correction bits
			std::uint8_t n = block[bpos] & 0x1;
			storw.write_byte(n);
		}
	}

	// fast processing after eob
	for (; bpos <= to; bpos++) {
		if (block[bpos] != 0) { // store correction bits
			std::uint8_t n = block[bpos] & 0x1;
			storw.write_byte(n);
		}
	}

	// check eob, increment eobrun if needed
	if (eob <= to) {
		eobrun++;
		// check eobrun, encode if needed
		if (eobrun == actbl.max_eobrun) {
			this->eobrun(huffw, actbl, eobrun);
			this->crbits(huffw, storw);
		}
	}

	// return eob
	return eob;
}


void JpgEncoder::eobrun(BitWriter& huffw, const HuffCodes& actbl, int& eobrun) {

	if (eobrun > 0) {
		while (eobrun > actbl.max_eobrun) {
			huffw.write(actbl.cval[0xE0], actbl.clen[0xE0]);
			huffw.write(e_envli(14, 32767), 14);
			eobrun -= actbl.max_eobrun;
		}
		std::uint8_t s = bitlen(eobrun);
		s--;
		std::uint16_t n = e_envli(s, eobrun);
		int hc = s << 4;
		huffw.write(actbl.cval[hc], actbl.clen[hc]);
		huffw.write(n, s);
		eobrun = 0;
	}
}

void JpgEncoder::crbits(BitWriter& huffw, Writer& storw) {
	const auto& data = storw.get_data();

	// write bits to huffwriter
	for (std::uint8_t bit : data) {
		huffw.write_bit(bit);
	}

	// reset writer, discard data
	storw.rewind();
}
