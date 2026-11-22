#include "jpeg_codec.h"

#include <cstring>
#include <limits>

namespace {

bool check_scan_huff_tables( CodecContext& ctx )
{
	for ( int csc = 0; csc < ctx.cs_cmpc; csc++ ) {
		const int cmp = ctx.cs_cmp[ csc ];
		if ( ( ( ctx.jpegtype == 1 || ( ( ctx.cs_cmpc > 1 || ctx.cs_to == 0 ) && ctx.cs_sah == 0 ) ) && ctx.htset[ 0 ][ ctx.cmpnfo[ cmp ].huffdc ] == 0 ) ||
			 ( ctx.jpegtype == 1 && ctx.htset[ 1 ][ ctx.cmpnfo[ cmp ].huffac ] == 0 ) ||
			 ( ctx.cs_cmpc == 1 && ctx.cs_to > 0 && ctx.htset[ 1 ][ ctx.cmpnfo[ cmp ].huffac ] == 0 ) ) {
			ctx.set_errormessage( "huffman table missing in scan%i", ctx.scnc );
			ctx.errorlevel = 2;
			return false;
		}
	}
	return true;
}


bool validate_scan_layout( CodecContext& ctx )
{
	if ( ctx.jpegtype == 1 ) {
		if ( ( ctx.cs_from != 0 ) || ( ctx.cs_to != 63 ) || ( ctx.cs_sah != 0 ) || ( ctx.cs_sal != 0 ) ) {
			ctx.errormessage = "invalid spectral parameters for sequential scan";
			ctx.errorlevel = 2;
			return false;
		}
	}
	else if ( ctx.jpegtype == 2 ) {
		if ( ( ( ctx.cs_from == 0 ) && ( ctx.cs_to != 0 ) ) ||
			 ( ( ctx.cs_from > 0 ) && ( ctx.cs_cmpc != 1 ) ) ||
			 ( ( ctx.cs_sah > 0 ) && ( ctx.cs_sah != ctx.cs_sal + 1 ) ) ) {
			ctx.errormessage = "invalid progressive scan parameters";
			ctx.errorlevel = 2;
			return false;
		}
	}
	else {
		ctx.errormessage = "header information is incomplete";
		ctx.errorlevel = 2;
		return false;
	}
	return true;
}

bool validate_scan_progression( CodecContext& ctx, std::array<std::array<int, 64>, 4>& prog_al )
{
	if ( !validate_scan_layout( ctx ) ) {
		return false;
	}
	for ( int csc = 0; csc < ctx.cs_cmpc; csc++ ) {
		const int cmp = ctx.cs_cmp[ csc ];
		for ( int bpos = ctx.cs_from; bpos <= ctx.cs_to; bpos++ ) {
			if ( ctx.cs_sah == 0 ) {
				if ( prog_al[ cmp ][ bpos ] != -1 ) {
					ctx.errormessage = "duplicate spectral band in scan progression";
					ctx.errorlevel = 2;
					return false;
				}
			}
			else if ( prog_al[ cmp ][ bpos ] != ctx.cs_sah ) {
				ctx.errormessage = "invalid successive approximation progression";
				ctx.errorlevel = 2;
				return false;
			}
			prog_al[ cmp ][ bpos ] = ctx.cs_sal;
		}
	}
	return true;
}

int jpg_encode_eobrun( BitWriter* huffw, const HuffCodes* actbl, int* eobrun )
{
	if ( ( *eobrun ) > 0 ) {
		if ( actbl->max_eobrun == 0 ) {
			return -1;
		}
		while ( ( *eobrun ) > actbl->max_eobrun ) {
			if ( actbl->clen[ 0xE0 ] == 0 ) return -1;
			huffw->write_u16( actbl->cval[ 0xE0 ], actbl->clen[ 0xE0 ] );
			huffw->write_u16( static_cast<std::uint16_t>( e_envli( 14, 32767 ) ), 14 );
			( *eobrun ) -= actbl->max_eobrun;
		}
		int s = bit_length_u32( static_cast<unsigned int>( *eobrun ) );
		s--;
		if ( ( s < 0 ) || ( s > 14 ) ) return -1;
		const auto n = static_cast<unsigned short>( e_envli( s, *eobrun ) );
		const int hc = ( s << 4 );
		if ( actbl->clen[ hc ] == 0 ) return -1;
		huffw->write_u16( actbl->cval[ hc ], actbl->clen[ hc ] );
		huffw->write_u16( n, static_cast<std::size_t>( s ) );
		( *eobrun ) = 0;
	}
	return 0;
}

int jpg_encode_crbits( BitWriter* huffw, std::vector<std::uint8_t>& storw )
{
	for ( const std::uint8_t bit : storw ) {
		huffw->write_bit( bit );
	}
	storw.clear();
	return 0;
}

int jpg_decode_block_seq( BitReader* huffr, const HuffTree* dctree, const HuffTree* actree, short* block )
{
	int eob = 64;

	// decode dc
	int hc = jpg_next_huffcode( huffr, dctree );
	if ( ( hc < 0 ) || ( hc > 12 ) ) return -1;
	unsigned char s = static_cast<unsigned char>( hc );
	unsigned short n = static_cast<unsigned short>( huffr->read( s ) );
	block[ 0 ] = static_cast<short>( devli( s, n ) );

	// decode ac
	for ( int bpos = 1; bpos < 64; ) {
		hc = jpg_next_huffcode( huffr, actree );
		if ( hc > 0 ) {
			unsigned char z = static_cast<unsigned char>( lbits_u8( static_cast<std::uint8_t>( hc ), 4 ) );
			s = static_cast<unsigned char>( rbits_u8( static_cast<std::uint8_t>( hc ), 4 ) );
			if ( ( ( s == 0 ) && ( z != 15 ) ) || ( s > 11 ) )
				return -1;
			n = static_cast<unsigned short>( huffr->read( s ) );
			if ( ( z + bpos ) >= 64 )
				return -1;
			while ( z > 0 ) {
				block[ bpos++ ] = 0;
				z--;
			}
			block[ bpos++ ] = static_cast<short>( devli( s, n ) );
		}
		else if ( hc == 0 ) {
			eob = bpos;
			break;
		}
		else {
			return -1;
		}
	}

	return eob;
}

int jpg_encode_block_seq( BitWriter* huffw, const HuffCodes* dctbl, const HuffCodes* actbl, const short* block )
{
	unsigned char s = static_cast<unsigned char>( jpg_vli_bitlen( block[ 0 ] ) );
	if ( ( s > 12 ) || ( dctbl->clen[ s ] == 0 ) ) return -1;
	unsigned short n = static_cast<unsigned short>( envli( s, block[ 0 ] ) );
	huffw->write_u16( dctbl->cval[ s ], dctbl->clen[ s ] );
	huffw->write_u16( n, s );

	unsigned char z = 0;
	for ( int bpos = 1; bpos < 64; bpos++ ) {
		if ( block[ bpos ] != 0 ) {
			while ( z >= 16 ) {
				if ( actbl->clen[ 0xF0 ] == 0 ) return -1;
				huffw->write_u16( actbl->cval[ 0xF0 ], actbl->clen[ 0xF0 ] );
				z -= 16;
			}
			s = static_cast<unsigned char>( jpg_vli_bitlen( block[ bpos ] ) );
			if ( s > 11 ) return -1;
			n = static_cast<unsigned short>( envli( s, block[ bpos ] ) );
			const int hc = ( z << 4 ) + s;
			if ( actbl->clen[ hc ] == 0 ) return -1;
			huffw->write_u16( actbl->cval[ hc ], actbl->clen[ hc ] );
			huffw->write_u16( n, s );
			z = 0;
		}
		else {
			z++;
		}
	}
	if ( z > 0 ) {
		if ( actbl->clen[ 0x00 ] == 0 ) return -1;
		huffw->write_u16( actbl->cval[ 0x00 ], actbl->clen[ 0x00 ] );
	}

	return 64 - z;
}

int jpg_decode_dc_prg_fs( BitReader* huffr, const HuffTree* dctree, short* block )
{
	const int hc = jpg_next_huffcode( huffr, dctree );
	if ( ( hc < 0 ) || ( hc > 12 ) ) return -1;
	const auto s = static_cast<unsigned char>( hc );
	const auto n = static_cast<unsigned short>( huffr->read( s ) );
	block[ 0 ] = static_cast<short>( devli( s, n ) );
	return 0;
}

int jpg_encode_dc_prg_fs( BitWriter* huffw, const HuffCodes* dctbl, const short* block )
{
	const auto s = static_cast<unsigned char>( jpg_vli_bitlen( block[ 0 ] ) );
	if ( ( s > 12 ) || ( dctbl->clen[ s ] == 0 ) ) return -1;
	const auto n = static_cast<unsigned short>( envli( s, block[ 0 ] ) );
	huffw->write_u16( dctbl->cval[ s ], dctbl->clen[ s ] );
	huffw->write_u16( n, s );
	return 0;
}

int jpg_decode_ac_prg_fs( BitReader* huffr, const HuffTree* actree, short* block, int* eobrun, int from, int to )
{
	int eob = to + 1;

	for ( int bpos = from; bpos <= to; ) {
		const int hc = jpg_next_huffcode( huffr, actree );
		if ( hc < 0 ) return -1;
		const int l = static_cast<int>( lbits_u8( static_cast<std::uint8_t>( hc ), 4 ) );
		const int r = static_cast<int>( rbits_u8( static_cast<std::uint8_t>( hc ), 4 ) );
		if ( ( l == 15 ) || ( r > 0 ) ) {
			if ( r > 11 ) return -1;
			auto z = static_cast<unsigned char>( l );
			const auto s = static_cast<unsigned char>( r );
			const auto n = static_cast<unsigned short>( huffr->read( s ) );
			if ( ( z + bpos ) > to )
				return -1;
			while ( z > 0 ) {
				block[ bpos++ ] = 0;
				z--;
			}
			block[ bpos++ ] = static_cast<short>( devli( s, n ) );
		}
		else {
			eob = bpos;
			const auto s = static_cast<unsigned char>( l );
			const auto n = static_cast<unsigned short>( huffr->read( s ) );
			( *eobrun ) = e_devli( s, n );
			break;
		}
	}

	return eob;
}

int jpg_encode_ac_prg_fs( BitWriter* huffw, const HuffCodes* actbl, const short* block, int* eobrun, int from, int to )
{
	unsigned char z = 0;
	for ( int bpos = from; bpos <= to; bpos++ ) {
		if ( block[ bpos ] != 0 ) {
			if ( jpg_encode_eobrun( huffw, actbl, eobrun ) < 0 ) return -1;
			while ( z >= 16 ) {
				if ( actbl->clen[ 0xF0 ] == 0 ) return -1;
				huffw->write_u16( actbl->cval[ 0xF0 ], actbl->clen[ 0xF0 ] );
				z -= 16;
			}
			const auto s = static_cast<unsigned char>( jpg_vli_bitlen( block[ bpos ] ) );
			if ( s > 11 ) return -1;
			const auto n = static_cast<unsigned short>( envli( s, block[ bpos ] ) );
			const int hc = ( z << 4 ) + s;
			if ( actbl->clen[ hc ] == 0 ) return -1;
			huffw->write_u16( actbl->cval[ hc ], actbl->clen[ hc ] );
			huffw->write_u16( n, s );
			z = 0;
		}
		else {
			z++;
		}
	}

	if ( z > 0 ) {
		if ( actbl->max_eobrun == 0 ) return -1;
		( *eobrun )++;
		if ( ( *eobrun ) == actbl->max_eobrun ) {
			if ( jpg_encode_eobrun( huffw, actbl, eobrun ) < 0 ) return -1;
		}
		return 1 + to - z;
	}
	return 1 + to;
}

int jpg_decode_dc_prg_sa( BitReader* huffr, short* block )
{
	block[ 0 ] = static_cast<short>( huffr->read( 1 ) );
	return 0;
}

int jpg_encode_dc_prg_sa( BitWriter* huffw, const short* block )
{
	huffw->write_u16( static_cast<std::uint16_t>( block[ 0 ] ), 1 );
	return 0;
}

int jpg_decode_ac_prg_sa( BitReader* huffr, const HuffTree* actree, short* block, int* eobrun, int from, int to, bool* trailing_zrl )
{
	int bpos = from;
	int eob = to;
	if ( trailing_zrl != nullptr ) {
		*trailing_zrl = false;
	}

	if ( ( *eobrun ) == 0 ) {
		while ( bpos <= to ) {
			const int hc = jpg_next_huffcode( huffr, actree );
			if ( hc < 0 ) return -1;
			const int l = static_cast<int>( lbits_u8( static_cast<std::uint8_t>( hc ), 4 ) );
			const int r = static_cast<int>( rbits_u8( static_cast<std::uint8_t>( hc ), 4 ) );
			if ( ( l == 15 ) || ( r > 0 ) ) {
				signed char z = static_cast<signed char>( l );
				const auto s = static_cast<unsigned char>( r );
				signed char v = 0;
				if ( s == 0 ) {
					v = 0;
					if ( trailing_zrl != nullptr ) *trailing_zrl = true;
				}
				else if ( s == 1 ) {
					const auto n = static_cast<unsigned short>( huffr->read( 1 ) );
					v = ( n == 0 ) ? -1 : 1;
					if ( trailing_zrl != nullptr ) *trailing_zrl = false;
				}
				else {
					return -1;
				}
				while ( true ) {
					if ( block[ bpos ] == 0 ) {
						if ( z > 0 ) z--;
						else {
							block[ bpos++ ] = v;
							break;
						}
					}
					else {
						const auto n = static_cast<short>( huffr->read( 1 ) );
						block[ bpos ] = ( block[ bpos ] > 0 ) ? n : static_cast<short>( -n );
					}
					if ( bpos++ >= to ) return -1;
				}
			}
			else {
				eob = bpos;
				const auto s = static_cast<unsigned char>( l );
				const auto n = static_cast<unsigned short>( huffr->read( s ) );
				( *eobrun ) = e_devli( s, n );
				break;
			}
		}
	}

	if ( ( *eobrun ) > 0 ) {
		for ( ; bpos <= to; bpos++ ) {
			if ( block[ bpos ] != 0 ) {
				const auto n = static_cast<short>( huffr->read( 1 ) );
				block[ bpos ] = ( block[ bpos ] > 0 ) ? n : static_cast<short>( -n );
			}
		}
	}

	return eob;
}

int jpg_encode_ac_prg_sa(
	BitWriter* huffw,
	std::vector<std::uint8_t>& storw,
	const HuffCodes* actbl,
	const short* block,
	int* eobrun,
	int from,
	int to )
{
	int eob = from;

	for ( int bpos = to; bpos >= from; bpos-- ) {
		if ( ( block[ bpos ] == 1 ) || ( block[ bpos ] == -1 ) ) {
			eob = bpos + 1;
			break;
		}
	}

	if ( ( eob > from ) && ( ( *eobrun ) > 0 ) ) {
		if ( jpg_encode_eobrun( huffw, actbl, eobrun ) < 0 ) return -1;
		jpg_encode_crbits( huffw, storw );
	}

	unsigned char z = 0;
	int bpos = from;
	for ( ; bpos < eob; bpos++ ) {
		if ( block[ bpos ] == 0 ) {
			z++;
			if ( z == 16 ) {
				if ( actbl->clen[ 0xF0 ] == 0 ) return -1;
				huffw->write_u16( actbl->cval[ 0xF0 ], actbl->clen[ 0xF0 ] );
				jpg_encode_crbits( huffw, storw );
				z = 0;
			}
		}
		else if ( ( block[ bpos ] == 1 ) || ( block[ bpos ] == -1 ) ) {
			const auto s = static_cast<unsigned char>( jpg_vli_bitlen( block[ bpos ] ) );
			const auto n = static_cast<unsigned short>( envli( s, block[ bpos ] ) );
			const int hc = ( z << 4 ) + s;
			if ( actbl->clen[ hc ] == 0 ) return -1;
			huffw->write_u16( actbl->cval[ hc ], actbl->clen[ hc ] );
			huffw->write_u16( n, s );
			jpg_encode_crbits( huffw, storw );
			z = 0;
		}
		else {
			storw.emplace_back( static_cast<std::uint8_t>( block[ bpos ] & 0x1 ) );
		}
	}

	for ( ; bpos <= to; bpos++ ) {
		if ( block[ bpos ] != 0 ) {
			storw.emplace_back( static_cast<std::uint8_t>( block[ bpos ] & 0x1 ) );
		}
	}

	if ( eob <= to ) {
		if ( actbl->max_eobrun == 0 ) return -1;
		( *eobrun )++;
		if ( ( *eobrun ) == actbl->max_eobrun ) {
			if ( jpg_encode_eobrun( huffw, actbl, eobrun ) < 0 ) return -1;
			jpg_encode_crbits( huffw, storw );
		}
	}

	return eob;
}

int jpg_decode_eobrun_sa( BitReader* huffr, short* block, int from, int to )
{
	for ( int bpos = from; bpos <= to; bpos++ ) {
		if ( block[ bpos ] != 0 ) {
			const auto n = static_cast<short>( huffr->read( 1 ) );
			block[ bpos ] = ( block[ bpos ] > 0 ) ? n : static_cast<short>( -n );
		}
	}
	return 0;
}

int jpg_next_mcupos( const CodecContext& ctx, int* mcu, int* cmp, int* csc, int* sub, int* dpos, int* rstw )
{
	int sta = 0;

	if ( ( ++( *sub ) ) >= ctx.cmpnfo[ *cmp ].mbs ) {
		( *sub ) = 0;

		if ( ( ++( *csc ) ) >= ctx.cs_cmpc ) {
			( *csc ) = 0;
			( *cmp ) = ctx.cs_cmp[ 0 ];
			( *mcu )++;
			if ( ( *mcu ) >= ctx.mcuc ) sta = 2;
			else if ( ctx.rsti > 0 ) {
				if ( --( *rstw ) == 0 ) sta = 1;
			}
		}
		else {
			( *cmp ) = ctx.cs_cmp[ *csc ];
		}
	}

	const auto& ci = ctx.cmpnfo[ *cmp ];
	if ( ci.sfh > 1 ) {
		( *dpos )  = ( ( *mcu ) / ctx.mcuh ) * ci.sfh + ( ( *sub ) / ci.sfv );
		( *dpos ) *= ci.bch;
		( *dpos ) += ( ( *mcu ) % ctx.mcuh ) * ci.sfv + ( ( *sub ) % ci.sfv );
	}
	else if ( ci.sfv > 1 ) {
		( *dpos ) = ( ( *mcu ) * ci.mbs ) + ( *sub );
	}
	else {
		( *dpos ) = ( *mcu );
	}

	return sta;
}

int jpg_next_mcuposn( const CodecContext& ctx, int* cmp, int* dpos, int* rstw )
{
	( *dpos )++;
	const auto& ci = ctx.cmpnfo[ *cmp ];

	if ( ci.bch != ci.nch ) {
		if ( ( *dpos ) % ci.bch == ci.nch )
			( *dpos ) += ( ci.bch - ci.nch );
	}

	if ( ci.bcv != ci.ncv ) {
		if ( ( *dpos ) / ci.bch == ci.ncv )
			( *dpos ) = ci.bc;
	}

	if ( ( *dpos ) >= ci.bc ) return 2;
	if ( ( ctx.rsti > 0 ) && ( --( *rstw ) == 0 ) ) return 1;

	return 0;
}

int jpg_skip_eobrun( const CodecContext& ctx, int* cmp, int* dpos, int* rstw, int* eobrun )
{
	const int count = *eobrun;
	*eobrun = 0;
	for ( int i = 0; i < count; i++ ) {
		if ( jpg_next_mcuposn( ctx, cmp, dpos, rstw ) != 0 ) {
			return -1;
		}
	}
	return 0;
}

} // namespace


