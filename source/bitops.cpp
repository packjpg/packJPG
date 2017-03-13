/*
This file contains special classes for bitwise
reading and writing of arrays
*/

#include "bitops.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

#if defined(_WIN32) || defined(WIN32)
#include <fcntl.h>
#include <io.h>
#endif

/* -----------------------------------------------
	constructor for abitreader class
	----------------------------------------------- */

abitreader::abitreader(const std::vector<std::uint8_t>& bits) : data(bits) {
	eof_ = data.empty();
}

/* -----------------------------------------------
	destructor for abitreader class
	----------------------------------------------- */

abitreader::~abitreader() {}

/* -----------------------------------------------
	reads n bits from abitreader
	----------------------------------------------- */

unsigned int abitreader::read(int nbits) {
	unsigned int retval = 0;

	// safety check for eof
	if (eof()) {
		peof_ += nbits;
		return 0;
	}

	while (nbits >= cbit) {
		nbits -= cbit;
		retval |= (RBITS( data[cbyte], cbit ) << nbits);
		cbit = 8;
		cbyte++;
		if (cbyte >= data.size()) {
			peof_ = nbits;
			eof_ = true;
			return retval;
		}
	}

	if (nbits > 0) {
		retval |= (MBITS( data[cbyte], cbit, (cbit-nbits) ));
		cbit -= nbits;
	}

	return retval;
}

/* -----------------------------------------------
	reads one bit from abitreader
	----------------------------------------------- */

unsigned char abitreader::read_bit() {
	unsigned char bit;

	// safety check for eof
	if (eof()) {
		peof_++;
		return 0;
	}

	// read one bit
	bit = BITN( data[cbyte], --cbit );
	if (cbit == 0) {
		cbyte++;
		if (cbyte == data.size()) {
			eof_ = true;
		}
		cbit = 8;
	}

	return bit;
}

/* -----------------------------------------------
	to skip padding from current byte
	----------------------------------------------- */

unsigned char abitreader::unpad(unsigned char fillbit) {
	if ((cbit == 8) || eof()) {
		return fillbit;
	} else {
		fillbit = read(1);
		while (cbit != 8)
			read(1);
	}

	return fillbit;
}

/* -----------------------------------------------
	get current position in array
	----------------------------------------------- */

int abitreader::getpos() {
	return cbyte;
}

/* -----------------------------------------------
	get current bit position
	----------------------------------------------- */

int abitreader::getbitp() {
	return cbit;
}

/* -----------------------------------------------
	set byte and bit position
	----------------------------------------------- */

void abitreader::setpos(int pbyte, int pbit) {
	if (pbyte < data.size()) {
		// reset eof
		eof_ = false;
		// set positions
		cbyte = pbyte;
		cbit = pbit;
	} else {
		// set eof
		eof_ = true;
		// set positions
		cbyte = data.size();
		cbit = 8;
		peof_ = ((pbyte - data.size()) * 8) + 8 - pbit;
	}
}

/* -----------------------------------------------
	rewind n bits
	----------------------------------------------- */

void abitreader::rewind_bits(int nbits) {
	if (eof()) {
		if (nbits > peof_) {
			nbits -= peof_;
			peof_ = 0;
		} else {
			peof_ -= nbits;
			return;
		}
		eof_ = false;
	}

	cbit += nbits;
	cbyte -= cbit / 8;
	cbit = cbit % 8;
	if (cbyte < 0) {
		cbyte = 0;
		cbit = 8;
	}
}

bool abitreader::eof() {
	return eof_;
}

int abitreader::peof() {
	return peof_;
}


/* -----------------------------------------------
	constructor for abitwriter class
	----------------------------------------------- */

abitwriter::abitwriter(int size) : data(std::max(size, 65536)) {}

/* -----------------------------------------------
	destructor for abitwriter class
	----------------------------------------------- */

abitwriter::~abitwriter() {}

/* -----------------------------------------------
	writes n bits to abitwriter
	----------------------------------------------- */

void abitwriter::write(unsigned int val, int nbits) {
	// safety check for error
	if (nbits < 0) {
		return;
	}

	// test if pointer beyond flush treshold
	if (cbyte > (data.size() - 5)) {
		data.resize(data.size() * 2);
	}

	// write data
	while (nbits >= cbit) {
		data[cbyte] |= (MBITS32(val, nbits, (nbits-cbit)));
		nbits -= cbit;
		cbyte++;
		cbit = 8;
	}

	if (nbits > 0) {
		data[cbyte] |= ((RBITS32(val, nbits)) << (cbit - nbits));
		cbit -= nbits;
	}
}

