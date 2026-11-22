#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#if defined(_WIN32) || defined(WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

struct CliInputItem {
	std::string path;
	std::string rel_root;
};

#if defined(_WIN32) || defined(WIN32)
inline std::wstring widen_utf8_or_acp( const std::string& s )
{
	if ( s.empty() ) {
		return {};
	}
	int n = MultiByteToWideChar(
		CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>( s.size() ), nullptr, 0 );
	UINT cp = CP_UTF8;
	DWORD flags = MB_ERR_INVALID_CHARS;
	if ( n <= 0 ) {
		cp = CP_ACP;
		flags = 0;
		n = MultiByteToWideChar( cp, flags, s.data(), static_cast<int>( s.size() ), nullptr, 0 );
	}
	if ( n <= 0 ) {
		return {};
	}
	std::wstring out( static_cast<std::size_t>( n ), L'\0' );
	MultiByteToWideChar( cp, flags, s.data(), static_cast<int>( s.size() ), out.data(), n );
	return out;
}

inline std::string narrow_utf8_from_wide( std::wstring_view ws )
{
	if ( ws.empty() ) {
		return {};
	}
	const int n = WideCharToMultiByte(
		CP_UTF8, 0, ws.data(), static_cast<int>( ws.size() ), nullptr, 0, nullptr, nullptr );
	if ( n <= 0 ) {
		return {};
	}
	std::string out( static_cast<std::size_t>( n ), '\0' );
	WideCharToMultiByte(
		CP_UTF8, 0, ws.data(), static_cast<int>( ws.size() ), out.data(), n, nullptr, nullptr );
	return out;
}
#endif

inline std::filesystem::path path_from_utf8( const std::string& s )
{
#if defined(_WIN32) || defined(WIN32)
	return std::filesystem::path( widen_utf8_or_acp( s ) );
#else
	return std::filesystem::path( s );
#endif
}

inline std::string path_to_utf8( const std::filesystem::path& p )
{
#if defined(_WIN32) || defined(WIN32)
	return narrow_utf8_from_wide( p.wstring() );
#else
	return p.string();
#endif
}

inline FILE* fopen_utf8( const std::string& path_utf8, const char* mode ) noexcept
{
	if ( path_utf8.empty() || ( mode == nullptr ) ) {
		return nullptr;
	}
	try {
#if defined(_WIN32) || defined(WIN32)
		const std::wstring wpath = widen_utf8_or_acp( path_utf8 );
		const std::wstring wmode = widen_utf8_or_acp( mode );
		if ( wpath.empty() || wmode.empty() ) {
			return nullptr;
		}
		return _wfopen( wpath.c_str(), wmode.c_str() );
#else
		return std::fopen( path_utf8.c_str(), mode );
#endif
	} catch ( ... ) {
		return nullptr;
	}
}

inline bool path_or_symlink_exists( const std::filesystem::path& p ) noexcept
{
	try {
		std::error_code ec;
		if ( std::filesystem::exists( p, ec ) ) {
			return true;
		}
		ec.clear();
		const auto st = std::filesystem::symlink_status( p, ec );
		return !ec && ( st.type() != std::filesystem::file_type::not_found );
	} catch ( ... ) {
		return false;
	}
}

inline bool file_exists( const std::string& filename ) noexcept
{
	if ( filename.empty() ) {
		return false;
	}
	try {
		return path_or_symlink_exists( path_from_utf8( filename ) );
	} catch ( ... ) {
		return false;
	}
}

inline bool remove_file_utf8( const std::string& filename ) noexcept
{
	if ( filename.empty() ) {
		return false;
	}
	try {
		std::error_code ec;
		return std::filesystem::remove( path_from_utf8( filename ), ec ) && !ec;
	} catch ( ... ) {
		return false;
	}
}