/* -----------------------------------------------
	Read in header & image data
	----------------------------------------------- */
bool read_jpeg( CodecContext& ctx )
{
	unsigned int  ssize = 1024;
	unsigned char type  = 0x00;
	unsigned int  len   = 0;
	unsigned int  crst  = 0;
	unsigned int  cpos  = 0;
	unsigned char tmp   = 0;

	ctx.scnc = 0;
	ctx.scnp.clear();
	ctx.scnp.push_back( 0 );
	ctx.rstp.clear();
	bool rst_order_err = false;

	MemoryWriter hdrw;
	MemoryWriter huffw;
	huffw.reserve( ctx.str_in->get_size() );
	ctx.hdrdata.clear();
	ctx.huffdata.clear();
	ctx.grbgdata.clear();

	std::vector<unsigned char> segment( ssize, 0 );
	std::vector<unsigned int> scan_rst;

	while ( true ) {
		if ( type == 0xDA ) {
			cpos = 0;
			crst = 0;
			const std::size_t scan_start = huffw.num_bytes_written();
			scan_rst.clear();
			while ( true ) {
				std::size_t avail = 0;
				const std::uint8_t* span = ctx.str_in->peek_contiguous( avail );
				if ( ( avail > 0 ) && ( span != nullptr ) ) {
					const auto* ff_pos = static_cast<const std::uint8_t*>( std::memchr( span, 0xFF, avail ) );
					const std::size_t run = ( ff_pos != nullptr )
						? static_cast<std::size_t>( ff_pos - span )
						: avail;
					if ( run > 0 ) {
						crst = 0;
						huffw.write( span, run );
						ctx.str_in->skip( run );
						if ( huffw.num_bytes_written() > ( 512u << 20 ) ) {
							ctx.errormessage = "coded image data exceeds maximum supported size";
							ctx.errorlevel = 2;
							return false;
						}
					}
					if ( ff_pos == nullptr ) {
						break;
					}
				}

				if ( !ctx.str_in->read_byte( &tmp ) )
					break;

				if ( tmp != 0xFF ) {
					crst = 0;
					while ( tmp != 0xFF ) {
						huffw.write_byte( tmp );
						if ( huffw.num_bytes_written() > ( 512u << 20 ) ) {
							ctx.errormessage = "coded image data exceeds maximum supported size";
							ctx.errorlevel = 2;
							return false;
						}
						if ( !ctx.str_in->read_byte( &tmp ) )
							break;
					}
				}

				if ( tmp == 0xFF ) {
					if ( !ctx.str_in->read_byte( &tmp ) )
						break;
					if ( tmp == 0x00 ) {
						crst = 0;
						huffw.write_byte( 0xFF );
						if ( huffw.num_bytes_written() > ( 512u << 20 ) ) {
							ctx.errormessage = "coded image data exceeds maximum supported size";
							ctx.errorlevel = 2;
							return false;
						}
					}
					else if ( tmp == 0xD0 + ( cpos % 8 ) ) {
						const std::size_t cur_written = huffw.num_bytes_written();
						scan_rst.push_back( ( cur_written > scan_start )
							? static_cast<unsigned int>( cur_written - 1 )
							: 0xFFFFFFFFu );
						cpos++;
						crst++;
					}
					else {
						const unsigned int trailing_rst = crst;
						if ( trailing_rst <= scan_rst.size() ) {
							scan_rst.resize( scan_rst.size() - trailing_rst );
						}
						else {
							scan_rst.clear();
						}
						for ( std::size_t k = 0; k < scan_rst.size(); k++ ) {
							if ( ( scan_rst[ k ] == 0xFFFFFFFFu ) ||
								 ( ( k > 0 ) && ( scan_rst[ k ] <= scan_rst[ k - 1 ] ) ) ) {
								rst_order_err = true;
							}
						}
						ctx.rstp.insert( ctx.rstp.end(), scan_rst.begin(), scan_rst.end() );
						ctx.scnp.push_back( static_cast<unsigned int>( huffw.num_bytes_written() ) );

						if ( crst > 0 ) {
							ctx.has_rst_err = true;
						}
						if ( ctx.has_rst_err ) {
							if ( ctx.rst_err.size() < static_cast<std::size_t>( ctx.scnc + 1 ) ) {
								ctx.rst_err.resize( static_cast<std::size_t>( ctx.scnc + 1 ), 0 );
							}
							if ( crst > 255 ) {
								ctx.set_errormessage( "Severe false use of RST markers (%i)", static_cast<int>( crst ) );
								ctx.errorlevel = 1;
								crst = 255;
							}
							ctx.rst_err[ ctx.scnc ] = static_cast<unsigned char>( crst );
						}
						ctx.scnc++;
						segment[ 0 ] = 0xFF;
						segment[ 1 ] = tmp;
						break;
					}
				}
				else {
					break;
				}
			}
		}
		else {
			if ( ctx.str_in->read( segment.data(), 2 ) != 2 ) break;
			if ( segment[ 0 ] != 0xFF ) {
				ctx.set_errormessage( "size mismatch in marker segment FF %2X", type );
				ctx.errorlevel = 2;
				if ( type == 0xFE ) {
					if ( ctx.str_in->read( segment.data(), 2 ) != 2 ) break;
					if ( segment[ 0 ] == 0xFF ) ctx.errorlevel = 1;
				}
				if ( ctx.errorlevel == 2 ) {
					return false;
				}
			}
		}

		type = segment[ 1 ];

		if ( type == 0xD9 ) {
			ctx.hdrdata  = hdrw.take_data();
			ctx.huffdata = huffw.take_data();
			break;
		}
		if ( ( type == 0xFF ) || ( type == 0x00 ) || ( type == 0x01 ) ) {
			ctx.errormessage = "unsupported fill byte or standalone marker in header";
			ctx.errorlevel = 2;
			return false;
		}

		if ( ctx.str_in->read( segment.data() + 2, 2 ) != 2 ) break;
		len = static_cast<unsigned int>( 2 + b_short( segment[ 2 ], segment[ 3 ] ) );
		if ( len < 4 ) break;

		if ( ssize < len ) {
			segment.resize( len );
			ssize = len;
		}

		if ( ctx.str_in->read( segment.data() + 4, len - 4 ) != static_cast<std::size_t>( len - 4 ) ) break;
		hdrw.write( segment.data(), len );
	}

	if ( ctx.hdrdata.empty() || ctx.huffdata.empty() ) {
		ctx.errormessage = "unexpected end of data encountered";
		ctx.errorlevel = 2;
		return false;
	}

	if ( rst_order_err ) {
		ctx.errormessage = "false use of rst markers";
		ctx.errorlevel = 1;
	}

	if ( ctx.str_in->read_byte( &tmp ) ) {
		MemoryWriter grbgw;
		grbgw.write_byte( tmp );
		while ( true ) {
			const auto nread = ctx.str_in->read( segment.data(), ssize );
			if ( nread == 0 ) break;
			grbgw.write( segment.data(), nread );
		}
		ctx.grbgdata = grbgw.take_data();
	}

	ctx.jpgfilesize = static_cast<std::int64_t>( ctx.str_in->get_size() );

	if ( !jpg_setup_imginfo( ctx ) ) {
		return false;
	}

	return true;
}