/* -----------------------------------------------
	writes one bit to abitwriter
	----------------------------------------------- */

void abitwriter::write_bit(unsigned char bit) {

	// write data
	if (bit) {
		data[cbyte] |= 0x1 << (--cbit);
	} else {
		--cbit;
	}
	if (cbit == 0) {
		// test if pointer beyond flush treshold
		cbyte++;
		if (cbyte > (data.size() - 5)) {
			data.resize(data.size() * 2);
		}
		cbit = 8;
	}
}

/* -----------------------------------------------
	Sets the fillbit for padding data.
   ----------------------------------------------- */
void abitwriter::set_fillbit(unsigned char fillbit) {
	fillbit_ = fillbit;
}


/* -----------------------------------------------
	pads data using fillbit
	----------------------------------------------- */

void abitwriter::pad() {
	while (cbit < 8) {
		write(fillbit_, 1);
	}
}

/* -----------------------------------------------
	gets data array from abitwriter
	----------------------------------------------- */

std::vector<std::uint8_t> abitwriter::get_data() {
	pad(); // Pad the last bits of the data before returning it.
	data.resize(cbyte);
	return data;
}

/* -----------------------------------------------
	gets size of data array from abitwriter
	----------------------------------------------- */

int abitwriter::getpos() {
	return cbyte;
}

/* -----------------------------------------------
	get current bit position
	----------------------------------------------- */

int abitwriter::getbitp() {
	return cbit;
}


/* -----------------------------------------------
	constructor for abytewriter class
	----------------------------------------------- */

abytereader::abytereader(const std::vector<std::uint8_t>& bytes) : data(bytes) {
	cbyte = 0;
	_eof = bytes.empty();
}

/* -----------------------------------------------
	destructor for abytewriter class
	----------------------------------------------- */

abytereader::~abytereader() {}

/* -----------------------------------------------
	reads 1 byte from abytereader
	----------------------------------------------- */

int abytereader::read(unsigned char* byte) {
	if (cbyte >= data.size()) {
		cbyte = data.size();
		_eof = true;
		return 0;
	} else {
		*byte = data[cbyte];
		cbyte++;
		_eof = cbyte >= data.size();
		return 1;
	}
}

/* -----------------------------------------------
	reads n bytes from abytereader
	----------------------------------------------- */
	
int abytereader::read_n( unsigned char* byte, int n )
{
	if (n <= 0 || byte == nullptr) {
		return 0;
	}
	int numAvailable = data.size() - cbyte;
	int numRead = std::min(numAvailable, n);
	auto start = std::next(std::begin(data), cbyte);
	auto end = std::next(std::begin(data), cbyte + numRead);
	std::copy(start, end, byte);
	cbyte += numRead;
	_eof = cbyte >= data.size();
	return numRead;
}

/* -----------------------------------------------
	go to position in data
	----------------------------------------------- */
	
void abytereader::seek( int pos )
{
	int newPos = std::max(pos, 0);
	cbyte = std::min(newPos, int(data.size()));
	_eof = cbyte >= data.size();
}

/* -----------------------------------------------
	gets size of current data
	----------------------------------------------- */
	
int abytereader::getsize()
{
	return data.size();
}

/* -----------------------------------------------
	gets current position from abytereader
	----------------------------------------------- */	

int abytereader::getpos()
{
	return cbyte;
}

bool abytereader::eof()
{
	return _eof;
}


/* -----------------------------------------------
	constructor for abytewriter class
	----------------------------------------------- */	

abytewriter::abytewriter(int size) : data(std::max(size, 65536)) {}

/* -----------------------------------------------
	destructor for abytewriter class
	----------------------------------------------- */	

abytewriter::~abytewriter() {}

/* -----------------------------------------------
	writes 1 byte to abytewriter
	----------------------------------------------- */

void abytewriter::write(unsigned char byte) {
	// test if pointer beyond flush threshold
	if (cbyte == data.size()) {
		data.resize(data.size() * 2);
	}

	// write data
	data[cbyte] = byte;
	cbyte++;
}

/* -----------------------------------------------
	writes n byte to abytewriter
	----------------------------------------------- */

void abytewriter::write_n(const unsigned char* byte, int n) {
	// safety check for error
	if (n <= 0) {
		return;
	}

	// make sure that pointer doesn't get beyond flush threshold
	while (cbyte + n >= data.size()) {
		data.resize(data.size() * 2);
	}

	std::copy(byte, byte + n, std::next(std::begin(data), cbyte));
	cbyte += n;
}

