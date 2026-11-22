#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#if defined(_WIN32) || defined(WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/syscall.h>
#endif
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

inline void lowercase_wide_inplace( std::wstring& ws ) noexcept
{
	if ( !ws.empty() ) {
		LCMapStringW(
			LOCALE_INVARIANT,
			LCMAP_LOWERCASE,
			ws.data(),
			static_cast<int>( ws.size() ),
			ws.data(),
			static_cast<int>( ws.size() ) );
	}
}

inline std::wstring widen_win32_path( const std::string& path_utf8 )
{
	std::wstring p = widen_utf8_or_acp( path_utf8 );
	if ( p.empty() || ( p.rfind( L"\\\\?\\", 0 ) == 0 ) || ( p.rfind( L"\\\\.\\", 0 ) == 0 ) ) {
		return p;
	}
	const DWORD need = GetFullPathNameW( p.c_str(), 0, nullptr, nullptr );
	if ( need > 1 ) {
		std::wstring full_buf( static_cast<std::size_t>( need ), L'\0' );
		const DWORD got = GetFullPathNameW( p.c_str(), need, full_buf.data(), nullptr );
		if ( ( got > 0 ) && ( got < need ) ) {
			full_buf.resize( static_cast<std::size_t>( got ) );
			p = std::move( full_buf );
		}
	}
	if ( p.size() >= 240 ) {
		for ( wchar_t& wc : p ) {
			if ( wc == L'/' ) wc = L'\\';
		}
		if ( p.rfind( L"\\\\", 0 ) == 0 ) {
			return L"\\\\?\\UNC\\" + p.substr( 2 );
		}
		if ( ( p.size() >= 3 ) && ( p[ 1 ] == L':' ) && ( p[ 2 ] == L'\\' ) ) {
			return L"\\\\?\\" + p;
		}
	}
	return p;
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
		const std::wstring wpath = widen_win32_path( path_utf8 );
		const std::wstring wmode = widen_utf8_or_acp( mode );
		return ( wpath.empty() || wmode.empty() ) ? nullptr : _wfopen( wpath.c_str(), wmode.c_str() );
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

inline std::filesystem::path resolve_symlink_chain_checked( std::filesystem::path cur, bool& loop_detected )
{
	loop_detected = false;
	std::error_code ec;
	for ( int depth = 0; depth < 32; ++depth ) {
		const auto st = std::filesystem::symlink_status( cur, ec );
		if ( ec || !std::filesystem::is_symlink( st ) ) {
			return cur;
		}
		const std::filesystem::path target = std::filesystem::read_symlink( cur, ec );
		if ( ec || target.empty() ) {
			return cur;
		}
		cur = target.is_absolute() ? target : ( cur.parent_path() / target );
	}
	const auto final_st = std::filesystem::symlink_status( cur, ec );
	loop_detected = !ec && std::filesystem::is_symlink( final_st );
	return cur;
}

inline std::filesystem::path resolve_symlink_chain( std::filesystem::path cur )
{
	bool loop_detected = false;
	return resolve_symlink_chain_checked( std::move( cur ), loop_detected );
}

inline bool resolve_overwrite_target_utf8(
	const std::string& raw_target_utf8,
	std::string& effective_target_utf8,
	std::string& err,
	const std::string* display_name = nullptr ) noexcept
{
	const std::string& shown = ( display_name != nullptr && !display_name->empty() ) ? *display_name : raw_target_utf8;
	if ( raw_target_utf8.empty() ) {
		err = "empty output filename";
		return false;
	}
	try {
		const std::filesystem::path p = path_from_utf8( raw_target_utf8 );
		std::error_code ec;
		if ( std::filesystem::is_symlink( std::filesystem::symlink_status( p, ec ) ) && !ec ) {
			bool loop_detected = false;
			const std::filesystem::path resolved = resolve_symlink_chain_checked( p, loop_detected );
			if ( loop_detected ) {
				err = "Unable to open " + shown + " for writing (symbolic link loop).";
				return false;
			}
			effective_target_utf8 = path_to_utf8( resolved );
			return !effective_target_utf8.empty();
		}
		effective_target_utf8 = raw_target_utf8;
		return true;
	} catch ( ... ) {
		err = "Unable to open " + shown + " for writing.";
		return false;
	}
}

inline std::string fold_filename_for_platform( const std::filesystem::path& leaf )
{
#if defined(_WIN32) || defined(WIN32)
	std::wstring wleaf = leaf.wstring();
	lowercase_wide_inplace( wleaf );
	return narrow_utf8_from_wide( wleaf );
#elif defined(__APPLE__)
	std::string s = leaf.string();
	for ( char& c : s ) c = static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );
	return s;
#else
	return leaf.string();
#endif
}