/* -----------------------------------------------
	Merges header & image data to jpeg
	----------------------------------------------- */
bool merge_jpeg( CodecContext& ctx )
{
	constexpr std::array<std::uint8_t, 2> SOI{ 0xFF, 0xD8 };
	constexpr std::array<std::uint8_t, 2> EOI{ 0xFF, 0xD9 };
	constexpr unsigned char mrk = 0xFF;
	constexpr unsigned char stv = 0x00;

	unsigned char type = 0x00;
	unsigned int  len  = 0;
	unsigned int  hpos = 0;
	unsigned int  rpos = 0;
	unsigned int  cpos = 0;
	unsigned int  scan = 1;

	std::array<std::uint8_t, 65536> out_buf{};
	std::size_t out_pos = 0;
	auto flush_out = [&]() {
		if ( out_pos > 0 ) {
			ctx.str_out->write( out_buf.data(), out_pos );
			out_pos = 0;
		}
	};
	auto emit_byte = [&]( std::uint8_t b ) {
		if ( out_pos == out_buf.size() ) {
			flush_out();
		}
		out_buf[ out_pos++ ] = b;
	};

	ctx.str_out->write( SOI );

	while ( true ) {
		const unsigned int tmp = hpos;

		for ( type = 0x00; type != 0xDA; ) {
			if ( hpos >= ctx.hdrdata.size() ) break;
			if ( !ctx.read_header_segment( hpos, type, len ) ) return false;
			hpos += len;
		}

		ctx.str_out->write( ctx.hdrdata.data() + tmp, hpos - tmp );

		if ( type != 0xDA ) break;

		cpos = 0;

		if ( ctx.scnp.empty() ||
			 ( static_cast<int>( scan ) > ctx.scnc ) ||
			 ( scan >= ctx.scnp.size() ) ||
			 ( ctx.scnp[ scan ] < ctx.scnp[ scan - 1 ] ) ||
			 ( ctx.scnp[ scan ] > ctx.huffdata.size() ) ) {
			ctx.errormessage = "corrupted scan position data";
			ctx.errorlevel = 2;
			return false;
		}

		for ( unsigned int ipos = ctx.scnp[ scan - 1 ]; ipos < ctx.scnp[ scan ]; ipos++ ) {
			const unsigned char b = ctx.huffdata[ ipos ];
			emit_byte( b );
			if ( b == 0xFF )
				emit_byte( stv );
			if ( !ctx.rstp.empty() && ( static_cast<int>( rpos ) < ctx.rstc ) ) {
				if ( ipos == ctx.rstp[ rpos ] ) {
					const auto rst = static_cast<unsigned char>( 0xD0 + ( cpos & 7u ) );
					emit_byte( mrk );
					emit_byte( rst );
					rpos++;
					cpos++;
				}
			}
		}

		if ( ctx.has_rst_err && ( ( scan - 1 ) < ctx.rst_err.size() ) ) {
			unsigned int rem_rst = ctx.rst_err[ scan - 1 ];
			while ( rem_rst > 0 ) {
				const auto rst = static_cast<unsigned char>( 0xD0 + ( cpos & 7u ) );
				emit_byte( mrk );
				emit_byte( rst );
				cpos++;
				rem_rst--;
			}
		}

		flush_out();
		scan++;
	}

	ctx.str_out->write( EOI );

	if ( !ctx.grbgdata.empty() )
		ctx.str_out->write( ctx.grbgdata );

	if ( ctx.str_out->error() ) {
		ctx.errormessage = "write error, possibly drive is full";
		ctx.errorlevel = 2;
		return false;
	}

	ctx.jpgfilesize = static_cast<std::int64_t>( ctx.str_out->num_bytes_written() );
	return true;
}


