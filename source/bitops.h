#ifndef BITOPS_H
#define BITOPS_H

#include <array>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

constexpr unsigned int rbits_u8( std::uint8_t c, int n ) noexcept {
	return ( n <= 0 ) ? 0u : ( ( n >= 8 ) ? static_cast<unsigned int>( c ) : ( static_cast<unsigned int>( c ) & ( 0xFFu >> ( 8 - n ) ) ) );
}

constexpr unsigned int lbits_u8( std::uint8_t c, int n ) noexcept {
	return ( n <= 0 ) ? 0u : ( ( n >= 8 ) ? static_cast<unsigned int>( c ) : ( static_cast<unsigned int>( c ) >> ( 8 - n ) ) );
}

constexpr unsigned int mbits_u8( std::uint8_t c, int l, int r ) noexcept {
	return ( r <= 0 ) ? rbits_u8( c, l ) : ( ( r >= 8 ) ? 0u : ( rbits_u8( c, l ) >> r ) );
}

template <typename T>
constexpr int bit_n( T c, int n ) noexcept {
	return static_cast<int>( ( static_cast<unsigned int>( c ) >> n ) & 0x1u );
}

constexpr int bit_length_u32( unsigned int v ) noexcept {
	int l = 0;
	while ( v > 0u ) {
		v >>= 1;
		++l;
	}
	return l;
}

constexpr int fdiv2( int v, int p ) noexcept {
	return ( v < 0 )
		? static_cast<int>( -( ( -static_cast<std::int64_t>( v ) ) >> p ) )
		: ( v >> p );
}


/* -----------------------------------------------
	class to read arrays bitwise
	----------------------------------------------- */
class BitReader {
public:
	BitReader( const unsigned char* array, int size ) noexcept
		: data( array ),
		  lbyte( size > 0 ? size : 0 ),
		  eof_( size <= 0 ) {}
	~BitReader() = default;

	inline bool has_fast_16() const noexcept {
		return cbyte + 2 < lbyte;
	}

	inline unsigned int peek_9_fast() const noexcept {
		const unsigned int two = ( static_cast<unsigned int>( data[ cbyte ] ) << 8 )
		                       | static_cast<unsigned int>( data[ cbyte + 1 ] );
		return ( two >> ( cbit - 1 ) ) & 0x1FFu;
	}

	inline void consume_fast( int nbits ) noexcept {
		const int bit_off = ( 8 - cbit ) + nbits;
		cbyte += ( bit_off >> 3 );
		cbit = 8 - ( bit_off & 7 );
	}

	inline unsigned int read( int nbits ) {
		if ( nbits <= 0 ) {
			return 0;
		}
		if ( ( cbyte + 2 < lbyte ) && ( nbits <= 16 ) ) {
			const unsigned int three = ( static_cast<unsigned int>( data[ cbyte ] ) << 16 )
			                         | ( static_cast<unsigned int>( data[ cbyte + 1 ] ) << 8 )
			                         | static_cast<unsigned int>( data[ cbyte + 2 ] );
			const unsigned int retval = ( three >> ( cbit + 16 - nbits ) ) & ( ( 1u << nbits ) - 1u );
			const int bit_off = ( 8 - cbit ) + nbits;
			cbyte += ( bit_off >> 3 );
			cbit = 8 - ( bit_off & 7 );
			return retval;
		}
		return read_slow( nbits );
	}

	inline unsigned char read_bit() {
		if ( eof_ ) {
			peof_++;
			return 0;
		}
		const unsigned char bit = static_cast<unsigned char>( bit_n( data[ cbyte ], --cbit ) );
		if ( cbit == 0 ) {
			update_curr_byte();
		}
		return bit;
	}

	int unpad( int fillbit );
	int getpos() const noexcept { return cbyte; }
	int getbitp() const noexcept { return cbit; }
	void setpos( int pbyte, int pbit );
	void rewind_bits( int nbits );
	bool eof() const noexcept { return eof_; }
	int peof() const noexcept {
		return ( peof_ > std::numeric_limits<int>::max() )
			? std::numeric_limits<int>::max()
			: static_cast<int>( peof_ );
	}

private:
	unsigned int read_slow( int nbits );

	inline void update_curr_byte() noexcept {
		cbyte++;
		eof_ = ( cbyte == lbyte );
		cbit = 8;
	}

	const unsigned char* data = nullptr;
	int lbyte = 0;
	int cbyte = 0;
	int cbit = 8;
	std::int64_t peof_ = 0;
	bool eof_ = false;
};


class BitWriter {
public:
	explicit BitWriter(std::uint8_t padbit) : padbit_(padbit & 1u) {}
	~BitWriter() = default;

	void reserve(std::size_t cap) {
		bytes_.reserve(cap);
	}

