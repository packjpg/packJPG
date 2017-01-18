#define RBITS( c, n )		( c & ( 0xFF >> (8 - n) ) )
#define LBITS( c, n )		( c >> (8 - n) )
#define MBITS( c, l, r )	( RBITS( c,l ) >> r )
#define RBITS32( c, n )		( c & ( 0xFFFFFFFF >> (32 - n) ) )
#define MBITS32( c, l, r )	( RBITS32( c,l ) >> r )
#define BITN( c, n )		( (c >> n) & 0x1 )
#define BITLEN( l, v )		for ( l = 0; ( v >> l ) > 0; l++ )
#define FDIV2( v, p )		( ( v < 0 ) ? -( (-v) >> p ) : ( v >> p ) )

#include <memory>

enum StreamType {
	kFile = 0,
	kMemory = 1,
	kStream = 2
};

enum StreamMode {
	kRead = 0,
	kWrite = 1
};

	
/* -----------------------------------------------
	class to read arrays bitwise
	----------------------------------------------- */

class abitreader
{
public:
	abitreader( unsigned char* array, int size );
	~abitreader();	
	unsigned int read( int nbits );
	unsigned char read_bit();
	unsigned char unpad( unsigned char fillbit );
	int getpos();
	int getbitp();
	void setpos( int pbyte, int pbit );
	void rewind_bits( int nbits );
	bool eof();
	int peof();
	
private:
	unsigned char* data;
	int lbyte;
	int cbyte;
	int cbit;
	int peof_;
	bool eof_;
};


/* -----------------------------------------------
	class to write arrays bitwise
	----------------------------------------------- */

class abitwriter
{
public:
	abitwriter( int size );
	~abitwriter();	
	void write( unsigned int val, int nbits );
	void write_bit( unsigned char bit );
	void set_fillbit( unsigned char fillbit );
	void pad();
	unsigned char* getptr();
	int getpos();
	int getbitp();
	bool error();
	
private:
	unsigned char fillbit_;
	unsigned char* data;
	int dsize;
	int cbyte;
	int cbit;
	bool fmem;
	bool error_;
};


/* -----------------------------------------------
	class to read arrays bytewise
	----------------------------------------------- */

class abytereader
{
public:
	abytereader( unsigned char* array, int size );
	~abytereader();	
	int read( unsigned char* byte );
	int read_n( unsigned char* byte, int n );
	void seek( int pos );
	int getsize();
	int getpos();
	bool eof();
	
private:
	unsigned char* data;
	int lbyte;
	int cbyte;
	bool _eof;
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
	void write_n( unsigned char* byte, int n );
	unsigned char* getptr();
	unsigned char* peekptr();
	int getpos();
	void reset();
	bool error();
	
private:
	unsigned char* data;
	int dsize;
	int cbyte;
	bool fmem;
	bool _error;
};


/* -----------------------------------------------
	class for input and output from file or memory
	----------------------------------------------- */

class iostream
{
public:
	iostream( void* src, StreamType srctype, int srcsize, StreamMode iomode );
	~iostream();
	void switch_mode();
	int read( void* to, int tpsize, int dtsize );
	int write( void* from, int tpsize, int dtsize );
	int flush();
	int rewind();
	int getpos();
	int getsize();
	unsigned char* getptr();
	bool chkerr();
	bool chkeof();
	
private:
	void open_file();
	void open_mem();
	void open_stream();
	
	int write_file( void* from, int tpsize, int dtsize );
	int read_file( void* to, int tpsize, int dtsize );
	int write_mem( void* from, int tpsize, int dtsize );
	int read_mem( void* to, int tpsize, int dtsize );
	
	FILE* fptr;
	std::unique_ptr<abytewriter> mwrt;
	std::unique_ptr<abytereader> mrdr;
	
	bool free_mem_sw;
	void* source;
	StreamMode mode;
	StreamType srct;
	int srcs;
};