/* -----------------------------------------------
	JPEG decoding routine
	----------------------------------------------- */
bool decode_jpeg( CodecContext& ctx )
{
	unsigned char type = 0x00;
	unsigned int  len  = 0;
	unsigned int  hpos = 0;

	int   lastdc[ 4 ];
	short block[ 64 ];

	std::array<std::array<int, 64>, 4> prog_al;
	for ( auto& row : prog_al ) {
		row.fill( -1 );
	}
	std::vector<unsigned int> decoded_rstp;

	BitReader huffr_obj( ctx.huffdata.data(), static_cast<int>( ctx.huffdata.size() ) );
	BitReader* huffr = &huffr_obj;

	ctx.scnc = 0;
	ctx.rsti = 0;
	std::memset( ctx.htset, 0, sizeof( ctx.htset ) );


	auto checked_short = []( int v, short& out ) noexcept -> bool {
		if ( ( v < -32768 ) || ( v > 32767 ) ) {
			return false;
		}
		out = static_cast<short>( v );
		return true;
	};

	while ( true ) {
		for ( type = 0x00; type != 0xDA; ) {
			if ( hpos >= ctx.hdrdata.size() ) break;
			if ( !ctx.read_header_segment( hpos, type, len ) ) return false;
			if ( ( type == 0xC4 ) || ( type == 0xDA ) || ( type == 0xDD ) ) {
				if ( !jpg_parse_jfif( ctx, type, len, &ctx.hdrdata[ hpos ] ) ) {
					return false;
				}
			}
			hpos += len;
		}

		if ( type != 0xDA ) break;

		if ( !check_scan_huff_tables( ctx ) )
			return false;
		if ( !validate_scan_progression( ctx, prog_al ) )
			return false;

		int cmp  = ctx.cs_cmp[ 0 ];
		int csc  = 0;
		int mcu  = 0;
		int sub  = 0;
		int dpos = 0;

		const unsigned int scan_start_byte = ( static_cast<std::size_t>( ctx.scnc ) < ctx.scnp.size() )
			? ctx.scnp[ ctx.scnc ]
			: 0u;
		const std::size_t scan_rst_base = decoded_rstp.size();

		while ( true ) {
			lastdc[ 0 ] = 0;
			lastdc[ 1 ] = 0;
			lastdc[ 2 ] = 0;
			lastdc[ 3 ] = 0;

			int eob     = 0;
			int sta     = 0;
			int eobrun  = 0;
			int peobrun = 0;
			int rstw    = ctx.rsti;

			if ( ctx.cs_cmpc > 1 ) {
				if ( ctx.jpegtype == 1 ) {
					while ( sta == 0 ) {
						eob = jpg_decode_block_seq( huffr,
							&ctx.htrees[ 0 ][ ctx.cmpnfo[ cmp ].huffdc ],
							&ctx.htrees[ 1 ][ ctx.cmpnfo[ cmp ].huffac ],
							block );

						if ( eob < 0 ) {
							sta = -1;
							break;
						}

						if ( ( eob > 1 ) && ( block[ eob - 1 ] == 0 ) ) {
							ctx.errormessage = "reconstruction of inefficient coding not supported";
							ctx.errorlevel = 1;
						}

						if ( !checked_short( static_cast<int>( block[ 0 ] ) + lastdc[ cmp ], block[ 0 ] ) ) {
							sta = -1;
							break;
						}
						lastdc[ cmp ] = block[ 0 ];

						for ( int bpos = 0; bpos < eob; bpos++ )
							ctx.colldata[ cmp ][ bpos ][ dpos ] = block[ bpos ];

						sta = jpg_next_mcupos( ctx, &mcu, &cmp, &csc, &sub, &dpos, &rstw );
						if ( ( sta == 0 ) && ( ( ctx.err_tol <= 1 ) ? ( huffr->peof() > 0 ) : ( huffr->peof() > 65536 ) ) ) {
							ctx.errormessage = "coded image data truncated / too short";
							ctx.errorlevel = 1;
							return true;
						}
					}
				}
				else if ( ctx.cs_sah == 0 ) {
					while ( sta == 0 ) {
						sta = jpg_decode_dc_prg_fs( huffr,
							&ctx.htrees[ 0 ][ ctx.cmpnfo[ cmp ].huffdc ],
							block );
						if ( sta == -1 ) break;

						short dc_unscaled = 0;
						if ( !checked_short( static_cast<int>( block[ 0 ] ) + lastdc[ cmp ], dc_unscaled ) ||
							 !checked_short( static_cast<int>( dc_unscaled ) * ( 1 << ctx.cs_sal ), ctx.colldata[ cmp ][ 0 ][ dpos ] ) ) {
							sta = -1;
							break;
						}
						lastdc[ cmp ] = dc_unscaled;

						sta = jpg_next_mcupos( ctx, &mcu, &cmp, &csc, &sub, &dpos, &rstw );
						if ( ( sta == 0 ) && ( ( ctx.err_tol <= 1 ) ? ( huffr->peof() > 0 ) : ( huffr->peof() > 65536 ) ) ) {
							ctx.errormessage = "coded image data truncated / too short";
							ctx.errorlevel = 1;
							return true;
						}
					}
				}
				else {
					while ( sta == 0 ) {
						sta = jpg_decode_dc_prg_sa( huffr, block );
						if ( sta == -1 ) break;

						if ( !checked_short( static_cast<int>( ctx.colldata[ cmp ][ 0 ][ dpos ] ) + static_cast<int>( block[ 0 ] ) * ( 1 << ctx.cs_sal ),
											 ctx.colldata[ cmp ][ 0 ][ dpos ] ) ) {
							sta = -1;
							break;
						}

						sta = jpg_next_mcupos( ctx, &mcu, &cmp, &csc, &sub, &dpos, &rstw );
						if ( ( sta == 0 ) && ( ( ctx.err_tol <= 1 ) ? ( huffr->peof() > 0 ) : ( huffr->peof() > 65536 ) ) ) {
							ctx.errormessage = "coded image data truncated / too short";
							ctx.errorlevel = 1;
							return true;
						}
					}
				}
			}
			else {
				if ( ctx.jpegtype == 1 ) {
					while ( sta == 0 ) {
						eob = jpg_decode_block_seq( huffr,
							&ctx.htrees[ 0 ][ ctx.cmpnfo[ cmp ].huffdc ],
							&ctx.htrees[ 1 ][ ctx.cmpnfo[ cmp ].huffac ],
							block );

						if ( eob < 0 ) {
							sta = -1;
							break;
						}

						if ( ( eob > 1 ) && ( block[ eob - 1 ] == 0 ) ) {
							ctx.errormessage = "reconstruction of inefficient coding not supported";
							ctx.errorlevel = 1;
						}

						if ( !checked_short( static_cast<int>( block[ 0 ] ) + lastdc[ cmp ], block[ 0 ] ) ) {
							sta = -1;
							break;
						}
						lastdc[ cmp ] = block[ 0 ];

						for ( int bpos = 0; bpos < eob; bpos++ )
							ctx.colldata[ cmp ][ bpos ][ dpos ] = block[ bpos ];

						sta = jpg_next_mcuposn( ctx, &cmp, &dpos, &rstw );
						if ( ( sta == 0 ) && ( ( ctx.err_tol <= 1 ) ? ( huffr->peof() > 0 ) : ( huffr->peof() > 65536 ) ) ) {
							ctx.errormessage = "coded image data truncated / too short";
							ctx.errorlevel = 1;
							return true;
						}
					}
				}
				else if ( ctx.cs_to == 0 ) {
					if ( ctx.cs_sah == 0 ) {
						while ( sta == 0 ) {
							sta = jpg_decode_dc_prg_fs( huffr,
								&ctx.htrees[ 0 ][ ctx.cmpnfo[ cmp ].huffdc ],
								block );
							if ( sta == -1 ) break;

							short dc_unscaled = 0;
							if ( !checked_short( static_cast<int>( block[ 0 ] ) + lastdc[ cmp ], dc_unscaled ) ||
								 !checked_short( static_cast<int>( dc_unscaled ) * ( 1 << ctx.cs_sal ), ctx.colldata[ cmp ][ 0 ][ dpos ] ) ) {
								sta = -1;
								break;
							}
							lastdc[ cmp ] = dc_unscaled;

							sta = jpg_next_mcuposn( ctx, &cmp, &dpos, &rstw );
							if ( ( sta == 0 ) && ( ( ctx.err_tol <= 1 ) ? ( huffr->peof() > 0 ) : ( huffr->peof() > 65536 ) ) ) {
								ctx.errormessage = "coded image data truncated / too short";
								ctx.errorlevel = 1;
								return true;
							}
						}
					}
					else {
						while ( sta == 0 ) {
							sta = jpg_decode_dc_prg_sa( huffr, block );
							if ( sta == -1 ) break;

							if ( !checked_short( static_cast<int>( ctx.colldata[ cmp ][ 0 ][ dpos ] ) + static_cast<int>( block[ 0 ] ) * ( 1 << ctx.cs_sal ),
												 ctx.colldata[ cmp ][ 0 ][ dpos ] ) ) {
								sta = -1;
								break;
							}

							sta = jpg_next_mcuposn( ctx, &cmp, &dpos, &rstw );
							if ( ( sta == 0 ) && ( ( ctx.err_tol <= 1 ) ? ( huffr->peof() > 0 ) : ( huffr->peof() > 65536 ) ) ) {
								ctx.errormessage = "coded image data truncated / too short";
								ctx.errorlevel = 1;
								return true;
							}
						}
					}
				}
				else {
					if ( ctx.cs_sah == 0 ) {
						while ( sta == 0 ) {
							if ( eobrun == 0 ) {
								eob = jpg_decode_ac_prg_fs( huffr,
									&ctx.htrees[ 1 ][ ctx.cmpnfo[ cmp ].huffac ],
									block, &eobrun, ctx.cs_from, ctx.cs_to );

								if ( eob < 0 ) {
									sta = -1;
									break;
								}

								if ( ( eob > ctx.cs_from ) && ( block[ eob - 1 ] == 0 ) ) {
									ctx.errormessage = "reconstruction of inefficient coding not supported";
									ctx.errorlevel = 1;
								}

								if ( eobrun > 0 ) {
									if ( ( eob == ctx.cs_from ) && ( peobrun > 0 ) &&
										 ( peobrun < ctx.hcodes[ 1 ][ ctx.cmpnfo[ cmp ].huffac ].max_eobrun ) ) {
										ctx.errormessage = "reconstruction of inefficient coding not supported";
										ctx.errorlevel = 1;
									}
									peobrun = eobrun;
									eobrun--;
								} else peobrun = 0;

								for ( int bpos = ctx.cs_from; bpos < eob; bpos++ ) {
									if ( !checked_short( static_cast<int>( block[ bpos ] ) * ( 1 << ctx.cs_sal ), ctx.colldata[ cmp ][ bpos ][ dpos ] ) ) {
										sta = -1;
										break;
									}
								}
								if ( sta == -1 ) break;
							} else eobrun--;

							sta = jpg_skip_eobrun( ctx, &cmp, &dpos, &rstw, &eobrun );

							if ( sta == 0 )
								sta = jpg_next_mcuposn( ctx, &cmp, &dpos, &rstw );
							if ( ( sta == 0 ) && ( ( ctx.err_tol <= 1 ) ? ( huffr->peof() > 0 ) : ( huffr->peof() > 65536 ) ) ) {
								ctx.errormessage = "coded image data truncated / too short";
								ctx.errorlevel = 1;
								return true;
							}
						}
					}
					else {
						while ( sta == 0 ) {
							for ( int bpos = ctx.cs_from; bpos <= ctx.cs_to; bpos++ )
								block[ bpos ] = ctx.colldata[ cmp ][ bpos ][ dpos ];

							if ( eobrun == 0 ) {
								bool trailing_zrl = false;
								eob = jpg_decode_ac_prg_sa( huffr,
									&ctx.htrees[ 1 ][ ctx.cmpnfo[ cmp ].huffac ],
									block, &eobrun, ctx.cs_from, ctx.cs_to, &trailing_zrl );

								if ( eob < 0 ) {
									sta = -1;
									break;
								}

								if ( trailing_zrl ) {
									ctx.errormessage = "reconstruction of inefficient coding not supported";
									ctx.errorlevel = 1;
								}

								if ( eobrun > 0 ) {
									if ( ( eob == ctx.cs_from ) && ( peobrun > 0 ) &&
										 ( peobrun < ctx.hcodes[ 1 ][ ctx.cmpnfo[ cmp ].huffac ].max_eobrun ) ) {
										ctx.errormessage = "reconstruction of inefficient coding not supported";
										ctx.errorlevel = 1;
									}
									peobrun = eobrun;
									eobrun--;
								} else peobrun = 0;
							}
							else {
								eob = jpg_decode_eobrun_sa( huffr, block, ctx.cs_from, ctx.cs_to );
								eobrun--;
							}

							for ( int bpos = ctx.cs_from; bpos <= ctx.cs_to; bpos++ ) {
								if ( !checked_short( static_cast<int>( ctx.colldata[ cmp ][ bpos ][ dpos ] ) + static_cast<int>( block[ bpos ] ) * ( 1 << ctx.cs_sal ),
													 ctx.colldata[ cmp ][ bpos ][ dpos ] ) ) {
									sta = -1;
									break;
								}
							}
							if ( sta == -1 ) break;

							if ( eob < 0 ) sta = -1;
							else sta = jpg_next_mcuposn( ctx, &cmp, &dpos, &rstw );
							if ( ( sta != 0 ) && ( eobrun > 0 ) ) {
								sta = -1;
								break;
							}
							if ( ( sta == 0 ) && ( ( ctx.err_tol <= 1 ) ? ( huffr->peof() > 0 ) : ( huffr->peof() > 65536 ) ) ) {
								ctx.errormessage = "coded image data truncated / too short";
								ctx.errorlevel = 1;
								return true;
							}
						}
					}
				}
			}

			if ( ( huffr->getbitp() != 8 ) && !huffr->eof() ) {
				const int p = static_cast<int>( huffr->unpad( 0 ) );
				if ( ( p == 2 ) || ( ( ctx.padbit != -1 ) && ( ctx.padbit != p ) ) ) {
					ctx.errormessage = "inconsistent use of padbits";
					ctx.padbit = 1;
					ctx.errorlevel = 1;
				}
				else {
					ctx.padbit = p;
				}
			}

			if ( sta == -1 ) {
				ctx.set_errormessage( "decode error in scan%i / mcu%i",
					ctx.scnc, ( ctx.cs_cmpc > 1 ) ? mcu : dpos );
				ctx.errorlevel = 2;
				return false;
			}
			else if ( sta == 1 ) {
				if ( ctx.rsti > 0 ) {
					const int pos = huffr->getpos();
					if ( pos <= static_cast<int>( scan_start_byte ) ) {
						ctx.errormessage = "false use of rst markers";
						ctx.errorlevel = 1;
					}
					else {
						const auto rbyte = static_cast<unsigned int>( pos - 1 );
						if ( ( decoded_rstp.size() > scan_rst_base ) && ( decoded_rstp.back() >= rbyte ) ) {
							ctx.errormessage = "false use of rst markers";
							ctx.errorlevel = 1;
						}
						decoded_rstp.push_back( rbyte );
					}
				}
			}
			else if ( sta == 2 ) {
				if ( ( static_cast<std::size_t>( ctx.scnc + 1 ) >= ctx.scnp.size() ) ||
					 ( static_cast<unsigned int>( huffr->getpos() ) != ctx.scnp[ ctx.scnc + 1 ] ) ) {
					ctx.errormessage = "scan data boundary mismatch";
					ctx.errorlevel = 1;
					if ( static_cast<std::size_t>( ctx.scnc + 1 ) < ctx.scnp.size() ) {
						huffr->setpos( static_cast<int>( ctx.scnp[ ctx.scnc + 1 ] ), 8 );
					}
				}
				ctx.scnc++;
				break;
			}
		}
	}

	if ( decoded_rstp != ctx.rstp ) {
		ctx.errormessage = "false use of rst markers";
		ctx.errorlevel = 1;
	}

	if ( huffr->peof() > 0 ) {
		ctx.errormessage = "coded image data truncated / too short";
		ctx.errorlevel = 1;
	}

	if ( !huffr->eof() ) {
		ctx.errormessage = "surplus data found after coded image data";
		ctx.errorlevel = 1;
	}

	return true;
}