	/*
	* Writes the first (lowest) n bits from val (n is assumed to be less than or equal to 16).
	*/
	inline void write_u16(std::uint16_t val, std::size_t num_bits) {
		if (num_bits == 0) {
			return;
		}
		const std::uint32_t v = (num_bits >= 16)
			? static_cast<std::uint32_t>(val)
			: (static_cast<std::uint32_t>(val) & ((1u << num_bits) - 1u));
		if (num_bits < curr_bit_) {
			curr_byte_ |= static_cast<std::uint8_t>(v << (curr_bit_ - num_bits));
			curr_bit_ -= num_bits;
			return;
		}
		bytes_.push_back(static_cast<std::uint8_t>(curr_byte_ | (v >> (num_bits - curr_bit_))));
		num_bits -= curr_bit_;
		while (num_bits >= 8) {
			bytes_.push_back(static_cast<std::uint8_t>(v >> (num_bits - 8)));
			num_bits -= 8;
		}
		curr_byte_ = (num_bits > 0) ? static_cast<std::uint8_t>(v << (8 - num_bits)) : 0;
		curr_bit_ = 8 - num_bits;
	}

	/*
	* Writes the bit (assumed to be 0 or 1).
	*/
	inline void write_bit(std::uint8_t bit) {
		curr_bit_--;
		curr_byte_ |= static_cast<std::uint8_t>((bit & 1u) << curr_bit_);
		if (curr_bit_ == 0) {
			write_curr_byte();
		}
	}

	/**
	* Fills in the remainder of the current byte using the padbit.
	*/
	void pad();
    /**
    * Returns a copy of the bytes written to the writer, padding the current byte if necessary.
    */
	std::vector<std::uint8_t> get_bytes();
	/**
	* Moves out the bytes written to the writer, padding the current byte if necessary.
	*/
	std::vector<std::uint8_t> take_bytes();
    /**
    * Allocates and returns a copy of the bytes written to the writer, padding the current byte if necessary.
    */
    unsigned char* get_c_bytes();
	/*
	* Returns the number of bytes written. If a byte has not been fully written (e.g., only 5 bits have been written),
	* then it is not counted in the number of bytes written.
	*/
	std::size_t num_bytes_written() const {
		return bytes_.size();
	}
private:
	inline void write_curr_byte() {
		bytes_.push_back(curr_byte_);
		curr_byte_ = 0;
		curr_bit_ = 8;
	}

	std::vector<std::uint8_t> bytes_;
	const std::uint8_t padbit_; // A bit, either 0 or 1, to fill the unwritten bits of the current byte with.
	std::uint8_t curr_byte_ = 0; // The current byte being written.

	std::size_t curr_bit_ = 8; // The position of the next bit in the current byte.
};

class Reader {
public:
	Reader() {}

	virtual ~Reader() {}

	virtual std::size_t read(std::uint8_t* to, std::size_t n) = 0;
	virtual std::size_t read(std::vector<std::uint8_t>& into, std::size_t n, std::size_t offset = 0) = 0;
	virtual std::uint8_t read_byte() = 0;
	virtual bool read_byte(std::uint8_t* to) = 0;

	virtual const std::uint8_t* peek_contiguous(std::size_t& avail) const {
		avail = 0;
		return nullptr;
	}

	virtual void skip(std::size_t n) = 0;
	virtual void rewind_bytes(std::size_t n) = 0;
	virtual void rewind() = 0;

	virtual std::size_t num_bytes_read() = 0;
	virtual std::size_t get_size() = 0;
	virtual std::vector<std::uint8_t> get_data() = 0;

    unsigned char* get_c_data();

	virtual bool error() = 0;
	virtual bool end_of_reader() = 0;
};

class MemoryReader : public Reader {
public:
	MemoryReader(const std::vector<std::uint8_t>& bytes);
	MemoryReader(std::vector<std::uint8_t>&& bytes);
    MemoryReader(const std::uint8_t* bytes, std::size_t size);

	~MemoryReader() {}

	std::size_t read(std::uint8_t* to, std::size_t num_to_read) override;
	std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0) override;
	std::uint8_t read_byte() override;
	bool read_byte(std::uint8_t* to) override;
	const std::uint8_t* peek_contiguous(std::size_t& avail) const override;

	void skip(std::size_t n) override;
	void rewind_bytes(std::size_t n) override;
	void rewind() override;

	std::size_t num_bytes_read() override;
	std::size_t get_size() override;
	std::vector<std::uint8_t> get_data() override;
	bool error() override;
	bool end_of_reader() override;

private:
	const std::vector<std::uint8_t> data_;
	std::vector<std::uint8_t>::const_iterator cbyte_;
};

class FileReader : public Reader {
public:
	FileReader(const std::string& file_path);
	~FileReader();

