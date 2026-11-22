#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

struct CliInputItem {
	std::string path;
	std::string rel_root;
};

inline bool file_exists( const std::string& filename ) noexcept
{
	if ( filename.empty() ) {
		return false;
	}
	std::error_code ec;
	return std::filesystem::exists( std::filesystem::path( filename ), ec );
}

inline std::string make_filename( const std::string& base, const char* extension )
{
	const std::filesystem::path p( base );
	const std::filesystem::path dir = p.parent_path();
	const std::string stem = p.stem().string();
	const std::string leaf = ( extension != nullptr )
		? ( stem + "." + extension )
		: stem;
	return ( dir.empty() ? std::filesystem::path( leaf ) : ( dir / leaf ) ).string();
}

inline std::string make_unique_filename( const std::string& base, const char* extension )
{
	const std::filesystem::path p( base );
	const std::filesystem::path dir = p.parent_path();
	std::string stem = p.stem().string();
	const std::string suffix = ( extension != nullptr )
		? ( std::string( "." ) + extension )
		: std::string();

	auto build = [&]() {
		const std::string leaf = stem + suffix;
		return dir.empty() ? std::filesystem::path( leaf ) : ( dir / leaf );
	};

	std::filesystem::path candidate = build();
	std::error_code ec;
	while ( std::filesystem::exists( candidate, ec ) ) {
		stem.push_back( '_' );
		candidate = build();
	}
	return candidate.string();
}

inline std::string make_output_base(
	const std::string& input_file,
	const std::string& out_dir,
	const std::string& rel_root )
{
	if ( out_dir.empty() ) {
		return input_file;
	}

	const std::filesystem::path in_path( input_file );
	std::filesystem::path dest_dir( out_dir );

	if ( !rel_root.empty() ) {
		const std::filesystem::path rel_parent =
			in_path.parent_path().lexically_relative( std::filesystem::path( rel_root ) );
		if ( !rel_parent.empty() && ( rel_parent != "." ) ) {
			const std::string first_part = rel_parent.begin()->string();
			if ( first_part != ".." ) {
				dest_dir /= rel_parent;
			}
		}
	}

	return ( dest_dir / in_path.filename() ).string();
}

inline bool ensure_parent_directory( const std::string& file_path ) noexcept
{
	const std::filesystem::path parent = std::filesystem::path( file_path ).parent_path();
	if ( parent.empty() ) {
		return true;
	}
	std::error_code ec;
	if ( std::filesystem::is_directory( parent, ec ) ) {
		return true;
	}
	std::filesystem::create_directories( parent, ec );
	return !ec && std::filesystem::is_directory( parent, ec );
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
	std::string ext = p.extension().string();
	for ( char& c : ext ) {
		c = static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );
	}
	return ( ext == ".jpg" ) || ( ext == ".jpeg" ) || ( ext == ".jpe" ) ||
	       ( ext == ".jfif" ) || ( ext == ".pjg" );
}

inline std::vector<CliInputItem> expand_cli_inputs(
	const std::vector<std::string>& raw_inputs,
	bool recursive,
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

	for ( const std::string& raw : raw_inputs ) {
		if ( raw == "-" ) {
			expanded.push_back( { "-", {} } );
			continue;
		}

		std::error_code ec;
		const std::filesystem::path p( raw );
		const bool exists_literally = std::filesystem::exists( p, ec );

		if ( exists_literally ) {
			if ( recursive && std::filesystem::is_directory( p, ec ) ) {
				had_directory_or_glob = true;
				std::vector<std::string> found;
				const auto opts = std::filesystem::directory_options::skip_permission_denied;
				for ( std::filesystem::recursive_directory_iterator it( p, opts, ec ), end;
				      !ec && ( it != end );
				      it.increment( ec ) ) {
					std::error_code entry_ec;
					if ( it->is_regular_file( entry_ec ) && is_supported_codec_file( it->path() ) ) {
						found.push_back( it->path().string() );
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

		const std::string fname = p.filename().string();
		const std::filesystem::path parent = p.parent_path();
		if ( has_wildcard_chars( fname ) && !has_wildcard_chars( parent.string() ) ) {
			const std::filesystem::path search_dir = parent.empty() ? std::filesystem::path( "." ) : parent;
			if ( std::filesystem::is_directory( search_dir, ec ) ) {
				std::vector<std::string> matches;
				const auto opts = std::filesystem::directory_options::skip_permission_denied;
				if ( recursive ) {
					for ( std::filesystem::recursive_directory_iterator it( search_dir, opts, ec ), end;
					      !ec && ( it != end );
					      it.increment( ec ) ) {
						std::error_code entry_ec;
						if ( it->is_regular_file( entry_ec ) &&
						     wildcard_match( fname, it->path().filename().string(), kCaseInsensitiveGlob ) ) {
							matches.push_back( it->path().string() );
						}
					}
				}
				else {
					for ( std::filesystem::directory_iterator it( search_dir, opts, ec ), end;
					      !ec && ( it != end );
					      it.increment( ec ) ) {
						std::error_code entry_ec;
						if ( it->is_regular_file( entry_ec ) &&
						     wildcard_match( fname, it->path().filename().string(), kCaseInsensitiveGlob ) ) {
							const std::filesystem::path rel = parent.empty()
								? it->path().filename()
								: ( parent / it->path().filename() );
							matches.push_back( rel.string() );
						}
					}
				}
				if ( !matches.empty() ) {
					had_directory_or_glob = true;
					std::sort( matches.begin(), matches.end() );
					const std::string root = recursive ? search_dir.string() : std::string();
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