inline bool paths_refer_to_same_existing_file( const std::string& a, const std::string& b ) noexcept
{
	if ( a.empty() || b.empty() ) {
		return false;
	}
	try {
		std::error_code ec;
		const bool same = std::filesystem::equivalent( path_from_utf8( a ), path_from_utf8( b ), ec );
		return !ec && same;
	} catch ( ... ) {
		return false;
	}
}

inline std::filesystem::path resolve_symlink_chain( std::filesystem::path cur )
{
	std::error_code ec;
	for ( int depth = 0; depth < 16; ++depth ) {
		const auto st = std::filesystem::symlink_status( cur, ec );
		if ( ec || !std::filesystem::is_symlink( st ) ) {
			break;
		}
		const std::filesystem::path target = std::filesystem::read_symlink( cur, ec );
		if ( ec || target.empty() ) {
			break;
		}
		cur = target.is_absolute() ? target : ( cur.parent_path() / target );
	}
	return cur;
}

inline std::string normalize_path_key( const std::string& raw )
{
	if ( raw.empty() ) {
		return {};
	}
	const std::filesystem::path resolved = resolve_symlink_chain( path_from_utf8( raw ) );
	std::error_code ec;
	std::filesystem::path norm = std::filesystem::weakly_canonical( resolved, ec );
	if ( ec ) {
		ec.clear();
		norm = std::filesystem::absolute( resolved, ec ).lexically_normal();
		if ( ec ) {
			norm = resolved.lexically_normal();
		}
	}
#if defined(_WIN32) || defined(WIN32)
	std::wstring wkey = norm.wstring();
	for ( wchar_t& wc : wkey ) {
		if ( wc == L'/' ) {
			wc = L'\\';
		}
	}
	if ( !wkey.empty() ) {
		LCMapStringW(
			LOCALE_INVARIANT,
			LCMAP_LOWERCASE,
			wkey.data(),
			static_cast<int>( wkey.size() ),
			wkey.data(),
			static_cast<int>( wkey.size() ) );
	}
	return narrow_utf8_from_wide( wkey );
#else
	return norm.string();
#endif
}

inline std::string make_filename( const std::string& base, const char* extension )
{
	const std::filesystem::path p = path_from_utf8( base );
	const std::filesystem::path dir = p.parent_path();
	const std::string stem = path_to_utf8( p.stem() );
	const std::string leaf = ( extension != nullptr )
		? ( stem + "." + extension )
		: stem;
	const std::filesystem::path leaf_p = path_from_utf8( leaf );
	return path_to_utf8( dir.empty() ? leaf_p : ( dir / leaf_p ) );
}

inline std::string make_unique_filename( const std::string& base, const char* extension )
{
	const std::filesystem::path p = path_from_utf8( base );
	const std::filesystem::path dir = p.parent_path();
	std::string stem = path_to_utf8( p.stem() );
	const std::string suffix = ( extension != nullptr )
		? ( std::string( "." ) + extension )
		: std::string();

	auto build = [&]() {
		const std::filesystem::path leaf_p = path_from_utf8( stem + suffix );
		return dir.empty() ? leaf_p : ( dir / leaf_p );
	};

	std::filesystem::path candidate = build();
	while ( path_or_symlink_exists( candidate ) ) {
		stem.push_back( '_' );
		candidate = build();
	}
	return path_to_utf8( candidate );
}

inline std::string make_output_base(
	const std::string& input_file,
	const std::string& out_dir,
	const std::string& rel_root )
{
	if ( out_dir.empty() ) {
		return input_file;
	}

	const std::filesystem::path in_path = path_from_utf8( input_file );
	std::filesystem::path dest_dir = path_from_utf8( out_dir );

	if ( !rel_root.empty() ) {
		const std::filesystem::path rel_parent =
			in_path.parent_path().lexically_relative( path_from_utf8( rel_root ) );
		if ( !rel_parent.empty() && ( rel_parent != "." ) ) {
			const std::string first_part = path_to_utf8( *rel_parent.begin() );
			if ( first_part != ".." ) {
				dest_dir /= rel_parent;
			}
		}
	}

	return path_to_utf8( dest_dir / in_path.filename() );
}