/* -----------------------------------------------
	JPEG encoding routine
	----------------------------------------------- */
bool recode_jpeg( CodecContext& ctx )
{
	unsigned char type = 0x00;
	unsigned int  len  = 0;
	unsigned int  hpos = 0;

	int   lastdc[ 4 ];
	short block[ 64 ];

	const std::uint8_t effective_padbit = ( ctx.padbit == 0 ) ? 0u : 1u;
	BitWriter huffw_obj( effective_padbit );
	huffw_obj.reserve( static_cast<std::size_t>( std::max( 1, ctx.mcuc ) ) * 32 );
	BitWriter* huffw = &huffw_obj;
	std::vector<std::uint8_t> storw;

	ctx.scnc = 0;
	ctx.rstc = 0;
	ctx.rsti = 0;
	std::memset( ctx.htset, 0, sizeof( ctx.htset ) );
	ctx.scnp.clear();
	ctx.rstp.clear();


	while ( true ) {
		for ( type = 0x00; type != 0xDA; ) {
			if ( hpos >= ctx.hdrdata.size() ) break;
			if ( !ctx.read_header_segment( hpos, type, len ) ) return false;
			if ( ( type == 0xC4 ) || ( type == 0xDA ) || ( type == 0xDD ) ) {
				if ( !jpg_parse_jfif( ctx, type, len, &ctx.hdrdata[ hpos ] ) ) {
					return false;
				}
			}
			hpos += len;
		}

		if ( type != 0xDA ) break;

		if ( !check_scan_huff_tables( ctx ) )
			return false;
		if ( !validate_scan_layout( ctx ) )
			return false;

		ctx.scnp.resize( static_cast<std::size_t>( ctx.scnc + 2 ), 0 );

		if ( ctx.rsti > 0 ) {
			const int tmp = ctx.rstc + ( ( ctx.cs_cmpc > 1 )
				? ( ctx.mcuc / ctx.rsti )
				: ( ctx.cmpnfo[ ctx.cs_cmp[ 0 ] ].bc / ctx.rsti ) );
			if ( ctx.rstp.size() < static_cast<std::size_t>( tmp + 1 ) )
				ctx.rstp.resize( static_cast<std::size_t>( tmp + 1 ), 0 );
		}

		int cmp  = ctx.cs_cmp[ 0 ];
		int csc  = 0;
		int mcu  = 0;
		int sub  = 0;
		int dpos = 0;

		ctx.scnp[ ctx.scnc ] = static_cast<unsigned int>( huffw->num_bytes_written() );

		while ( true ) {
			lastdc[ 0 ] = 0;
			lastdc[ 1 ] = 0;
			lastdc[ 2 ] = 0;
			lastdc[ 3 ] = 0;

			int sta    = 0;
			int eob    = 0;
			int eobrun = 0;
			int rstw   = ctx.rsti;

			if ( ctx.cs_cmpc > 1 ) {
				if ( ctx.jpegtype == 1 ) {
					while ( sta == 0 ) {
						for ( int bpos = 0; bpos < 64; bpos++ )
							block[ bpos ] = ctx.colldata[ cmp ][ bpos ][ dpos ];

						block[ 0 ] = static_cast<short>( block[ 0 ] - lastdc[ cmp ] );
						lastdc[ cmp ] = ctx.colldata[ cmp ][ 0 ][ dpos ];

						eob = jpg_encode_block_seq( huffw,
							&ctx.hcodes[ 0 ][ ctx.cmpnfo[ cmp ].huffdc ],
							&ctx.hcodes[ 1 ][ ctx.cmpnfo[ cmp ].huffac ],
							block );

						if ( eob < 0 ) sta = -1;
						else sta = jpg_next_mcupos( ctx, &mcu, &cmp, &csc, &sub, &dpos, &rstw );
					}
				}
				else if ( ctx.cs_sah == 0 ) {
					while ( sta == 0 ) {
						const int tmp = ctx.colldata[ cmp ][ 0 ][ dpos ] >> ctx.cs_sal;
						block[ 0 ] = static_cast<short>( tmp - lastdc[ cmp ] );
						lastdc[ cmp ] = tmp;

						sta = jpg_encode_dc_prg_fs( huffw,
							&ctx.hcodes[ 0 ][ ctx.cmpnfo[ cmp ].huffdc ],
							block );

						if ( sta != -1 )
							sta = jpg_next_mcupos( ctx, &mcu, &cmp, &csc, &sub, &dpos, &rstw );
					}
				}
				else {
					while ( sta == 0 ) {
						block[ 0 ] = static_cast<short>( bit_n( ctx.colldata[ cmp ][ 0 ][ dpos ], ctx.cs_sal ) );
						sta = jpg_encode_dc_prg_sa( huffw, block );

						if ( sta != -1 )
							sta = jpg_next_mcupos( ctx, &mcu, &cmp, &csc, &sub, &dpos, &rstw );
					}
				}
			}
			else {
				if ( ctx.jpegtype == 1 ) {
					while ( sta == 0 ) {
						for ( int bpos = 0; bpos < 64; bpos++ )
							block[ bpos ] = ctx.colldata[ cmp ][ bpos ][ dpos ];

						block[ 0 ] = static_cast<short>( block[ 0 ] - lastdc[ cmp ] );
						lastdc[ cmp ] = ctx.colldata[ cmp ][ 0 ][ dpos ];

						eob = jpg_encode_block_seq( huffw,
							&ctx.hcodes[ 0 ][ ctx.cmpnfo[ cmp ].huffdc ],
							&ctx.hcodes[ 1 ][ ctx.cmpnfo[ cmp ].huffac ],
							block );

						if ( eob < 0 ) sta = -1;
						else sta = jpg_next_mcuposn( ctx, &cmp, &dpos, &rstw );
					}
				}
				else if ( ctx.cs_to == 0 ) {
					if ( ctx.cs_sah == 0 ) {
						while ( sta == 0 ) {
							const int tmp = ctx.colldata[ cmp ][ 0 ][ dpos ] >> ctx.cs_sal;
							block[ 0 ] = static_cast<short>( tmp - lastdc[ cmp ] );
							lastdc[ cmp ] = tmp;

							sta = jpg_encode_dc_prg_fs( huffw,
								&ctx.hcodes[ 0 ][ ctx.cmpnfo[ cmp ].huffdc ],
								block );

							if ( sta != -1 )
								sta = jpg_next_mcuposn( ctx, &cmp, &dpos, &rstw );
						}
					}
					else {
						while ( sta == 0 ) {
							block[ 0 ] = static_cast<short>( bit_n( ctx.colldata[ cmp ][ 0 ][ dpos ], ctx.cs_sal ) );
							sta = jpg_encode_dc_prg_sa( huffw, block );

							if ( sta != -1 )
								sta = jpg_next_mcuposn( ctx, &cmp, &dpos, &rstw );
						}
					}
				}
				else {
					if ( ctx.cs_sah == 0 ) {
						while ( sta == 0 ) {
							for ( int bpos = ctx.cs_from; bpos <= ctx.cs_to; bpos++ )
								block[ bpos ] = static_cast<short>( fdiv2( ctx.colldata[ cmp ][ bpos ][ dpos ], ctx.cs_sal ) );

							eob = jpg_encode_ac_prg_fs( huffw,
								&ctx.hcodes[ 1 ][ ctx.cmpnfo[ cmp ].huffac ],
								block, &eobrun, ctx.cs_from, ctx.cs_to );

							if ( eob < 0 ) sta = -1;
							else sta = jpg_next_mcuposn( ctx, &cmp, &dpos, &rstw );
						}

						if ( ( sta != -1 ) && ( jpg_encode_eobrun( huffw, &ctx.hcodes[ 1 ][ ctx.cmpnfo[ cmp ].huffac ], &eobrun ) < 0 ) )
							sta = -1;
					}
					else {
						while ( sta == 0 ) {
							for ( int bpos = ctx.cs_from; bpos <= ctx.cs_to; bpos++ )
								block[ bpos ] = static_cast<short>( fdiv2( ctx.colldata[ cmp ][ bpos ][ dpos ], ctx.cs_sal ) );

							eob = jpg_encode_ac_prg_sa( huffw, storw,
								&ctx.hcodes[ 1 ][ ctx.cmpnfo[ cmp ].huffac ],
								block, &eobrun, ctx.cs_from, ctx.cs_to );

							if ( eob < 0 ) sta = -1;
							else sta = jpg_next_mcuposn( ctx, &cmp, &dpos, &rstw );
						}

						if ( sta != -1 ) {
							if ( jpg_encode_eobrun( huffw, &ctx.hcodes[ 1 ][ ctx.cmpnfo[ cmp ].huffac ], &eobrun ) < 0 )
								sta = -1;
							else
								jpg_encode_crbits( huffw, storw );
						}
					}
				}
			}

			huffw->pad();

			if ( sta == -1 ) {
				ctx.set_errormessage( "encode error in scan%i / mcu%i",
					ctx.scnc, ( ctx.cs_cmpc > 1 ) ? mcu : dpos );
				ctx.errorlevel = 2;
				return false;
			}
			else if ( sta == 2 ) {
				ctx.scnc++;
				break;
			}
			else if ( sta == 1 ) {
				if ( ctx.rsti > 0 ) {
					if ( huffw->num_bytes_written() == 0 ) {
						ctx.errormessage = "invalid restart marker position";
						ctx.errorlevel = 2;
						return false;
					}
					if ( static_cast<std::size_t>( ctx.rstc + 1 ) >= ctx.rstp.size() )
						ctx.rstp.resize( static_cast<std::size_t>( ctx.rstc + 2 ), 0 );
					ctx.rstp[ ctx.rstc++ ] = static_cast<unsigned int>( huffw->num_bytes_written() - 1 );
				}
			}
		}
	}

	ctx.huffdata = huffw->take_bytes();
	const auto hufs = static_cast<unsigned int>( ctx.huffdata.size() );
	if ( !ctx.scnp.empty() )
		ctx.scnp[ ctx.scnc ] = hufs;
	if ( !ctx.rstp.empty() )
		ctx.rstp[ ctx.rstc ] = hufs;

	return true;
}


