/*
This file contains special classes for bitwise
reading and writing of arrays
*/

#include "bitops.h"

#include <algorithm>
#include <array>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#if defined(_WIN32) || defined(WIN32)
#include <fcntl.h>
#include <io.h>
#endif


// special realloc with guaranteed free() of previous memory
template <class T>
static inline T* frealloc( T* ptr, size_t size ) {
	T* n_ptr = (T*)realloc( ptr, (size) ? size : 1 );
	if ( n_ptr == nullptr ) free( ptr );
	return n_ptr;
}


/* -----------------------------------------------
	constructor for abitreader class
	----------------------------------------------- */	

abitreader::abitreader( unsigned char* array, int size )
{
	cbyte = 0;	
	cbit = 8;
	peof_ = 0;
	eof_ = false;
	
	data = array;
	lbyte = size;	
}

/* -----------------------------------------------
	destructor for abitreader class
	----------------------------------------------- */	

abitreader::~abitreader()
{
}

/* -----------------------------------------------
	reads n bits from abitreader
	----------------------------------------------- */	

unsigned int abitreader::read( int nbits )
{
	unsigned int retval = 0;
	
	// safety check for eof
	if ( eof()) {
		peof_ += nbits;
		return 0;
	}
	
	while ( nbits >= cbit ) {
		nbits -= cbit;
		retval |= ( RBITS( data[cbyte], cbit ) << nbits );		
		cbit = 8;
		if ( ++cbyte >= lbyte ) {
			peof_ = nbits;
			eof_ = true;
			return retval;
		}
	}
	
	if ( nbits > 0 ) {		
		retval |= ( MBITS( data[cbyte], cbit, (cbit-nbits) ) );
		cbit -= nbits;		
	}
	
	return retval;
}

/* -----------------------------------------------
	reads one bit from abitreader
	----------------------------------------------- */	
	
unsigned char abitreader::read_bit()
{
	unsigned char bit;
	
	// safety check for eof
	if (eof()) {
		peof_++;
		return 0;
	}
	
	// read one bit
	bit = BITN( data[cbyte], --cbit );
	if ( cbit == 0 ) {
		if ( ++cbyte == lbyte ) eof_ = true;
		cbit = 8;
	} 
	
	return bit;
}

/* -----------------------------------------------
	to skip padding from current byte
	----------------------------------------------- */

unsigned char abitreader::unpad( unsigned char fillbit )
{
	if ( ( cbit == 8 ) || eof()) return fillbit;
	else {
		fillbit = read( 1 );
		while ( cbit != 8 ) read( 1 );
	}
	
	return fillbit;
}

/* -----------------------------------------------
	get current position in array
	----------------------------------------------- */	

int abitreader::getpos()
{
	return cbyte;
}

/* -----------------------------------------------
	get current bit position
	----------------------------------------------- */
	
int abitreader::getbitp()
{
	return cbit;
}

/* -----------------------------------------------
	set byte and bit position
	----------------------------------------------- */
	
void abitreader::setpos( int pbyte, int pbit )
{
	if ( pbyte < lbyte ) {
		// reset eof
		eof_ = false;
		// set positions
		cbyte = pbyte;
		cbit = pbit;
	} else {
		// set eof
		eof_ = true;
		// set positions
		cbyte = lbyte;
		cbit = 8;
		peof_ = ( ( pbyte - lbyte ) * 8 ) + 8 - pbit;
	}	
}

/* -----------------------------------------------
	rewind n bits
	----------------------------------------------- */
	