inline bool ensure_parent_directory( const std::string& file_path ) noexcept
{
	try {
		const std::filesystem::path parent = path_from_utf8( file_path ).parent_path();
		if ( parent.empty() ) {
			return true;
		}
		std::error_code ec;
		if ( std::filesystem::is_directory( parent, ec ) ) {
			return true;
		}
		ec.clear();
		std::filesystem::create_directories( parent, ec );
		return !ec && std::filesystem::is_directory( parent, ec );
	} catch ( ... ) {
		return false;
	}
}

inline bool can_overwrite_or_create_target( const std::string& target_utf8, std::string& err ) noexcept
{
	if ( target_utf8.empty() ) {
		err = "empty output filename";
		return false;
	}
	try {
		const std::filesystem::path p = path_from_utf8( target_utf8 );
		std::error_code ec;
		if ( std::filesystem::is_directory( p, ec ) ) {
			err = "Unable to open " + target_utf8 + " for writing (target is a directory).";
			return false;
		}
		ec.clear();
		if ( std::filesystem::exists( p, ec ) ) {
#if defined(_WIN32) || defined(WIN32)
			const std::wstring wp = p.wstring();
			const DWORD attrs = GetFileAttributesW( wp.c_str() );
			if ( ( attrs != INVALID_FILE_ATTRIBUTES ) && ( ( attrs & FILE_ATTRIBUTE_READONLY ) != 0 ) ) {
				err = "Unable to open " + target_utf8 + " for writing.";
				return false;
			}
			HANDLE h = CreateFileW(
				wp.c_str(),
				GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr );
			if ( h == INVALID_HANDLE_VALUE ) {
				err = "Unable to open " + target_utf8 + " for writing.";
				return false;
			}
			CloseHandle( h );
#else
			if ( ::access( target_utf8.c_str(), W_OK ) != 0 ) {
				err = "Unable to open " + target_utf8 + " for writing.";
				return false;
			}
#endif
		}
		return true;
	} catch ( ... ) {
		err = "Unable to open " + target_utf8 + " for writing.";
		return false;
	}
}

inline std::string make_staging_temp_path( const std::string& target_utf8 )
{
	static std::atomic<std::uint64_t> seq{ 0 };
	const std::filesystem::path target_p = path_from_utf8( target_utf8 );
	const std::filesystem::path dir = target_p.parent_path();
#if defined(_WIN32) || defined(WIN32)
	const unsigned long pid = static_cast<unsigned long>( GetCurrentProcessId() );
#else
	const unsigned long pid = static_cast<unsigned long>( ::getpid() );
#endif
	for ( unsigned int attempt = 0; attempt < 4096; ++attempt ) {
		const std::uint64_t id = seq.fetch_add( 1, std::memory_order_relaxed ) + 1;
		const std::string leaf = ".packjpg_tmp_" + std::to_string( pid ) + "_" + std::to_string( id ) + ".part";
		const std::filesystem::path cand = dir.empty() ? path_from_utf8( leaf ) : ( dir / path_from_utf8( leaf ) );
		if ( !path_or_symlink_exists( cand ) ) {
			return path_to_utf8( cand );
		}
	}
	return target_utf8 + ".packjpg_tmp";
}

inline bool commit_staging_file( const std::string& staging_utf8, const std::string& target_utf8, std::string& err ) noexcept
{
	try {
#if defined(_WIN32) || defined(WIN32)
		const std::wstring wsrc = widen_utf8_or_acp( staging_utf8 );
		const std::wstring wdst = widen_utf8_or_acp( target_utf8 );
		if ( wsrc.empty() || wdst.empty() ||
		     !MoveFileExW( wsrc.c_str(), wdst.c_str(), MOVEFILE_REPLACE_EXISTING ) ) {
			err = "failed to commit output file " + target_utf8;
			return false;
		}
		return true;
#else
		std::error_code ec;
		std::filesystem::rename( path_from_utf8( staging_utf8 ), path_from_utf8( target_utf8 ), ec );
		if ( ec ) {
			err = "failed to commit output file " + target_utf8 + ": " + ec.message();
			return false;
		}
		return true;
#endif
	} catch ( ... ) {
		err = "failed to commit output file " + target_utf8;
		return false;
	}
}

