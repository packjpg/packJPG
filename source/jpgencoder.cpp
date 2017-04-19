#include "jpgencoder.h"

#include "pjpgtbl.h"

int JpgEncoder::block_seq(const std::unique_ptr<BitWriter>& huffw, const HuffCodes& dctbl, const HuffCodes& actbl, const std::array<std::int16_t, 64>& block) {
	// encode DC
	this->dc_prg_fs(huffw, dctbl, block);

	// encode AC
	int z = 0;
	for (int bpos = 1; bpos < 64; bpos++) {
		// if nonzero is encountered
		if (block[bpos] != 0) {
			// write remaining zeroes
			while (z >= 16) {
				huffw->write(actbl.cval[0xF0], actbl.clen[0xF0]);
				z -= 16;
			}
			// vli encode
			int s = pjg::bitlen2048n(block[bpos]);
			std::uint16_t n = envli(s, block[bpos]);
			int hc = ((z << 4) + s);
			// write to huffman writer
			huffw->write(actbl.cval[hc], actbl.clen[hc]);
			huffw->write(n, s);
			// reset zeroes
			z = 0;
		} else { // increment zero counter
			z++;
		}
	}
	// write eob if needed
	if (z > 0) {
		huffw->write(actbl.cval[0x00], actbl.clen[0x00]);
	}

	return 64 - z;
}

void JpgEncoder::dc_prg_fs(const std::unique_ptr<BitWriter>& huffw, const HuffCodes& dctbl, const std::array<std::int16_t, 64>& block) {
	// encode DC	
	int s = pjg::bitlen2048n(block[0]);
	std::uint16_t n = envli(s, block[0]);
	huffw->write(dctbl.cval[s], dctbl.clen[s]);
	huffw->write(n, s);
}

int JpgEncoder::ac_prg_fs(const std::unique_ptr<BitWriter>& huffw, const HuffCodes& actbl, const std::array<std::int16_t, 64>& block, int* eobrun, int from, int to) {
	int bpos;
	int hc;

	// encode AC
	std::uint8_t z = 0;
	for (bpos = from; bpos <= to; bpos++) {
		// if nonzero is encountered
		if (block[bpos] != 0) {
			// encode eobrun
			this->eobrun(huffw, actbl, eobrun);
			// write remaining zeroes
			while (z >= 16) {
				huffw->write(actbl.cval[0xF0], actbl.clen[0xF0]);
				z -= 16;
			}
			// vli encode
			std::uint8_t s = pjg::bitlen2048n(block[bpos]);
			std::uint16_t n = envli(s, block[bpos]);
			hc = ((z << 4) + s);
			// write to huffman writer
			huffw->write(actbl.cval[hc], actbl.clen[hc]);
			huffw->write(n, s);
			// reset zeroes
			z = 0;
		} else { // increment zero counter
			z++;
		}
	}

	// check eob, increment eobrun if needed
	if (z > 0) {
		(*eobrun)++;
		// check eobrun, encode if needed
		if ((*eobrun) == actbl.max_eobrun)
			this->eobrun(huffw, actbl, eobrun);
		return 1 + to - z;
	} else {
		return 1 + to;
	}
}

void JpgEncoder::dc_prg_sa(const std::unique_ptr<BitWriter>& huffw, const std::array<std::int16_t, 64>& block) {
	// enocode next bit of dc coefficient
	huffw->write_bit(block[0]);
}


int JpgEncoder::ac_prg_sa(const std::unique_ptr<BitWriter>& huffw, const std::unique_ptr<MemoryWriter>& storw, const HuffCodes& actbl, const std::array<std::int16_t, 64>& block, int* eobrun, int from, int to) {
	int eob = from;
	int bpos;
	int hc;

	// check if block contains any newly nonzero coefficients and find out position of eob
	for (bpos = to; bpos >= from; bpos--) {
		if ((block[bpos] == 1) || (block[bpos] == -1)) {
			eob = bpos + 1;
			break;
		}
	}

	// encode eobrun if needed
	if ((eob > from) && ((*eobrun) > 0)) {
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
				huffw->write(actbl.cval[0xF0], actbl.clen[0xF0]);
				this->crbits(huffw, storw);
				z = 0;
			}
		}
		// if nonzero is encountered
		else if ((block[bpos] == 1) || (block[bpos] == -1)) {
			// vli encode			
			std::uint8_t s = pjg::bitlen2048n(block[bpos]);
			std::uint16_t n = envli(s, block[bpos]);
			hc = ((z << 4) + s);
			// write to huffman writer
			huffw->write(actbl.cval[hc], actbl.clen[hc]);
			huffw->write(n, s);
			// write correction bits
			this->crbits(huffw, storw);
			// reset zeroes
			z = 0;
		} else { // store correction bits
			std::uint8_t n = block[bpos] & 0x1;
			storw->write_byte(n);
		}
	}

	// fast processing after eob
	for (; bpos <= to; bpos++) {
		if (block[bpos] != 0) { // store correction bits
			std::uint8_t n = block[bpos] & 0x1;
			storw->write_byte(n);
		}
	}

	// check eob, increment eobrun if needed
	if (eob <= to) {
		(*eobrun)++;
		// check eobrun, encode if needed
		if ((*eobrun) == actbl.max_eobrun) {
			this->eobrun(huffw, actbl, eobrun);
			this->crbits(huffw, storw);
		}
	}

	// return eob
	return eob;
}


void JpgEncoder::eobrun(const std::unique_ptr<BitWriter>& huffw, const HuffCodes& actbl, int* eobrun) {
	int hc;

	if ((*eobrun) > 0) {
		while ((*eobrun) > actbl.max_eobrun) {
			huffw->write(actbl.cval[0xE0], actbl.clen[0xE0]);
			huffw->write(e_envli(14, 32767), 14);
			(*eobrun) -= actbl.max_eobrun;
		}
		std::uint8_t s = bitlen((*eobrun));
		s--;
		std::uint16_t n = e_envli(s, (*eobrun));
		hc = (s << 4);
		huffw->write(actbl.cval[hc], actbl.clen[hc]);
		huffw->write(n, s);
		(*eobrun) = 0;
	}
}

void JpgEncoder::crbits(const std::unique_ptr<BitWriter>& huffw, const std::unique_ptr<MemoryWriter>& storw) {
	const auto& data = storw->get_data();

	// write bits to huffwriter
	for (std::uint8_t bit : data) {
		huffw->write_bit(bit);
	}

	// reset writer, discard data
	storw->rewind();
}