/* -----------------------------------------------
	Parses header for imageinfo
	----------------------------------------------- */
bool jpg_setup_imginfo( CodecContext& ctx )
{
	unsigned char type = 0x00;
	unsigned int  len  = 0;
	unsigned int  hpos = 0;

	while ( hpos < ctx.hdrdata.size() ) {
		if ( !ctx.read_header_segment( hpos, type, len ) ) return false;
		if ( ( type != 0xDA ) && ( type != 0xC4 ) && ( type != 0xDD ) ) {
			if ( !jpg_parse_jfif( ctx, type, len, &ctx.hdrdata[ hpos ] ) )
				return false;
		}
		hpos += len;
	}

	if ( ctx.cmpc <= 0 ) {
		ctx.errormessage = "header contains incomplete information";
		ctx.errorlevel = 2;
		return false;
	}
	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		if ( ( ctx.cmpnfo[ cmp ].sfv < 1 ) || ( ctx.cmpnfo[ cmp ].sfv > 4 ) ||
			 ( ctx.cmpnfo[ cmp ].sfh < 1 ) || ( ctx.cmpnfo[ cmp ].sfh > 4 ) ||
			 ( ctx.cmpnfo[ cmp ].qtable == nullptr ) ||
			 ( ctx.cmpnfo[ cmp ].qtable[ 0 ] == 0 ) ||
			 ( ctx.jpegtype == 0 ) ) {
			ctx.errormessage = "header information is incomplete";
			ctx.errorlevel = 2;
			return false;
		}
	}

	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		if ( ctx.cmpnfo[ cmp ].sfh > ctx.sfhm ) ctx.sfhm = ctx.cmpnfo[ cmp ].sfh;
		if ( ctx.cmpnfo[ cmp ].sfv > ctx.sfvm ) ctx.sfvm = ctx.cmpnfo[ cmp ].sfv;
	}
	const int mcu_px_v = 8 * ctx.sfhm;
	const int mcu_px_h = 8 * ctx.sfvm;
	ctx.mcuv = ( ctx.imgheight + mcu_px_v - 1 ) / mcu_px_v;
	ctx.mcuh = ( ctx.imgwidth  + mcu_px_h - 1 ) / mcu_px_h;
	if ( ( std::uint64_t( ctx.mcuv ) * std::uint64_t( ctx.mcuh ) ) > 16777216ULL ) {
		ctx.errormessage = "image dimensions exceed maximum supported size";
		ctx.errorlevel = 2;
		return false;
	}
	ctx.mcuc = ctx.mcuv * ctx.mcuh;

	const std::uint64_t stream_bytes = ( ctx.str_in != nullptr ) ? ctx.str_in->get_size() : 0;
	std::uint64_t max_blocks = ( ctx.str_in == nullptr )
		? 16777216ULL
		: std::min<std::uint64_t>(
			16777216ULL,
			std::max<std::uint64_t>(
				4096ULL,
				( stream_bytes <= ( 16777216ULL / 4096ULL ) ) ? ( stream_bytes * 4096ULL ) : 16777216ULL ) );
	if ( ( ctx.filetype == F_JPG ) && ( ctx.jpegtype == 1 ) ) {
		const auto huff_bytes = static_cast<std::uint64_t>( ctx.huffdata.size() );
		max_blocks = std::min<std::uint64_t>(
			16777216ULL,
			std::max<std::uint64_t>( 4096ULL, ( huff_bytes <= ( 16777216ULL / 64ULL ) ) ? ( huff_bytes * 64ULL ) : 16777216ULL ) );
	}

	std::uint64_t total_blocks = 0;
	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		ctx.cmpnfo[ cmp ].mbs = ctx.cmpnfo[ cmp ].sfv * ctx.cmpnfo[ cmp ].sfh;
		ctx.cmpnfo[ cmp ].bcv = ctx.mcuv * ctx.cmpnfo[ cmp ].sfh;
		ctx.cmpnfo[ cmp ].bch = ctx.mcuh * ctx.cmpnfo[ cmp ].sfv;
		const std::uint64_t cmp_bc = std::uint64_t( ctx.cmpnfo[ cmp ].bcv ) * std::uint64_t( ctx.cmpnfo[ cmp ].bch );
		total_blocks += cmp_bc;
		if ( cmp_bc == 0 || total_blocks > max_blocks ) {
			ctx.errormessage = "image dimensions exceed maximum supported size";
			ctx.errorlevel = 2;
			return false;
		}
		ctx.cmpnfo[ cmp ].bc  = static_cast<int>( cmp_bc );
		ctx.cmpnfo[ cmp ].ncv = static_cast<int>(
			( std::uint64_t( ctx.imgheight ) * std::uint64_t( ctx.cmpnfo[ cmp ].sfh ) + std::uint64_t( mcu_px_v - 1 ) ) /
			std::uint64_t( mcu_px_v ) );
		ctx.cmpnfo[ cmp ].nch = static_cast<int>(
			( std::uint64_t( ctx.imgwidth ) * std::uint64_t( ctx.cmpnfo[ cmp ].sfv ) + std::uint64_t( mcu_px_h - 1 ) ) /
			std::uint64_t( mcu_px_h ) );
		ctx.cmpnfo[ cmp ].nc  = ctx.cmpnfo[ cmp ].ncv * ctx.cmpnfo[ cmp ].nch;
	}

	if ( ctx.cmpc <= 3 ) {
		for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) ctx.cmpnfo[ cmp ].sid = cmp;
	}
	else {
		for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) ctx.cmpnfo[ cmp ].sid = 0;
	}

	for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
		const auto bc = static_cast<std::size_t>( ctx.cmpnfo[ cmp ].bc );
		for ( int bpos = 0; bpos < 64; bpos++ ) {
			ctx.colldata[ cmp ][ bpos ].assign( bc, 0 );
		}
		ctx.zdstdata[ cmp ].assign( bc, 0 );
		ctx.eobxhigh[ cmp ].assign( bc, 0 );
		ctx.eobyhigh[ cmp ].assign( bc, 0 );
		ctx.zdstxlow[ cmp ].assign( bc, 0 );
		ctx.zdstylow[ cmp ].assign( bc, 0 );
	}

	if ( ctx.auto_set ) {
		ctx.segm_cnt = { 10, 10, 10, 10 };
		ctx.nois_trs = { 6, 6, 6, 6 };
		for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
			int i = 0;
			for ( ; conf_sets[ i ][ ctx.cmpnfo[ cmp ].sid ] > static_cast<unsigned int>( ctx.cmpnfo[ cmp ].bc ); i++ );
			ctx.segm_cnt[ cmp ] = conf_segm[ i ][ ctx.cmpnfo[ cmp ].sid ];
			ctx.nois_trs[ cmp ] = conf_ntrs[ i ][ ctx.cmpnfo[ cmp ].sid ];
		}
	}

	return true;
}