inline bool paths_refer_to_same_target_location( const std::string& a, const std::string& b ) noexcept
{
	if ( a.empty() || b.empty() ) {
		return false;
	}
	if ( paths_refer_to_same_existing_file( a, b ) ) {
		return true;
	}
	try {
		const std::filesystem::path pa = resolve_symlink_chain( path_from_utf8( a ) );
		const std::filesystem::path pb = resolve_symlink_chain( path_from_utf8( b ) );
		const std::filesystem::path dir_a = pa.parent_path().empty() ? std::filesystem::path( "." ) : pa.parent_path();
		const std::filesystem::path dir_b = pb.parent_path().empty() ? std::filesystem::path( "." ) : pb.parent_path();
		std::error_code ec;
		return std::filesystem::equivalent( dir_a, dir_b, ec ) && !ec &&
		       ( fold_filename_for_platform( pa.filename() ) == fold_filename_for_platform( pb.filename() ) );
	} catch ( ... ) {
		return false;
	}
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
		if ( wc == L'/' ) wc = L'\\';
	}
	lowercase_wide_inplace( wkey );
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
	const std::string leaf = ( extension != nullptr ) ? ( stem + "." + extension ) : stem;
	const std::filesystem::path leaf_p = path_from_utf8( leaf );
	return path_to_utf8( dir.empty() ? leaf_p : ( dir / leaf_p ) );
}

inline std::string append_stem_underscore( const std::string& path_utf8 )
{
	const std::filesystem::path p = path_from_utf8( path_utf8 );
	const std::filesystem::path dir = p.parent_path();
	const std::filesystem::path leaf = path_from_utf8( path_to_utf8( p.stem() ) + "_" + path_to_utf8( p.extension() ) );
	return path_to_utf8( dir.empty() ? leaf : ( dir / leaf ) );
}

inline std::string make_unique_filename( const std::string& base, const char* extension )
{
	const std::filesystem::path p = path_from_utf8( base );
	const std::filesystem::path dir = p.parent_path();
	std::string stem = path_to_utf8( p.stem() );
	const std::string suffix = ( extension != nullptr ) ? ( std::string( "." ) + extension ) : std::string();
	for ( unsigned int i = 0; i < 4096; ++i ) {
		const std::filesystem::path leaf = path_from_utf8( stem + suffix );
		const std::string candidate = path_to_utf8( dir.empty() ? leaf : ( dir / leaf ) );
		if ( !file_exists( candidate ) ) {
			return candidate;
		}
		stem.push_back( '_' );
	}
	const std::filesystem::path leaf = path_from_utf8( stem + suffix );
	return path_to_utf8( dir.empty() ? leaf : ( dir / leaf ) );
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
		if ( !rel_parent.empty() && ( rel_parent != "." ) && ( path_to_utf8( *rel_parent.begin() ) != ".." ) ) {
			dest_dir /= rel_parent;
		}
	}
	return path_to_utf8( dest_dir / in_path.filename() );
}