inline bool has_wildcard_chars( std::string_view s ) noexcept
{
	return ( s.find( '*' ) != std::string_view::npos ) ||
	       ( s.find( '?' ) != std::string_view::npos );
}

inline char ascii_fold( char c, bool case_insensitive ) noexcept
{
	return case_insensitive
		? static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) )
		: c;
}

inline bool wildcard_match( std::string_view pattern, std::string_view text, bool case_insensitive ) noexcept
{
	// Leading-dot hidden entries (e.g. ".jpg", "._photo.jpg") require an explicit leading '.' in pattern.
	if ( !text.empty() && ( text.front() == '.' ) && ( pattern.empty() || ( pattern.front() != '.' ) ) ) {
		return false;
	}

	std::size_t pi = 0;
	std::size_t ti = 0;
	std::size_t star_pi  = std::string_view::npos;
	std::size_t match_ti = 0;

	while ( ti < text.size() ) {
		if ( ( pi < pattern.size() ) &&
		     ( ( pattern[ pi ] == '?' ) ||
		       ( ascii_fold( pattern[ pi ], case_insensitive ) == ascii_fold( text[ ti ], case_insensitive ) ) ) ) {
			++pi;
			++ti;
		}
		else if ( ( pi < pattern.size() ) && ( pattern[ pi ] == '*' ) ) {
			star_pi  = pi++;
			match_ti = ti;
		}
		else if ( star_pi != std::string_view::npos ) {
			pi = star_pi + 1;
			ti = ++match_ti;
		}
		else {
			return false;
		}
	}

	while ( ( pi < pattern.size() ) && ( pattern[ pi ] == '*' ) ) {
		++pi;
	}
	return pi == pattern.size();
}

inline bool is_supported_codec_file( const std::filesystem::path& p )
{
	const std::string fname = path_to_utf8( p.filename() );
	if ( fname.empty() || ( fname.front() == '.' ) ) {
		return false;
	}
	std::string ext = path_to_utf8( p.extension() );
	for ( char& c : ext ) {
		c = static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );
	}
	return ( ext == ".jpg" ) || ( ext == ".jpeg" ) || ( ext == ".jpe" ) ||
	       ( ext == ".jfif" ) || ( ext == ".pjg" );
}

