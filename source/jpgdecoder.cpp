#include "jpgdecoder.h"


void JpgDecoder::build_trees(const std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2>& hcodes, std::array<std::array<std::unique_ptr<HuffTree>, 4>, 2>& htrees) {
	for (std::size_t i = 0; i < hcodes.size(); i++) {
		for (std::size_t j = 0; j < hcodes[i].size(); j++) {
			if (hcodes[i][j]) {
				htrees[i][j] = std::make_unique<HuffTree>(*hcodes[i][j]);
			}
		}
	}
}


int JpgDecoder::block_seq(const HuffTree& dctree, const HuffTree& actree, short* block) {
	int eob = 64;
	int bpos;
	int hc;


	// decode dc
	if (this->dc_prg_fs(dctree, block) == CodingStatus::ERROR) {
		return -1; // Return error
	}

	// decode ac
	for (bpos = 1; bpos < 64;) {
		// decode next
		hc = actree.next_huffcode(huffr);
		// analyse code
		if (hc > 0) {
			std::uint8_t z = bitops::LBITS(hc, 4);
			std::uint8_t s = bitops::RBITS(hc, 4);
			std::uint16_t n = huffr->read(s);
			if ((z + bpos) >= 64)
				return -1; // run is to long
			while (z > 0) { // write zeroes
				block[bpos++] = 0;
				z--;
			}
			block[bpos++] = static_cast<short>(devli(s, n)); // decode cvli
		} else if (hc == 0) { // EOB
			eob = bpos;
			// while( bpos < 64 ) // fill remaining block with zeroes
			//	block[ bpos++ ] = 0;
			break;
		} else {
			return -1; // return error
		}
	}


	// return position of eob
	return eob;
}

CodingStatus JpgDecoder::dc_prg_fs(const HuffTree& dctree, short* block) {
	// decode dc
	int hc = dctree.next_huffcode(huffr);
	if (hc < 0) {
		return CodingStatus::ERROR; // return error
	}
	int s = hc;
	std::uint16_t n = huffr->read(s);
	block[0] = devli(s, n);

	// return 0 if everything is ok
	return CodingStatus::OKAY;
}

int JpgDecoder::ac_prg_fs(const HuffTree& actree, short* block, int* eobrun, int from, int to) {
	int eob = to + 1;
	int bpos;
	int hc;
	int l;
	int r;


	// decode ac
	for (bpos = from; bpos <= to;) {
		// decode next
		hc = actree.next_huffcode(huffr);
		if (hc < 0)
			return -1;
		l = bitops::LBITS(hc, 4);
		r = bitops::RBITS(hc, 4);
		// analyse code
		if ((l == 15) || (r > 0)) { // decode run/level combination
			std::uint8_t z = l;
			std::uint8_t s = r;
			std::uint16_t n = huffr->read(s);
			if ((z + bpos) > to)
				return -1; // run is to long			
			while (z > 0) { // write zeroes
				block[bpos++] = 0;
				z--;
			}
			block[bpos++] = static_cast<short>(devli(s, n)); // decode cvli
		} else { // decode eobrun
			eob = bpos;
			std::uint8_t s = l;
			std::uint16_t n = huffr->read(s);
			(*eobrun) = e_devli(s, n);
			// while( bpos <= to ) // fill remaining block with zeroes
			//	block[ bpos++ ] = 0;
			break;
		}
	}


	// return position of eob
	return eob;
}

void JpgDecoder::dc_prg_sa(short* block) {
	// decode next bit of dc coefficient
	block[0] = huffr->read_bit();
}

int JpgDecoder::ac_prg_sa(const HuffTree& actree, short* block, int* eobrun, int from, int to) {
	signed char z;
	signed char v;
	int bpos = from;
	int eob = to;
	int hc;
	int l;
	int r;


	// decode AC succesive approximation bits
	if ((*eobrun) == 0)
		while (bpos <= to) {
			// decode next
			hc = actree.next_huffcode(huffr);
			if (hc < 0)
				return -1;
			l = bitops::LBITS(hc, 4);
			r = bitops::RBITS(hc, 4);
			// analyse code
			if ((l == 15) || (r > 0)) { // decode run/level combination
				z = l;
				std::uint8_t s = r;
				if (s == 0)
					v = 0;
				else if (s == 1) {
					std::uint8_t n = huffr->read_bit();
					v = (n == 0) ? -1 : 1; // fast decode vli
				} else
					return -1; // decoding error
				// write zeroes / write correction bits
				while (true) {
					if (block[bpos] == 0) { // skip zeroes / write value
						if (z > 0)
							z--;
						else {
							block[bpos++] = v;
							break;
						}
					} else { // read correction bit
						std::int16_t n = huffr->read_bit();
						block[bpos] = (block[bpos] > 0) ? n : -n;
					}
					if (bpos++ >= to)
						return -1; // error check					
				}
			} else { // decode eobrun
				eob = bpos;
				std::uint8_t s = l;
				std::uint16_t n = huffr->read(s);
				(*eobrun) = e_devli(s, n);
				break;
			}
		}

	// read after eob correction bits
	if ((*eobrun) > 0) {
		for (; bpos <= to; bpos++) {
			if (block[bpos] != 0) {
				std::int16_t n = huffr->read_bit();
				block[bpos] = (block[bpos] > 0) ? n : -n;
			}
		}
	}

	// return eob
	return eob;
}

void JpgDecoder::eobrun_sa(short* block, int from, int to) {
	// fast eobrun decoding routine for succesive approximation
	for (int bpos = from; bpos <= to; bpos++) {
		if (block[bpos] != 0) {
			std::int16_t n = huffr->read_bit();
			block[bpos] = (block[bpos] > 0) ? n : -n;
		}
	}
}

CodingStatus JpgDecoder::skip_eobrun(const Component& cmpt, int rsti, int* dpos, int* rstw, int* eobrun) {
	if ((*eobrun) > 0) // error check for eobrun
	{
		// compare rst wait counter if needed
		if (rsti > 0) {
			if ((*eobrun) > (*rstw))
				return CodingStatus::ERROR;
			else
				(*rstw) -= (*eobrun);
		}

		// fix for non interleaved mcu - horizontal
		if (cmpt.bch != cmpt.nch) {
			(*dpos) += ((((*dpos) % cmpt.bch) + (*eobrun)) /
				cmpt.nch) * (cmpt.bch - cmpt.nch);
		}

		// fix for non interleaved mcu - vertical
		if (cmpt.bcv != cmpt.ncv) {
			if ((*dpos) / cmpt.bch >= cmpt.ncv)
				(*dpos) += (cmpt.bcv - cmpt.ncv) *
					cmpt.bch;
		}

		// skip blocks 
		(*dpos) += (*eobrun);

		// reset eobrun
		(*eobrun) = 0;

		// check position
		if ((*dpos) == cmpt.bc)
			return CodingStatus::DONE;
		else if ((*dpos) > cmpt.bc)
			return CodingStatus::ERROR;
		else if (rsti > 0)
			if ((*rstw) == 0)
				return CodingStatus::RESTART;
	}

	return CodingStatus::OKAY;
}
