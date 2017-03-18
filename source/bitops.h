#ifndef BITOPS_H
#define BITOPS_H

#define RBITS( c, n )		( c & ( 0xFF >> (8 - n) ) )
#define LBITS( c, n )		( c >> (8 - n) )
#define MBITS( c, l, r )	( RBITS( c,l ) >> r )
#define RBITS32( c, n )		( c & ( 0xFFFFFFFF >> (32 - n) ) )
#define MBITS32( c, l, r )	( RBITS32( c,l ) >> r )
#define BITN( c, n )		( (c >> n) & 0x1 )

#include <cstdint>
#include <memory>
#include <vector>

enum class StreamType {
	kFile = 0,
	kMemory = 1,
	kStream = 2
};

enum class StreamMode {
	kRead = 0,
	kWrite = 1
};

	
/* -----------------------------------------------
	class to read arrays bitwise
	----------------------------------------------- */

class abitreader {
public:
	abitreader(const std::vector<std::uint8_t>& bits);
	~abitreader();
	unsigned int read(int nbits);
	unsigned char read_bit();
	unsigned char unpad(unsigned char fillbit);
	int getpos();
	int getbitp();
	void setpos(int pbyte, int pbit);
	void rewind_bits(int nbits);
	bool eof();
	int peof();

private:
	std::vector<std::uint8_t> data;
	int cbyte = 0;
	int cbit = 8;
	int peof_ = 0;
	bool eof_ = false;
};


/* -----------------------------------------------
	class to write arrays bitwise
	----------------------------------------------- */

class abitwriter {
public:
	abitwriter(int size);
	~abitwriter();
	void write(unsigned int val, int nbits);
	void write_bit(unsigned char bit);
	void set_fillbit(unsigned char fillbit);
	void pad();
	std::vector<std::uint8_t> get_data();
	int getpos();
	int getbitp();

private:
	unsigned char fillbit_ = 1;
	std::vector<std::uint8_t> data;
	int cbyte = 0;
	int cbit = 8;
};


/* -----------------------------------------------
	class to read arrays bytewise
	----------------------------------------------- */

class abytereader {
public:
	abytereader(const std::vector<std::uint8_t>& bytes);
	~abytereader();
	int read(unsigned char* byte);
	int read_n(unsigned char* byte, int n);
	std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0);
	void seek(int pos);
	int getsize();
	int getpos();
	bool eof();
	std::vector<std::uint8_t> get_data();

private:
	const std::vector<std::uint8_t> data;
	int cbyte = 0;
	bool _eof = false;
};


/* -----------------------------------------------
	class to write arrays bytewise
	----------------------------------------------- */

class abytewriter
{
public:
	abytewriter( int size );
	~abytewriter();	
	void write( unsigned char byte );
	void write_n(const unsigned char* byte, int n );
	std::vector<std::uint8_t> get_data();
	int getpos();
	void reset();
	
private:
	std::vector<std::uint8_t> data;
	int cbyte = 0;
};


/* -----------------------------------------------
	class for input and output from file or memory
	----------------------------------------------- */

class iostream
{
public:
	iostream() {}
	virtual ~iostream() {}
	virtual void switch_mode() = 0;
	virtual int read(unsigned char* to, int dtsize) = 0;
	virtual std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0) = 0;
	virtual bool read_byte(unsigned char* to) = 0;
	virtual int write(const unsigned char* from, int dtsize) = 0;
	virtual int write_byte(unsigned char byte) = 0;
	virtual int rewind() = 0;
	virtual int getpos() = 0;
	virtual int getsize() = 0;
	virtual std::vector<std::uint8_t> get_data() = 0;
	virtual bool chkerr() = 0;
	virtual bool chkeof() = 0;
};

class MemStream : public iostream {
public:
	MemStream(StreamMode mode);
	MemStream(const std::vector<std::uint8_t>& bytes, StreamMode mode);
	~MemStream();
	void switch_mode() override;
	int read(unsigned char* to, int dtsize) override;
	std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0) override;
	bool read_byte(unsigned char* to) override;
	int write(const unsigned char* from, int dtsize) override;
	int write_byte(unsigned char byte) override;
	int rewind() override;
	int getpos() override;
	int getsize() override;
	bool chkerr() override;
	bool chkeof() override;
	std::vector<std::uint8_t> get_data() override;

private:
	std::unique_ptr<abytewriter> mwrt;
	std::unique_ptr<abytereader> mrdr;
	StreamMode io_mode = StreamMode::kRead;
	bool is_stream;
};

class FileStream : public iostream {
public:
	FileStream(const std::string& file_path, StreamMode iomode);
	~FileStream();
	void switch_mode() override;
	int read(unsigned char* to, int dtsize) override;
	std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0) override;
	bool read_byte(unsigned char* to) override;
	int write(const unsigned char* from, int dtsize) override;
	int write_byte(unsigned char byte) override;
	int rewind() override;
	int getpos() override;
	int getsize() override;
	bool chkerr() override;
	bool chkeof() override;
	std::vector<std::uint8_t> get_data() override;

private:
	FILE* fptr = nullptr;
	std::vector<char> file_buffer; // Used to replace the default file buffer for reads/writes to improve performance.
	const std::string file_path;
	StreamMode io_mode;
};

#endif