/* -----------------------------------------------
	gets data array from abytewriter
	----------------------------------------------- */

std::vector<std::uint8_t> abytewriter::get_data()
{
	// realloc data
	data.resize(cbyte);
	
	return data;
}

/* -----------------------------------------------
	gets size of data array from abytewriter
	----------------------------------------------- */	

int abytewriter::getpos()
{
	return cbyte;
}

/* -----------------------------------------------
	reset without realloc
	----------------------------------------------- */	
	
void abytewriter::reset()
{
	// set position of current byte
	cbyte = 0;
}


/* -----------------------------------------------
	constructor for iostream class
	----------------------------------------------- */

iostream::iostream(const std::vector<std::uint8_t>& bytes, StreamMode iomode) : data(bytes), mode(iomode), srct(StreamType::kMemory) {
	open_mem();
}

iostream::iostream(StreamMode iomode) : mode(iomode), srct(StreamType::kStream) {
#if defined(_WIN32) || defined(WIN32)
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
#endif
	open_stream();
}

iostream::iostream() {
}

/* -----------------------------------------------
	destructor for iostream class
	----------------------------------------------- */

iostream::~iostream()
{
	// if needed, write memory to stream or free memory from buffered stream
	if ( srct == StreamType::kStream) {
		if ( mode == StreamMode::kWrite ) {
			const auto& data = mwrt->get_data();
			fwrite(data.data(), sizeof(std::uint8_t), data.size(), stdout);
		}
	}
}

/* -----------------------------------------------
	switches mode from reading to writing and vice versa
	----------------------------------------------- */
	
void iostream::switch_mode()
{	
	// return immediately if there's an error
	if ( chkerr() ) return;
	
	
	if ( mode == StreamMode::kRead) {
		// WARNING: when switching from reading to writing, information might be lost forever
		switch ( srct ) {
			case StreamType::kFile:
				break;
			case StreamType::kMemory:
			case StreamType::kStream:
				mrdr.reset();
				mwrt = std::make_unique<abytewriter>( data.size() );
				break;
			default:
				break;
		}
		mode = StreamMode::kWrite;
	}
	else {
		// switching from writing to reading is a bit more complicated
		switch ( srct ) {
			case StreamType::kFile:
				break;
			case StreamType::kMemory:
			case StreamType::kStream:
				mrdr = std::make_unique<abytereader>(mwrt->get_data());
				mwrt.reset();
				break;
			default:
				break;
		}
		mode = StreamMode::kRead;
	}
}

/* -----------------------------------------------
	generic read function
	----------------------------------------------- */
	
int iostream::read(unsigned char* to, int dtsize)
{
	return read_mem( to, dtsize );
}

bool iostream::read_byte(unsigned char* to) {
	return  read_mem_byte(to);
}

/* -----------------------------------------------
	generic write function
	----------------------------------------------- */

int iostream::write(const unsigned char* from, int dtsize )
{
	return write_mem( from, dtsize );
}

int iostream::write_byte(unsigned char byte) {
	return write_mem_byte(byte);
}

/* -----------------------------------------------
	rewind to beginning of stream
	----------------------------------------------- */

int iostream::rewind()
{
	// WARNING: when writing, rewind might lose all your data
	if ( mode == StreamMode::kRead )
		mrdr->seek( 0 );
	else
		mwrt->reset();
	
	return getpos();
}

/* -----------------------------------------------
	get current position in stream
	----------------------------------------------- */

int iostream::getpos()
{
	int pos;
	
	if ( mode == StreamMode::kRead )
		pos = mrdr->getpos();
	else
		pos = mwrt->getpos();

	return pos;
}

/* -----------------------------------------------
	get size of file
	----------------------------------------------- */

int iostream::getsize()
{
	int siz;

	if (mode == StreamMode::kRead) {
		siz = mrdr->getsize();
	} else {
		siz = getpos();
	}

	return siz;
}

/* -----------------------------------------------
	get data pointer (for mem io only)
	----------------------------------------------- */

std::vector<std::uint8_t> iostream::get_data()
{
	if (srct == StreamType::kMemory) {
		return (mode == StreamMode::kRead) ? data : mwrt->get_data();
	} else {
		std::vector<uint8_t> buffer(getsize());
		read(buffer.data(), buffer.size());
		return buffer;
	}
}