inline std::vector<CliInputItem> expand_cli_inputs(
	const std::vector<std::string>& raw_inputs,
	bool recursive,
	const std::string& out_dir,
	bool& had_directory_or_glob )
{
#if defined(_WIN32) || defined(WIN32)
	constexpr bool kCaseInsensitiveGlob = true;
#else
	constexpr bool kCaseInsensitiveGlob = false;
#endif

	std::vector<CliInputItem> expanded;
	expanded.reserve( raw_inputs.size() );
	had_directory_or_glob = false;

	const std::string norm_out_dir = out_dir.empty() ? std::string() : normalize_path_key( out_dir );
	auto is_excluded_out_subdir = [&]( const std::filesystem::path& dir_path ) noexcept {
		if ( out_dir.empty() ) {
			return false;
		}
		try {
			const std::string s = path_to_utf8( dir_path );
			if ( paths_refer_to_same_existing_file( s, out_dir ) ) {
				return true;
			}
			return !norm_out_dir.empty() && ( normalize_path_key( s ) == norm_out_dir );
		} catch ( ... ) {
			return false;
		}
	};

	for ( const std::string& raw : raw_inputs ) {
		if ( raw == "-" ) {
			expanded.push_back( { "-", {} } );
			continue;
		}

		std::error_code ec;
		const std::filesystem::path p = path_from_utf8( raw );
		const bool exists_literally = std::filesystem::exists( p, ec );

		if ( exists_literally ) {
			if ( recursive && std::filesystem::is_directory( p, ec ) ) {
				had_directory_or_glob = true;
				std::vector<std::string> found;
				const auto opts = std::filesystem::directory_options::skip_permission_denied;
				std::filesystem::recursive_directory_iterator it( p, opts, ec ), end;
				if ( !ec ) {
					while ( it != end ) {
						std::error_code entry_ec;
						const std::filesystem::path entry_path = it->path();
						const std::string entry_name = path_to_utf8( entry_path.filename() );
						if ( it->is_directory( entry_ec ) ) {
							if ( ( !entry_name.empty() && entry_name.front() == '.' ) ||
							     is_excluded_out_subdir( entry_path ) ) {
								it.disable_recursion_pending();
							}
						}
						else if ( it->is_regular_file( entry_ec ) && is_supported_codec_file( entry_path ) ) {
							found.push_back( path_to_utf8( entry_path ) );
						}
						ec.clear();
						it.increment( ec );
						if ( ec ) {
							ec.clear();
						}
					}
				}
				std::sort( found.begin(), found.end() );
				for ( std::string& f : found ) {
					expanded.push_back( { std::move( f ), raw } );
				}
			}
			else {
				expanded.push_back( { raw, {} } );
			}
			continue;
		}

		const std::string fname = path_to_utf8( p.filename() );
		const std::filesystem::path parent = p.parent_path();
		if ( has_wildcard_chars( fname ) && !has_wildcard_chars( path_to_utf8( parent ) ) ) {
			const std::filesystem::path search_dir = parent.empty() ? std::filesystem::path( "." ) : parent;
			if ( std::filesystem::is_directory( search_dir, ec ) ) {
				std::vector<std::string> matches;
				const auto opts = std::filesystem::directory_options::skip_permission_denied;
				if ( recursive ) {
					std::filesystem::recursive_directory_iterator it( search_dir, opts, ec ), end;
					if ( !ec ) {
						while ( it != end ) {
							std::error_code entry_ec;
							const std::filesystem::path entry_path = it->path();
							const std::string entry_name = path_to_utf8( entry_path.filename() );
							if ( it->is_directory( entry_ec ) ) {
								if ( ( !entry_name.empty() && ( entry_name.front() == '.' ) && ( fname.front() != '.' ) ) ||
								     is_excluded_out_subdir( entry_path ) ) {
									it.disable_recursion_pending();
								}
							}
							else if ( it->is_regular_file( entry_ec ) &&
							          wildcard_match( fname, entry_name, kCaseInsensitiveGlob ) ) {
								matches.push_back( path_to_utf8( entry_path ) );
							}
							ec.clear();
							it.increment( ec );
							if ( ec ) {
								ec.clear();
							}
						}
					}
				}
				else {
					std::filesystem::directory_iterator it( search_dir, opts, ec ), end;
					if ( !ec ) {
						while ( it != end ) {
							std::error_code entry_ec;
							const std::string entry_name = path_to_utf8( it->path().filename() );
							if ( it->is_regular_file( entry_ec ) &&
							     wildcard_match( fname, entry_name, kCaseInsensitiveGlob ) ) {
								const std::filesystem::path rel = parent.empty()
									? it->path().filename()
									: ( parent / it->path().filename() );
								matches.push_back( path_to_utf8( rel ) );
							}
							ec.clear();
							it.increment( ec );
							if ( ec ) {
								ec.clear();
							}
						}
					}
				}
				if ( !matches.empty() ) {
					had_directory_or_glob = true;
					std::sort( matches.begin(), matches.end() );
					const std::string root = recursive ? path_to_utf8( search_dir ) : std::string();
					for ( std::string& m : matches ) {
						expanded.push_back( { std::move( m ), root } );
					}
					continue;
				}
			}
		}

		expanded.push_back( { raw, {} } );
	}

	return expanded;
}

#endif