inline bool is_non_regular_stream_target( const std::string& target_utf8 ) noexcept
{
	if ( target_utf8.empty() ) {
		return false;
	}
	try {
#if defined(_WIN32) || defined(WIN32)
		const std::wstring wp = widen_utf8_or_acp( target_utf8 );
		if ( wp.rfind( L"\\\\.\\", 0 ) == 0 ) {
			return true;
		}
		const std::filesystem::path p( wp );
		if ( p.parent_path().empty() ) {
			std::string stem = path_to_utf8( p.stem() );
			for ( char& c : stem ) c = static_cast<char>( std::toupper( static_cast<unsigned char>( c ) ) );
			if ( stem == "NUL" || stem == "CON" || stem == "PRN" || stem == "AUX" ) {
				return true;
			}
			if ( stem.size() == 4 &&
			     ( stem.rfind( "COM", 0 ) == 0 || stem.rfind( "LPT", 0 ) == 0 ) &&
			     ( stem[ 3 ] >= '1' && stem[ 3 ] <= '9' ) ) {
				return true;
			}
		}
		return false;
#else
		std::error_code ec;
		const auto st = std::filesystem::status( path_from_utf8( target_utf8 ), ec );
		return !ec && (
			st.type() == std::filesystem::file_type::character ||
			st.type() == std::filesystem::file_type::fifo ||
			st.type() == std::filesystem::file_type::socket );
#endif
	} catch ( ... ) {
		return false;
	}
}

inline bool query_existing_regular_file_mode( const std::string& target_utf8, unsigned int& mode_out ) noexcept
{
	mode_out = 0666u;
#if defined(_WIN32) || defined(WIN32)
	(void)target_utf8;
	return false;
#else
	struct stat st{};
	if ( !target_utf8.empty() && ( ::stat( target_utf8.c_str(), &st ) == 0 ) && S_ISREG( st.st_mode ) ) {
		mode_out = static_cast<unsigned int>( st.st_mode & 07777 );
		return true;
	}
	return false;
#endif
}