/* -----------------------------------------------
	check for errors
	----------------------------------------------- */
	
bool iostream::chkerr()
{
	bool error = false;
	
	// check for io errors
	if ( mode == StreamMode::kRead ) {
		if ( mrdr == nullptr )			
			error = true;
	} else {		
		if ( mwrt == nullptr )
			error = true;
	}
	
	return error;
}

/* -----------------------------------------------
	check for eof (read only)
	----------------------------------------------- */
	
bool iostream::chkeof()
{
	return false;
}

/* -----------------------------------------------
	open function for memory
	----------------------------------------------- */

void iostream::open_mem()
{
	if ( mode == StreamMode::kRead )
		mrdr = std::make_unique<abytereader>(std::vector<std::uint8_t>(data));
	else
		mwrt = std::make_unique<abytewriter>(data.size());
}

/* -----------------------------------------------
	open function for streams
	----------------------------------------------- */

void iostream::open_stream()
{	
	
	if ( mode == StreamMode::kRead ) {
		// read whole stream into memory buffer
		auto strwrt = std::make_unique<abytewriter>( 0 );
		constexpr int buffer_capacity = 1024 * 1024;
    std::vector<unsigned char> buffer(buffer_capacity);

		int bytesRead = fread(buffer.data(), sizeof(buffer[0]), buffer_capacity, stdin);
		while (bytesRead > 0) {
			strwrt->write_n(buffer.data(), bytesRead);
			bytesRead = fread(buffer.data(), sizeof(buffer[0]), buffer_capacity, stdin);
		}
		data = strwrt->get_data();
	}
	
	// for writing: simply open new stream in mem writer
	// writing to stream will be done later
	open_mem();
}

/* -----------------------------------------------
	write function for memory
	----------------------------------------------- */
	
int iostream::write_mem(const unsigned char* from, int dtsize )
{	
	mwrt->write_n(from, dtsize);
	
	return dtsize;
}

int iostream::write_mem_byte(unsigned char byte) {
	mwrt->write(byte);
	return 1;
}

/* -----------------------------------------------
	read function for memory
	----------------------------------------------- */

int iostream::read_mem(unsigned char* to, int dtsize)
{	
	return mrdr->read_n(to, dtsize);
}

bool iostream::read_mem_byte(unsigned char* to) {
	return mrdr->read(to) == 1;
}

FileStream::FileStream(const std::string& file, StreamMode iomode) : file_path(file), io_mode(iomode) {
	iostream();
	// open file for reading / writing
	fptr = fopen(file_path.c_str(), (io_mode == StreamMode::kRead) ? "rb" : "wb");
	if (fptr != nullptr) {
		file_buffer.reserve(32768);
		std::setvbuf(fptr, file_buffer.data(), _IOFBF, file_buffer.capacity());
	}
}

void FileStream::switch_mode() {
	if (io_mode == StreamMode::kRead) {
		fclose(fptr);
		fptr = fopen(file_path.c_str(), "wb");
		io_mode = StreamMode::kWrite;
	} else {
		fflush(fptr);
		fclose(fptr);
		fptr = fopen(file_path.c_str(), "rb");
		io_mode = StreamMode::kRead;
	}
}

int FileStream::read(unsigned char* to, int dtsize) {
	return fread(to, sizeof(unsigned char), dtsize, fptr);
}

bool FileStream::read_byte(unsigned char* to) {
	const int val = fgetc(fptr);
	*to = val;
	return val != EOF;
}

int FileStream::write(const unsigned char* from, int dtsize) {
	return fwrite(from, sizeof(unsigned char), dtsize, fptr);
}

int FileStream::write_byte(unsigned char byte) {
	return fputc(byte, fptr) == byte;
}

int FileStream::rewind() {
	return fseek(fptr, 0, SEEK_SET);
}

int FileStream::getpos() {
	return ftell(fptr);
}

int FileStream::getsize() {
	const int pos = ftell(fptr);
	fseek(fptr, 0, SEEK_END);
	const int size = ftell(fptr);
	fseek(fptr, pos, SEEK_SET);
	return size;
}

bool FileStream::chkerr() {
	if (fptr == nullptr) {
		return true;
	} else if (ferror(fptr)) {
		return true;
	}
	return false;
}

bool FileStream::chkeof() {
	return  feof(fptr) != 0;
}

std::vector<std::uint8_t> FileStream::get_data() {
	std::vector<uint8_t> buffer(getsize());
	read(buffer.data(), buffer.size());
	return buffer;
}