void abitreader::rewind_bits( int nbits )
{
	if ( eof()) {
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
	if ( cbyte < 0 ) {
		cbyte = 0;
		cbit = 8;
	}
}

bool abitreader::eof()
{
	return eof_;
}

int abitreader::peof()
{
	return peof_;
}


/* -----------------------------------------------
	constructor for abitwriter class
	----------------------------------------------- */	

abitwriter::abitwriter( int size )
{
	fillbit_ = 1;
	cbyte   = 0;
	cbit    = 8;
	
	error_ = false;
	fmem  = true;
	
	dsize = std::max(size, 65536);
	data = ( unsigned char* ) calloc ( dsize, sizeof(unsigned char) );
	if ( data == nullptr ) {
		error_ = true;
		return;
	}
}

/* -----------------------------------------------
	destructor for abitwriter class
	----------------------------------------------- */	

abitwriter::~abitwriter()
{
	// free memory if pointer was not given out
	if ( fmem )	free( data );
}

/* -----------------------------------------------
	writes n bits to abitwriter
	----------------------------------------------- */	

void abitwriter::write( unsigned int val, int nbits )
{
	// safety check for error
	if ( error() || nbits < 0 ) return;
	
	// test if pointer beyond flush treshold
	if ( cbyte > ( dsize - 5 ) ) {
		data = frealloc( data, dsize * 2 );
		if ( data == nullptr ) {
			error_ = true;
			return;
		}
		dsize *= 2;
		std::fill(data + cbyte + 1, data + dsize, unsigned char(0));
	}
	
	// write data
	while ( nbits >= cbit ) {
		data[cbyte] |= ( MBITS32(val, nbits, (nbits-cbit)) );		
		nbits -= cbit;		
		cbyte++;
		cbit = 8;
	}
	
	if ( nbits > 0 ) {		
		data[cbyte] |= ( (RBITS32(val, nbits)) << (cbit - nbits) );
		cbit -= nbits;		
	}	
}

/* -----------------------------------------------
	writes one bit to abitwriter
	----------------------------------------------- */	

void abitwriter::write_bit( unsigned char bit )
{
	// safety check for error
	if ( error() ) return;
	
	// write data
	if ( bit ) data[cbyte] |= 0x1 << (--cbit);
	else --cbit;
	if ( cbit == 0 ) {
		// test if pointer beyond flush treshold
		if ( ++cbyte > ( dsize - 5 ) ) {
			data = frealloc( data, dsize * 2 );
			if ( data == nullptr ) {
				error_ = true;
				return;
			}
			dsize *= 2;
			std::fill(data + cbyte + 1, data + dsize, unsigned char(0));
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
	
void abitwriter::pad()
{
	while ( cbit < 8 )
		write( fillbit_, 1 );
}

/* -----------------------------------------------
	gets data array from abitwriter
	----------------------------------------------- */	

unsigned char* abitwriter::getptr()
{
	// data is padded here
	pad();
	// forbid freeing memory
	fmem = false;
	// realloc data
	data = frealloc( data, cbyte );
	
	return data;
}

/* -----------------------------------------------
	gets size of data array from abitwriter
	----------------------------------------------- */	

int abitwriter::getpos()
{
	return cbyte;
}

/* -----------------------------------------------
	get current bit position
	----------------------------------------------- */
	
int abitwriter::getbitp()
{
	return cbit;
}

bool abitwriter::error()
{
	return error_;
}


/* -----------------------------------------------
	constructor for abytewriter class
	----------------------------------------------- */

abytereader::abytereader( unsigned char* array, int size )
{
	cbyte = 0;
	_eof = false;
	
	data = array;
	lbyte = size;
	
	if ( ( data == nullptr ) || ( lbyte == 0 ) )
		_eof = true;
}

/* -----------------------------------------------
	destructor for abytewriter class
	----------------------------------------------- */

abytereader::~abytereader()
{
}

/* -----------------------------------------------
	reads 1 byte from abytereader
	----------------------------------------------- */

int abytereader::read( unsigned char* byte )
{
	if ( cbyte >= lbyte ) {
		cbyte = lbyte;
		_eof = true;
		return 0;
	}
	else {
		*byte = data[ cbyte ];
		cbyte++;
		_eof = cbyte >= lbyte;
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
	int numAvailable = lbyte - cbyte;
	int numRead = std::min(numAvailable, n);
	std::copy(data + cbyte, data + cbyte + numRead, byte);
	cbyte += numRead;
	_eof = cbyte >= lbyte;
	return numRead;
}

/* -----------------------------------------------
	go to position in data
	----------------------------------------------- */
	
void abytereader::seek( int pos )
{
	int newPos = std::max(pos, 0);
	cbyte = std::min(newPos, lbyte);
	_eof = cbyte >= lbyte;
}

/* -----------------------------------------------
	gets size of current data
	----------------------------------------------- */
	
int abytereader::getsize()
{
	return lbyte;
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

abytewriter::abytewriter( int size )
{
	cbyte = 0;
	
	_error = false;
	fmem  = true;
	
	dsize = std::max(size, 65536);
	data = (unsigned char*) malloc( dsize );
	if ( data == nullptr ) {
		_error = true;
	}
}

/* -----------------------------------------------
	destructor for abytewriter class
	----------------------------------------------- */	

abytewriter::~abytewriter()
{
	// free data if pointer is not read
	if ( fmem )	free( data );
}

/* -----------------------------------------------
	writes 1 byte to abytewriter
	----------------------------------------------- */	

void abytewriter::write( unsigned char byte )
{
	// safety check for error
	if ( error()) return;
	
	// test if pointer beyond flush threshold
	if ( cbyte >= dsize ) {
		data = frealloc( data, dsize * 2 );
		if ( data == nullptr ) {
			_error = true;
			return;
		}
		dsize *= 2;
	}
	
	// write data
	data[ cbyte ] = byte;
	cbyte++;
}

/* -----------------------------------------------
	writes n byte to abytewriter
	----------------------------------------------- */
	
void abytewriter::write_n( unsigned char* byte, int n )
{
	// safety check for error
	if ( error() || n < 0 ) return;
	
	// make sure that pointer doesn't get beyond flush threshold
	while ( cbyte + n >= dsize ) {
		data = frealloc( data, dsize * 2);
		if ( data == nullptr ) {
			_error = true;
			return;
		}
		dsize *= 2;
	}
	
	std::copy(byte, byte + n, data + cbyte);
	cbyte += n;
}

/* -----------------------------------------------
	gets data array from abytewriter
	----------------------------------------------- */

unsigned char* abytewriter::getptr()
{
	// safety check for error
	if ( error()) return nullptr;
	// forbid freeing memory
	fmem = false;
	// realloc data
	data = frealloc( data, cbyte );
	
	return data;
}

/* -----------------------------------------------
	peeks into data array from abytewriter
	----------------------------------------------- */
	
unsigned char* abytewriter::peekptr()
{
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

bool abytewriter::error()
{
	return _error;
}


/* -----------------------------------------------
	constructor for iostream class
	----------------------------------------------- */

iostream::iostream( void* src, StreamType srctype, int srcsize, StreamMode iomode )
{
	// locally copy source, source type # and io mode #
	source = src;
	srct   = srctype;
	srcs   = srcsize;
	mode   = iomode;
	
	// don't free memory when reading - this will be useful if switching occurs
	free_mem_sw = false;
	
	// set binary mode for streams
	#if defined(_WIN32) || defined(WIN32)
		_setmode( _fileno( stdin ), _O_BINARY);
		_setmode( _fileno( stdout ), _O_BINARY);
	#endif
	
	// open file/mem/stream
	switch ( srct )
	{
		case StreamType::kFile:
			open_file();
			break;
		
		case StreamType::kMemory:
			open_mem();
			break;
		
		case StreamType::kStream:
			open_stream();
			break;
		
		default:			
			break;
	}
}

/* -----------------------------------------------
	destructor for iostream class
	----------------------------------------------- */

iostream::~iostream()
{
	// if needed, write memory to stream or free memory from buffered stream
	if ( srct == StreamType::kStream) {
		if ( mode == StreamMode::kWrite ) {
			if ( !(mwrt->error()) ) {
				srcs   = mwrt->getpos();
				source = mwrt->getptr();
				fwrite( source, sizeof( char ), srcs, stdout );
			}
		}
	}
	
	// free all buffers
	if (srct == StreamType::kFile) {
		if (fptr != nullptr) {
			if (mode == StreamMode::kWrite) fflush(fptr);
			fclose(fptr);
		}
	}
	else if (mode == StreamMode::kRead) {
		if (free_mem_sw)
			free(source);
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
				fclose( fptr );
				fptr = fopen( ( char* ) source, "wb" );
				break;
			case StreamType::kMemory:
			case StreamType::kStream:
				mrdr.reset();
				if ( free_mem_sw )
					free( source );
				mwrt = std::make_unique<abytewriter>( srcs );
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
				fflush( fptr );
				fclose( fptr );
				fptr = fopen( ( char* ) source, "rb" );
				break;
			case StreamType::kMemory:
			case StreamType::kStream:
				source = mwrt->getptr();
				srcs   = mwrt->getpos();
				mwrt.reset();
				mrdr = std::make_unique<abytereader>( ( unsigned char* ) source, srcs );
				free_mem_sw = true;
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
	
int iostream::read( void* to, int tpsize, int dtsize )
{
	return ( srct == StreamType::kFile) ? read_file( to, tpsize, dtsize ) : read_mem( to, tpsize, dtsize );
}

/* -----------------------------------------------
	generic write function
	----------------------------------------------- */

int iostream::write( void* from, int tpsize, int dtsize )
{
	return ( srct == StreamType::kFile) ? write_file( from, tpsize, dtsize ) : write_mem( from, tpsize, dtsize );
}

/* -----------------------------------------------
	flush function 
	----------------------------------------------- */

int iostream::flush()
{
	if ( srct == StreamType::kFile)
		fflush( fptr );
	
	return getpos();
}

/* -----------------------------------------------
	rewind to beginning of stream
	----------------------------------------------- */

int iostream::rewind()
{
	// WARNING: when writing, rewind might lose all your data
	if ( srct == StreamType::kFile)
		fseek( fptr, 0, SEEK_SET );
	else if ( mode == StreamMode::kRead )
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
	
	if ( srct == StreamType::kFile)
		pos = ftell( fptr );
	else if ( mode == StreamMode::kRead )
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
	
	if ( mode == StreamMode::kRead ) {
		if ( srct == StreamType::kFile) {
			int pos = ftell( fptr );
			fseek( fptr, 0, SEEK_END );
			siz = ftell( fptr );
			fseek( fptr, pos, SEEK_SET );
		}
		else {
			siz = mrdr->getsize();
		}
	}
	else {
		siz = getpos();
	}

	return siz;
}

/* -----------------------------------------------
	get data pointer (for mem io only)
	----------------------------------------------- */

unsigned char* iostream::getptr()
{
	if ( srct == StreamType::kMemory)
		return ( mode == StreamMode::kRead ) ? ( unsigned char* ) source : mwrt->getptr();
	else
		return nullptr;
}

/* -----------------------------------------------
	check for errors
	----------------------------------------------- */
	
bool iostream::chkerr()
{
	bool error = false;
	
	// check for io errors
	if ( srct == StreamType::kFile) {
		if ( fptr == nullptr )
			error = true;
		else if ( ferror( fptr ) )
			error = true;
	}
	else if ( mode == StreamMode::kRead ) {
		if ( mrdr == nullptr )			
			error = true;
	}
	else {		
		if ( mwrt == nullptr )
			error = true;
		else if ( mwrt->error() )
			error = true;
	}
	
	return error;
}

/* -----------------------------------------------
	check for eof (read only)
	----------------------------------------------- */
	
bool iostream::chkeof()
{
	if ( mode == StreamMode::kRead )
		return ( srct == StreamType::kFile) ? feof( fptr ) != 0 : mrdr->eof();
	else
		return false;
}

/* -----------------------------------------------
	open function for files
	----------------------------------------------- */

void iostream::open_file()
{
	char* fn = (char*) source;
	
	// open file for reading / writing
	fptr = fopen( fn, ( mode == StreamMode::kRead ) ? "rb" : "wb" );
}

/* -----------------------------------------------
	open function for memory
	----------------------------------------------- */

void iostream::open_mem()
{
	if ( mode == StreamMode::kRead )
		mrdr = std::make_unique<abytereader>( ( unsigned char* ) source, srcs );
	else
		mwrt = std::make_unique<abytewriter>( srcs );
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
		if ( strwrt->error() ) {
			source = nullptr;
			srcs   = 0;
		}
		else {
			source = strwrt->getptr();
			srcs   = strwrt->getpos();
		}
		// free memory after done
		free_mem_sw = true;
	}
	
	// for writing: simply open new stream in mem writer
	// writing to stream will be done later
	open_mem();
}

/* -----------------------------------------------
	write function for files
	----------------------------------------------- */

int iostream::write_file( void* from, int tpsize, int dtsize )
{
	return fwrite( from, tpsize, dtsize, fptr );
}

/* -----------------------------------------------
	read function for files
	----------------------------------------------- */

int iostream::read_file( void* to, int tpsize, int dtsize )
{
	return fread( to, tpsize, dtsize, fptr );
}

/* -----------------------------------------------
	write function for memory
	----------------------------------------------- */
	
int iostream::write_mem( void* from, int tpsize, int dtsize )
{
	int n = tpsize * dtsize;
	
	mwrt->write_n( ( unsigned char* ) from, n );
	
	return ( mwrt->error()) ? 0 : n;
}

/* -----------------------------------------------
	read function for memory
	----------------------------------------------- */

int iostream::read_mem( void* to, int tpsize, int dtsize )
{
	int n = tpsize * dtsize;
	
	return ( mrdr->read_n( ( unsigned char* ) to, n ) ) / tpsize;
}