	std::size_t read(std::uint8_t* to, std::size_t num_to_read) override;
	std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0) override;
	std::uint8_t read_byte() override;
	bool read_byte(std::uint8_t* to) override;
	const std::uint8_t* peek_contiguous(std::size_t& avail) const override;

	void skip(std::size_t n) override;
	void rewind_bytes(std::size_t n) override;
	void rewind() override;

	std::size_t num_bytes_read() override;
	std::size_t get_size() override;
	std::vector<std::uint8_t> get_data() override;
	bool error() override;
	bool end_of_reader() override;

private:
	std::unique_ptr<MemoryReader> reader_;
};

class StreamReader : public Reader {
public:
	explicit StreamReader(FILE* stream = stdin);

	~StreamReader() {}

	std::size_t read(std::uint8_t* to, std::size_t num_to_read) override;
	std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0) override;
	std::uint8_t read_byte() override;
	bool read_byte(std::uint8_t* to) override;
	const std::uint8_t* peek_contiguous(std::size_t& avail) const override;

	void skip(std::size_t n) override;
	void rewind_bytes(std::size_t n) override;
	void rewind() override;

	std::size_t num_bytes_read() override;
	std::size_t get_size() override;
	std::vector<std::uint8_t> get_data() override;
	bool error() override;
	bool end_of_reader() override;

private:
	std::unique_ptr<MemoryReader> reader_;
};

class Writer {
public:
	Writer() {}

	virtual ~Writer() {}

	virtual std::size_t write(const std::uint8_t* from, std::size_t n) = 0;
	virtual std::size_t write(const std::vector<std::uint8_t>& bytes) = 0;
	virtual std::size_t write(const std::array<std::uint8_t, 2>& bytes) = 0;

	virtual bool write_byte(std::uint8_t byte) = 0;

	virtual std::vector<std::uint8_t> get_data() = 0;
    unsigned char* get_c_data();

	virtual void reset() = 0;
	virtual bool flush() { return !error(); }
	virtual std::size_t num_bytes_written() = 0;
	virtual bool error() = 0;
};

class MemoryWriter : public Writer {
public:
	MemoryWriter() = default;
	~MemoryWriter() override = default;

	void reserve(std::size_t cap) {
		data_.reserve(cap);
	}

	std::size_t write(const std::uint8_t* from, std::size_t n) override {
		data_.insert(std::end(data_), from, from + n);
		return n;
	}
	std::size_t write(const std::vector<std::uint8_t>& bytes) override {
		data_.insert(std::end(data_), std::begin(bytes), std::end(bytes));
		return bytes.size();
	}
	std::size_t write(const std::array<std::uint8_t, 2>& bytes) override {
		data_.insert(std::end(data_), std::begin(bytes), std::end(bytes));
		return bytes.size();
	}
	bool write_byte(std::uint8_t byte) override {
		data_.push_back(byte);
		return true;
	}

	std::vector<std::uint8_t> get_data() override;
	std::vector<std::uint8_t> take_data();

	void reset() override;
	std::size_t num_bytes_written() override;
	bool error() override;

private:
	std::vector<std::uint8_t> data_;
};

class FileWriter : public Writer {
public:
	FileWriter(const std::string& file_path);
	FileWriter(const FileWriter&) = delete;
	FileWriter& operator=(const FileWriter&) = delete;
	~FileWriter();

	std::size_t write(const std::uint8_t* from, std::size_t n) override;
	std::size_t write(const std::vector<std::uint8_t>& bytes) override;
	std::size_t write(const std::array<std::uint8_t, 2>& bytes) override;
	bool write_byte(std::uint8_t byte) override;

	std::vector<std::uint8_t> get_data() override;

	void reset() override;
	bool flush() override;
	std::size_t num_bytes_written() override;
	bool error() override;

private:
	FILE* fptr_ = nullptr;
	std::vector<char> file_buffer_;
	const std::string file_path_;
};

class StreamWriter : public Writer {
public:
	explicit StreamWriter(FILE* stream = stdout);
	StreamWriter(const StreamWriter&) = delete;
	StreamWriter& operator=(const StreamWriter&) = delete;
	~StreamWriter();

	std::size_t write(const std::uint8_t* from, std::size_t n) override;
	std::size_t write(const std::vector<std::uint8_t>& bytes) override;
	std::size_t write(const std::array<std::uint8_t, 2>& bytes) override;
	bool write_byte(std::uint8_t byte) override;

	std::vector<std::uint8_t> get_data() override;

	void reset() override;
	bool flush() override;
	std::size_t num_bytes_written() override;
	bool error() override;

private:
	FILE* stream_ = stdout;
	std::unique_ptr<MemoryWriter> writer_;
	std::size_t flushed_offset_ = 0;
	bool flushed_ = false;
	bool error_ = false;
};

#endif