/* -----------------------------------------------
	Parse routines for JFIF segments
	----------------------------------------------- */
bool jpg_parse_jfif( CodecContext& ctx, unsigned char type, unsigned int len, const unsigned char* segment )
{
	unsigned int hpos = 4;

	switch ( type ) {
		case 0xC4: // DHT segment
			while ( hpos < len ) {
				if ( hpos + 17 > len )
					break;
				const int lval = static_cast<int>( lbits_u8( segment[ hpos ], 4 ) );
				const int rval = static_cast<int>( rbits_u8( segment[ hpos ], 4 ) );
				if ( ( lval < 0 ) || ( lval >= 2 ) || ( rval < 0 ) || ( rval >= 4 ) )
					break;

				hpos++;
				int skip = 16;
				int kraft = 0;
				int i = 0;
				for ( ; i < 16; i++ ) {
					skip += static_cast<int>( segment[ hpos + i ] );
					kraft = ( kraft << 1 ) + static_cast<int>( segment[ hpos + i ] );
					if ( kraft > ( 1 << ( i + 1 ) ) )
						break;
				}
				if ( ( i < 16 ) || ( skip - 16 > 256 ) || ( hpos + static_cast<unsigned int>( skip ) > len ) )
					break;

				if ( ctx.filetype != F_PJG ) {
					std::array<unsigned char, 256> seen{};
					for ( int k = 0; k < skip - 16; k++ ) {
						const unsigned char sym = segment[ hpos + 16 + k ];
						if ( seen[ sym ] != 0 ) {
							ctx.errormessage = "reconstruction of inefficient coding not supported";
							ctx.errorlevel = 1;
							break;
						}
						seen[ sym ] = 1;
					}
				}


				jpg_build_huffcodes( &segment[ hpos + 0 ], &segment[ hpos + 16 ],
					&ctx.hcodes[ lval ][ rval ], &ctx.htrees[ lval ][ rval ] );
				ctx.htset[ lval ][ rval ] = 1;
				hpos += static_cast<unsigned int>( skip );
			}

			if ( hpos != len ) {
				ctx.errormessage = "size mismatch in dht marker";
				ctx.errorlevel = 2;
				return false;
			}
			return true;

		case 0xDB: // DQT segment
			while ( hpos < len ) {
				const int lval = static_cast<int>( lbits_u8( segment[ hpos ], 4 ) );
				const int rval = static_cast<int>( rbits_u8( segment[ hpos ], 4 ) );
				if ( ( lval < 0 ) || ( lval >= 2 ) ) break;
				if ( ( rval < 0 ) || ( rval >= 4 ) ) break;
				hpos++;
				if ( lval == 0 ) {
					if ( hpos + 64 > len ) break;
					int i = 0;
					for ( ; i < 64; i++ ) {
						ctx.qtables[ rval ][ i ] = segment[ hpos + i ];
						if ( ctx.qtables[ rval ][ i ] == 0 ) break;
					}
					if ( i < 64 ) {
						ctx.errormessage = "invalid quantization value in dqt marker";
						ctx.errorlevel = 2;
						return false;
					}
					hpos += 64;
				}
				else {
					if ( hpos + 128 > len ) break;
					int i = 0;
					for ( ; i < 64; i++ ) {
						ctx.qtables[ rval ][ i ] = static_cast<unsigned short>(
							b_short( segment[ hpos + ( 2 * i ) ], segment[ hpos + ( 2 * i ) + 1 ] ) );
						if ( ctx.qtables[ rval ][ i ] == 0 ) break;
					}
					if ( i < 64 ) {
						ctx.errormessage = "invalid quantization value in dqt marker";
						ctx.errorlevel = 2;
						return false;
					}
					hpos += 128;
				}
			}

			if ( hpos != len ) {
				ctx.errormessage = "size mismatch in dqt marker";
				ctx.errorlevel = 2;
				return false;
			}
			return true;

		case 0xDD: // DRI segment
			if ( hpos + 2 > len ) {
				ctx.errormessage = "size mismatch in dri marker";
				ctx.errorlevel = 2;
				return false;
			}
			ctx.rsti = b_short( segment[ hpos ], segment[ hpos + 1 ] );
			return true;

		case 0xDA: // SOS segment
			if ( hpos + 1 > len ) {
				ctx.errormessage = "size mismatch in sos marker";
				ctx.errorlevel = 2;
				return false;
			}
			ctx.cs_cmpc = segment[ hpos ];
			if ( ( ctx.cs_cmpc <= 0 ) || ( ctx.cs_cmpc > ctx.cmpc ) ) {
				ctx.set_errormessage( "%i components in scan, only %i are allowed", ctx.cs_cmpc, ctx.cmpc );
				ctx.errorlevel = 2;
				return false;
			}
			hpos++;
			if ( hpos + static_cast<unsigned int>( ctx.cs_cmpc ) * 2 + 3 > len ) {
				ctx.errormessage = "size mismatch in sos marker";
				ctx.errorlevel = 2;
				return false;
			}
			for ( int i = 0; i < ctx.cs_cmpc; i++ ) {
				int cmp = 0;
				for ( ; ( cmp < ctx.cmpc ) && ( segment[ hpos ] != ctx.cmpnfo[ cmp ].jid ); cmp++ );
				if ( cmp == ctx.cmpc ) {
					ctx.errormessage = "component id mismatch in start-of-scan";
					ctx.errorlevel = 2;
					return false;
				}
				for ( int prev = 0; prev < i; prev++ ) {
					if ( ctx.cs_cmp[ prev ] == cmp ) {
						ctx.errormessage = "duplicate component in start-of-scan";
						ctx.errorlevel = 2;
						return false;
					}
				}
				ctx.cs_cmp[ i ] = cmp;
				ctx.cmpnfo[ cmp ].huffdc = static_cast<int>( lbits_u8( segment[ hpos + 1 ], 4 ) );
				ctx.cmpnfo[ cmp ].huffac = static_cast<int>( rbits_u8( segment[ hpos + 1 ], 4 ) );
				if ( ( ctx.cmpnfo[ cmp ].huffdc < 0 ) || ( ctx.cmpnfo[ cmp ].huffdc >= 4 ) ||
					 ( ctx.cmpnfo[ cmp ].huffac < 0 ) || ( ctx.cmpnfo[ cmp ].huffac >= 4 ) ) {
					ctx.errormessage = "huffman table number mismatch";
					ctx.errorlevel = 2;
					return false;
				}
				hpos += 2;
			}
			ctx.cs_from = segment[ hpos + 0 ];
			ctx.cs_to   = segment[ hpos + 1 ];
			ctx.cs_sah  = static_cast<int>( lbits_u8( segment[ hpos + 2 ], 4 ) );
			ctx.cs_sal  = static_cast<int>( rbits_u8( segment[ hpos + 2 ], 4 ) );
			if ( ( ctx.cs_from > ctx.cs_to ) || ( ctx.cs_from > 63 ) || ( ctx.cs_to > 63 ) ) {
				ctx.errormessage = "spectral selection parameter out of range";
				ctx.errorlevel = 2;
				return false;
			}
			if ( ( ctx.cs_sah >= 12 ) || ( ctx.cs_sal >= 12 ) ) {
				ctx.errormessage = "successive approximation parameter out of range";
				ctx.errorlevel = 2;
				return false;
			}
			return true;

		case 0xC0: // SOF0
		case 0xC1: // SOF1
		case 0xC2: // SOF2
			if ( ctx.jpegtype != 0 ) {
				ctx.errormessage = "multiple sof markers in header";
				ctx.errorlevel = 2;
				return false;
			}
			ctx.jpegtype = ( type == 0xC2 ) ? 2 : 1;
			if ( hpos + 6 > len ) {
				ctx.errormessage = "size mismatch in sof marker";
				ctx.errorlevel = 2;
				return false;
			}
			if ( segment[ hpos ] != 8 ) {
				ctx.set_errormessage( "%i bit data precision is not supported", static_cast<int>( segment[ hpos ] ) );
				ctx.errorlevel = 2;
				return false;
			}
			ctx.imgheight = b_short( segment[ hpos + 1 ], segment[ hpos + 2 ] );
			ctx.imgwidth  = b_short( segment[ hpos + 3 ], segment[ hpos + 4 ] );
			ctx.cmpc      = segment[ hpos + 5 ];
			if ( ( ctx.imgwidth == 0 ) || ( ctx.imgheight == 0 ) ) {
				ctx.set_errormessage( "resolution is %ix%i, possible malformed JPEG", ctx.imgwidth, ctx.imgheight );
				ctx.errorlevel = 2;
				return false;
			}
			if ( ( ctx.cmpc <= 0 ) || ( ctx.cmpc > 4 ) ) {
				ctx.set_errormessage( "image has %i components, 1..4 are supported", ctx.cmpc );
				ctx.errorlevel = 2;
				return false;
			}
			hpos += 6;
			if ( hpos + static_cast<unsigned int>( ctx.cmpc ) * 3 != len ) {
				ctx.errormessage = "size mismatch in sof marker";
				ctx.errorlevel = 2;
				return false;
			}
			for ( int cmp = 0; cmp < ctx.cmpc; cmp++ ) {
				ctx.cmpnfo[ cmp ].jid = segment[ hpos ];
				for ( int prev = 0; prev < cmp; prev++ ) {
					if ( ctx.cmpnfo[ prev ].jid == ctx.cmpnfo[ cmp ].jid ) {
						ctx.errormessage = "duplicate component id in sof marker";
						ctx.errorlevel = 2;
						return false;
					}
				}
				ctx.cmpnfo[ cmp ].sfv = static_cast<int>( lbits_u8( segment[ hpos + 1 ], 4 ) );
				ctx.cmpnfo[ cmp ].sfh = static_cast<int>( rbits_u8( segment[ hpos + 1 ], 4 ) );
				if ( ( ctx.cmpnfo[ cmp ].sfv < 1 ) || ( ctx.cmpnfo[ cmp ].sfv > 4 ) ||
					 ( ctx.cmpnfo[ cmp ].sfh < 1 ) || ( ctx.cmpnfo[ cmp ].sfh > 4 ) ) {
					ctx.errormessage = "invalid sampling factor in sof marker";
					ctx.errorlevel = 2;
					return false;
				}
				if ( segment[ hpos + 2 ] >= 4 ) {
					ctx.errormessage = "quantization table index out of range";
					ctx.errorlevel = 2;
					return false;
				}
				ctx.cmpnfo[ cmp ].qtable = ctx.qtables[ segment[ hpos + 2 ] ].data();
				hpos += 3;
			}
			return true;

		case 0xC3:
			ctx.errormessage = "sof3 marker found, image is coded lossless";
			ctx.errorlevel = 2;
			return false;
		case 0xC5:
			ctx.errormessage = "sof5 marker found, image is coded diff. sequential";
			ctx.errorlevel = 2;
			return false;
		case 0xC6:
			ctx.errormessage = "sof6 marker found, image is coded diff. progressive";
			ctx.errorlevel = 2;
			return false;
		case 0xC7:
			ctx.errormessage = "sof7 marker found, image is coded diff. lossless";
			ctx.errorlevel = 2;
			return false;
		case 0xC9:
			ctx.errormessage = "sof9 marker found, image is coded arithm. sequential";
			ctx.errorlevel = 2;
			return false;
		case 0xCA:
			ctx.errormessage = "sof10 marker found, image is coded arithm. progressive";
			ctx.errorlevel = 2;
			return false;
		case 0xCB:
			ctx.errormessage = "sof11 marker found, image is coded arithm. lossless";
			ctx.errorlevel = 2;
			return false;
		case 0xCD:
			ctx.errormessage = "sof13 marker found, image is coded arithm. diff. sequential";
			ctx.errorlevel = 2;
			return false;
		case 0xCE:
			ctx.errormessage = "sof14 marker found, image is coded arithm. diff. progressive";
			ctx.errorlevel = 2;
			return false;
		case 0xCF:
			ctx.errormessage = "sof15 marker found, image is coded arithm. diff. lossless";
			ctx.errorlevel = 2;
			return false;

		case 0xE0: case 0xE1: case 0xE2: case 0xE3:
		case 0xE4: case 0xE5: case 0xE6: case 0xE7:
		case 0xE8: case 0xE9: case 0xEA: case 0xEB:
		case 0xEC: case 0xED: case 0xEE: case 0xEF:
		case 0xFE:
			return true;

		case 0xD0: case 0xD1: case 0xD2: case 0xD3:
		case 0xD4: case 0xD5: case 0xD6: case 0xD7:
			ctx.errormessage = "rst marker found out of place";
			ctx.errorlevel = 2;
			return false;

		case 0xD8:
			ctx.errormessage = "soi marker found out of place";
			ctx.errorlevel = 2;
			return false;

		case 0xD9:
			ctx.errormessage = "eoi marker found out of place";
			ctx.errorlevel = 2;
			return false;

		default:
			if ( ctx.filetype != F_PJG ) {
				ctx.set_errormessage( "unknown marker found: FF %2X", type );
				ctx.errorlevel = 1;
			}
			return true;
	}
}