inline bool ensure_parent_directory( const std::string& file_path ) noexcept
{
	try {
		if ( is_non_regular_stream_target( file_path ) ) {
			return true;
		}
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

inline bool can_overwrite_or_create_target(
	const std::string& target_utf8,
	std::string& err,
	const std::string* display_name = nullptr ) noexcept
{
	const std::string& shown = ( display_name != nullptr && !display_name->empty() ) ? *display_name : target_utf8;
	if ( target_utf8.empty() ) {
		err = "empty output filename";
		return false;
	}
	try {
		std::string effective_target;
		if ( !resolve_overwrite_target_utf8( target_utf8, effective_target, err ) ) {
			return false;
		}
		if ( is_non_regular_stream_target( effective_target ) ) {
#if !defined(_WIN32) && !defined(WIN32)
			if ( ::access( effective_target.c_str(), W_OK ) != 0 ) {
				err = "Unable to open " + shown + " for writing.";
				return false;
			}
#endif
			return true;
		}

		const std::filesystem::path p = path_from_utf8( effective_target );
		std::error_code ec;
		if ( std::filesystem::is_directory( p, ec ) ) {
			err = "Unable to open " + shown + " for writing (target is a directory).";
			return false;
		}
		ec.clear();
		if ( std::filesystem::exists( p, ec ) ) {
#if defined(_WIN32) || defined(WIN32)
			const std::wstring wp = widen_win32_path( effective_target );
			const DWORD attrs = GetFileAttributesW( wp.c_str() );
			if ( ( attrs != INVALID_FILE_ATTRIBUTES ) && ( ( attrs & FILE_ATTRIBUTE_READONLY ) != 0 ) ) {
				err = "Unable to open " + shown + " for writing.";
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
				err = "Unable to open " + shown + " for writing.";
				return false;
			}
			CloseHandle( h );
#else
			if ( ::access( effective_target.c_str(), W_OK ) != 0 ) {
				err = "Unable to open " + shown + " for writing.";
				return false;
			}
#endif
		}
		return true;
	} catch ( ... ) {
		err = "Unable to open " + shown + " for writing.";
		return false;
	}
}

inline std::uint64_t next_staging_token() noexcept
{
	static std::atomic<std::uint64_t> seq{ 0 };
	const std::uint64_t c = seq.fetch_add( 1, std::memory_order_relaxed ) + 1;
	const auto now_ns = static_cast<std::uint64_t>(
		std::chrono::steady_clock::now().time_since_epoch().count() );
	const auto tid_h = static_cast<std::uint64_t>(
		std::hash<std::thread::id>{}( std::this_thread::get_id() ) );
	std::uint64_t z = c * 0x9E3779B97F4A7C15ULL ^ now_ns ^ ( tid_h << 1 );
	z = ( z ^ ( z >> 30 ) ) * 0xBF58476D1CE4E5B9ULL;
	z = ( z ^ ( z >> 27 ) ) * 0x94D049BB133111EBULL;
	return z ^ ( z >> 31 );
}

inline FILE* open_exclusive_file_utf8(
	const std::string& path_utf8,
	bool has_existing_mode = false,
	unsigned int existing_mode = 0666u,
	bool* out_already_exists = nullptr ) noexcept
{
	if ( out_already_exists != nullptr ) {
		*out_already_exists = false;
	}
	if ( path_utf8.empty() ) {
		return nullptr;
	}
	try {
#if defined(_WIN32) || defined(WIN32)
		(void)has_existing_mode;
		(void)existing_mode;
		const std::wstring wpath = widen_win32_path( path_utf8 );
		if ( wpath.empty() ) {
			return nullptr;
		}
		HANDLE h = CreateFileW(
			wpath.c_str(),
			GENERIC_WRITE,
			FILE_SHARE_READ,
			nullptr,
			CREATE_NEW,
			FILE_ATTRIBUTE_NORMAL,
			nullptr );
		if ( h == INVALID_HANDLE_VALUE ) {
			const DWORD gle = GetLastError();
			if ( ( out_already_exists != nullptr ) &&
			     ( gle == ERROR_FILE_EXISTS || gle == ERROR_ALREADY_EXISTS ) ) {
				*out_already_exists = true;
			}
			return nullptr;
		}
		const int fd = _open_osfhandle( reinterpret_cast<intptr_t>( h ), _O_WRONLY | _O_BINARY );
		if ( fd == -1 ) {
			CloseHandle( h );
			DeleteFileW( wpath.c_str() );
			return nullptr;
		}
		FILE* fp = _fdopen( fd, "wb" );
		if ( fp == nullptr ) {
			_close( fd );
			DeleteFileW( wpath.c_str() );
			return nullptr;
		}
		return fp;
#else
		const mode_t init_mode = has_existing_mode
			? static_cast<mode_t>( existing_mode & 0600u )
			: static_cast<mode_t>( 0666 );
		int flags = O_WRONLY | O_CREAT | O_EXCL;
#ifdef O_CLOEXEC
		flags |= O_CLOEXEC;
#endif
		const int fd = ::open( path_utf8.c_str(), flags, init_mode );
		if ( fd < 0 ) {
			if ( ( out_already_exists != nullptr ) && ( errno == EEXIST ) ) {
				*out_already_exists = true;
			}
			return nullptr;
		}
		if ( has_existing_mode ) {
			(void)::fchmod( fd, static_cast<mode_t>( existing_mode & 07777u ) );
		}
		FILE* fp = ::fdopen( fd, "wb" );
		if ( fp == nullptr ) {
			::close( fd );
			::unlink( path_utf8.c_str() );
			return nullptr;
		}
		return fp;
#endif
	} catch ( ... ) {
		return nullptr;
	}
}

inline FILE* open_exclusive_staging_stream(
	const std::string& effective_target_utf8,
	bool has_existing_mode,
	unsigned int existing_mode,
	std::string& out_staging_utf8 )
{
	out_staging_utf8.clear();
	const std::filesystem::path dir = path_from_utf8( effective_target_utf8 ).parent_path();
#if defined(_WIN32) || defined(WIN32)
	const unsigned long pid = static_cast<unsigned long>( GetCurrentProcessId() );
#else
	const unsigned long pid = static_cast<unsigned long>( ::getpid() );
#endif
	char leaf[ 64 ];
	for ( unsigned int attempt = 0; attempt < 4096; ++attempt ) {
		std::snprintf(
			leaf, sizeof( leaf ), ".packjpg_tmp_%lu_%016llx.part",
			pid, static_cast<unsigned long long>( next_staging_token() ) );
		const std::filesystem::path cand = dir.empty()
			? path_from_utf8( leaf )
			: ( dir / path_from_utf8( leaf ) );
		const std::string cand_utf8 = path_to_utf8( cand );
		bool existed = false;
		FILE* fp = open_exclusive_file_utf8( cand_utf8, has_existing_mode, existing_mode, &existed );
		if ( fp != nullptr ) {
			out_staging_utf8 = cand_utf8;
			return fp;
		}
		if ( !existed ) {
			return nullptr;
		}
	}
	return nullptr;
}

inline FILE* open_target_or_staging_cstream(
	const std::string& effective_target_utf8,
	bool overwrite,
	const char* direct_mode,
	std::string& out_staging_utf8 )
{
	out_staging_utf8.clear();
	if ( overwrite && is_non_regular_stream_target( effective_target_utf8 ) ) {
		return fopen_utf8( effective_target_utf8, direct_mode );
	}
	unsigned int existing_mode = 0666u;
	const bool has_mode = overwrite && query_existing_regular_file_mode( effective_target_utf8, existing_mode );
	FILE* fp = open_exclusive_staging_stream( effective_target_utf8, has_mode, existing_mode, out_staging_utf8 );
#if defined(_WIN32) || defined(WIN32)
	if ( ( fp != nullptr ) && ( direct_mode != nullptr ) && ( direct_mode[ 0 ] == 'w' ) && ( direct_mode[ 1 ] == '\0' ) ) {
		(void)_setmode( _fileno( fp ), _O_TEXT );
	}
#endif
	return fp;
}

enum class CommitStatus {
	Ok,
	TargetExists,
	Error
};

inline CommitStatus commit_staging_file(
	const std::string& staging_utf8,
	const std::string& target_utf8,
	bool allow_replace,
	std::string& err ) noexcept
{
	try {
#if defined(_WIN32) || defined(WIN32)
		const std::wstring wsrc = widen_win32_path( staging_utf8 );
		const std::wstring wdst = widen_win32_path( target_utf8 );
		if ( wsrc.empty() || wdst.empty() ) {
			err = "failed to commit output file " + target_utf8;
			return CommitStatus::Error;
		}
		if ( !allow_replace ) {
			if ( MoveFileExW( wsrc.c_str(), wdst.c_str(), 0 ) ) {
				return CommitStatus::Ok;
			}
			const DWORD gle = GetLastError();
			if ( gle == ERROR_ALREADY_EXISTS || gle == ERROR_FILE_EXISTS ) {
				return CommitStatus::TargetExists;
			}
			err = "failed to commit output file " + target_utf8;
			return CommitStatus::Error;
		}

		const DWORD dst_attrs = GetFileAttributesW( wdst.c_str() );
		if ( ( dst_attrs != INVALID_FILE_ATTRIBUTES ) && ( ( dst_attrs & FILE_ATTRIBUTE_DIRECTORY ) == 0 ) ) {
			if ( ReplaceFileW(
			         wdst.c_str(),
			         wsrc.c_str(),
			         nullptr,
			         REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS,
			         nullptr,
			         nullptr ) ) {
				return CommitStatus::Ok;
			}
			if ( ( dst_attrs & ( FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM ) ) != 0 ) {
				SetFileAttributesW( wdst.c_str(), dst_attrs & ~( FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM ) );
				const BOOL moved = MoveFileExW( wsrc.c_str(), wdst.c_str(), MOVEFILE_REPLACE_EXISTING );
				SetFileAttributesW( wdst.c_str(), dst_attrs );
				if ( moved ) {
					return CommitStatus::Ok;
				}
			}
		}

		if ( !MoveFileExW( wsrc.c_str(), wdst.c_str(), MOVEFILE_REPLACE_EXISTING ) ) {
			err = "failed to commit output file " + target_utf8;
			return CommitStatus::Error;
		}
		return CommitStatus::Ok;
#else
		if ( !allow_replace ) {
#if defined(__linux__) && defined(SYS_renameat2) && defined(AT_FDCWD)
			if ( ::syscall( SYS_renameat2, AT_FDCWD, staging_utf8.c_str(), AT_FDCWD, target_utf8.c_str(), 1u ) == 0 ) {
				return CommitStatus::Ok;
			}
			if ( errno == EEXIST ) {
				return CommitStatus::TargetExists;
			}
			if ( errno != ENOSYS && errno != EINVAL && errno != EOPNOTSUPP ) {
				err = "failed to commit output file " + target_utf8;
				return CommitStatus::Error;
			}
#elif defined(__APPLE__) && defined(RENAME_EXCL)
			if ( ::renamex_np( staging_utf8.c_str(), target_utf8.c_str(), RENAME_EXCL ) == 0 ) {
				return CommitStatus::Ok;
			}
			if ( errno == EEXIST ) {
				return CommitStatus::TargetExists;
			}
			if ( errno != ENOTSUP && errno != ENOSYS && errno != EINVAL ) {
				err = "failed to commit output file " + target_utf8;
				return CommitStatus::Error;
			}
#endif
			const std::filesystem::path dst_p = path_from_utf8( target_utf8 );
			if ( path_or_symlink_exists( dst_p ) ) {
				return CommitStatus::TargetExists;
			}
			if ( ::link( staging_utf8.c_str(), target_utf8.c_str() ) == 0 ) {
				(void)::unlink( staging_utf8.c_str() );
				return CommitStatus::Ok;
			}
			if ( errno == EEXIST ) {
				return CommitStatus::TargetExists;
			}
			if ( errno != EPERM && errno != EOPNOTSUPP && errno != ENOSYS ) {
				err = "failed to commit output file " + target_utf8;
				return CommitStatus::Error;
			}
			if ( path_or_symlink_exists( dst_p ) ) {
				return CommitStatus::TargetExists;
			}
		}

		std::error_code ec;
		std::filesystem::rename( path_from_utf8( staging_utf8 ), path_from_utf8( target_utf8 ), ec );
		if ( ec ) {
			err = "failed to commit output file " + target_utf8 + ": " + ec.message();
			return CommitStatus::Error;
		}
		return CommitStatus::Ok;
#endif
	} catch ( ... ) {
		err = "failed to commit output file " + target_utf8;
		return CommitStatus::Error;
	}
}

inline bool commit_staging_file( const std::string& staging_utf8, const std::string& target_utf8, std::string& err ) noexcept
{
	return commit_staging_file( staging_utf8, target_utf8, true, err ) == CommitStatus::Ok;
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

inline std::size_t utf8_next_char_len( std::string_view s, std::size_t pos ) noexcept
{
	if ( pos >= s.size() ) {
		return 0;
	}
	const auto b0 = static_cast<unsigned char>( s[ pos ] );
	if ( b0 < 0x80u ) {
		return 1;
	}
	const std::size_t len = ( ( b0 & 0xE0u ) == 0xC0u ) ? 2u
	                      : ( ( b0 & 0xF0u ) == 0xE0u ) ? 3u
	                      : ( ( b0 & 0xF8u ) == 0xF0u ) ? 4u : 1u;
	if ( ( len == 1u ) || ( pos + len > s.size() ) ) {
		return 1;
	}
	for ( std::size_t k = 1; k < len; ++k ) {
		if ( ( static_cast<unsigned char>( s[ pos + k ] ) & 0xC0u ) != 0x80u ) {
			return 1;
		}
	}
	return len;
}

inline bool wildcard_match( std::string_view pattern, std::string_view text, bool case_insensitive ) noexcept
{
	if ( !text.empty() && ( text.front() == '.' ) && ( pattern.empty() || ( pattern.front() != '.' ) ) ) {
		return false;
	}

	std::size_t pi = 0;
	std::size_t ti = 0;
	std::size_t star_pi  = std::string_view::npos;
	std::size_t match_ti = 0;

	while ( ti < text.size() ) {
		const std::size_t tlen = utf8_next_char_len( text, ti );
		if ( ( pi < pattern.size() ) && ( pattern[ pi ] == '?' ) ) {
			++pi;
			ti += tlen;
		}
		else if ( ( pi < pattern.size() ) && ( pattern[ pi ] == '*' ) ) {
			star_pi  = pi++;
			match_ti = ti;
		}
		else {
			const std::size_t plen = ( pi < pattern.size() ) ? utf8_next_char_len( pattern, pi ) : 0u;
			const bool eq = ( plen > 0u ) && ( ( plen == 1u && tlen == 1u )
				? ( ascii_fold( pattern[ pi ], case_insensitive ) == ascii_fold( text[ ti ], case_insensitive ) )
				: ( pattern.substr( pi, plen ) == text.substr( ti, tlen ) ) );
			if ( eq ) {
				pi += plen;
				ti += tlen;
			}
			else if ( star_pi != std::string_view::npos ) {
				pi = star_pi + 1;
				match_ti += utf8_next_char_len( text, match_ti );
				ti = match_ti;
			}
			else {
				return false;
			}
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
			return paths_refer_to_same_existing_file( s, out_dir ) ||
			       ( !norm_out_dir.empty() && ( normalize_path_key( s ) == norm_out_dir ) );
		} catch ( ... ) {
			return false;
		}
	};

	auto walk_directory_tree = [&](
		const std::filesystem::path& root_dir,
		bool recurse_subdirs,
		bool allow_dot_dirs,
		auto&& accept_regular_file,
		std::vector<std::string>& out_files ) {
		std::vector<std::filesystem::path> stack{ root_dir };
		const auto opts = std::filesystem::directory_options::skip_permission_denied;
		while ( !stack.empty() ) {
			const std::filesystem::path cur_dir = std::move( stack.back() );
			stack.pop_back();

			std::error_code dir_ec;
			std::filesystem::directory_iterator it( cur_dir, opts, dir_ec ), end;
			if ( dir_ec ) {
				continue;
			}
			while ( it != end ) {
				std::error_code entry_ec;
				const std::filesystem::path entry_path = it->path();
				const std::string entry_name = path_to_utf8( entry_path.filename() );
				const bool is_link = it->is_symlink( entry_ec );
				entry_ec.clear();
				if ( recurse_subdirs && !is_link && it->is_directory( entry_ec ) ) {
					if ( ( allow_dot_dirs || entry_name.empty() || ( entry_name.front() != '.' ) ) &&
					     !is_excluded_out_subdir( entry_path ) ) {
						stack.push_back( entry_path );
					}
				}
				else if ( ( entry_ec.clear(), it->is_regular_file( entry_ec ) ) &&
				          accept_regular_file( entry_path, entry_name ) ) {
					out_files.push_back( path_to_utf8( entry_path ) );
				}
				dir_ec.clear();
				it.increment( dir_ec );
				if ( dir_ec ) {
					break;
				}
			}
		}
	};

	for ( const std::string& raw : raw_inputs ) {
		if ( raw == "-" ) {
			expanded.push_back( { "-", {} } );
			continue;
		}

		std::error_code ec;
		const std::filesystem::path p = path_from_utf8( raw );
		if ( std::filesystem::exists( p, ec ) ) {
			if ( recursive && std::filesystem::is_directory( p, ec ) ) {
				had_directory_or_glob = true;
				std::vector<std::string> found;
				walk_directory_tree(
					p, true, false,
					[]( const std::filesystem::path& ep, const std::string& ) { return is_supported_codec_file( ep ); },
					found );
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
				walk_directory_tree(
					search_dir,
					recursive,
					!fname.empty() && ( fname.front() == '.' ),
					[&]( const std::filesystem::path&, const std::string& entry_name ) {
						return wildcard_match( fname, entry_name, kCaseInsensitiveGlob );
					},
					matches );
				if ( !recursive && parent.empty() ) {
					for ( std::string& m : matches ) {
						m = path_to_utf8( path_from_utf8( m ).filename() );
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