/* -----------------------------------------------
	JFIF header rebuilding routine
	----------------------------------------------- */
bool jpg_rebuild_header( CodecContext& ctx )
{
	unsigned char type = 0x00;
	unsigned int  len  = 0;
	unsigned int  hpos = 0;

	MemoryWriter hdrw;

	while ( hpos < ctx.hdrdata.size() ) {
		if ( !ctx.read_header_segment( hpos, type, len ) ) return false;
		if ( ( type == 0xDA ) || ( type == 0xC4 ) || ( type == 0xDB ) ||
			 ( type == 0xC0 ) || ( type == 0xC1 ) || ( type == 0xC2 ) ||
			 ( type == 0xDD ) ) {
			hdrw.write( &ctx.hdrdata[ hpos ], len );
		}
		hpos += len;
	}

	ctx.hdrdata = hdrw.take_data();
	return true;
}


/* -----------------------------------------------
	returns next code (from huffman-tree & -data)
	----------------------------------------------- */
int jpg_next_huffcode( BitReader* huffr, const HuffTree* ctree )
{
	if ( huffr->has_fast_16() ) {
		const unsigned int peek = huffr->peek_9_fast();
		const int len = ctree->lut_len[ peek ];
		if ( len > 0 ) {
			huffr->consume_fast( len );
			return static_cast<int>( ctree->lut_sym[ peek ] );
		}
	}

	int node = 0;

	while ( node < 512 ) {
		node = ( huffr->read_bit() == 1 ) ? ctree->r[ node ] : ctree->l[ node ];
		if ( node == 0 ) break;
	}

	return ( node - 512 );
}


/* -----------------------------------------------
	creates huffman-codes & -trees from dht-data
	----------------------------------------------- */
void jpg_build_huffcodes( const unsigned char* clen, const unsigned char* cval, HuffCodes* hc, HuffTree* ht )
{
	std::memset( hc->clen, 0, sizeof( hc->clen ) );
	std::memset( hc->cval, 0, sizeof( hc->cval ) );
	std::memset( ht->l, 0, sizeof( ht->l ) );
	std::memset( ht->r, 0, sizeof( ht->r ) );
	std::memset( ht->lut_sym, 0, sizeof( ht->lut_sym ) );
	std::memset( ht->lut_len, 0, sizeof( ht->lut_len ) );

	int k        = 0;
	int code     = 0;
	int nextfree = 1;

	for ( int i = 0; i < 16; i++ ) {
		const int len = 1 + i;
		for ( int j = 0; j < static_cast<int>( clen[ i ] ); j++ ) {
			const int sym = static_cast<int>( cval[ k ] );
			if ( hc->clen[ sym ] == 0 ) {
				hc->clen[ sym ] = static_cast<unsigned short>( len );
				hc->cval[ sym ] = static_cast<unsigned short>( code );
			}

			if ( len <= 9 ) {
				const int shift = 9 - len;
				const int base  = code << shift;
				const int count = 1 << shift;
				if ( ( base >= 0 ) && ( base + count <= 512 ) ) {
					std::memset( &ht->lut_sym[ base ], sym, static_cast<std::size_t>( count ) );
					std::memset( &ht->lut_len[ base ], len, static_cast<std::size_t>( count ) );
				}
			}

			int node = 0;
			bool tree_ok = true;
			for ( int b = len - 1; b > 0; b-- ) {
				if ( bit_n( code, b ) == 1 ) {
					if ( ht->r[ node ] == 0 ) {
						if ( nextfree >= 512 ) { tree_ok = false; break; }
						ht->r[ node ] = static_cast<unsigned short>( nextfree++ );
					}
					node = ht->r[ node ];
				}
				else {
					if ( ht->l[ node ] == 0 ) {
						if ( nextfree >= 512 ) { tree_ok = false; break; }
						ht->l[ node ] = static_cast<unsigned short>( nextfree++ );
					}
					node = ht->l[ node ];
				}
				if ( node >= 512 ) { tree_ok = false; break; }
			}
			if ( tree_ok ) {
				if ( bit_n( code, 0 ) == 1 )
					ht->r[ node ] = static_cast<unsigned short>( sym + 512 );
				else
					ht->l[ node ] = static_cast<unsigned short>( sym + 512 );
			}

			k++;
			code++;
		}
		code <<= 1;
	}

	hc->max_eobrun = 0;
	for ( int i = 14; i >= 0; i-- ) {
		if ( hc->clen[ i << 4 ] > 0 ) {
			hc->max_eobrun = static_cast<unsigned short>( ( 2 << i ) - 1 );
			break;
		}
	}
}

