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

inline bool paths_refer_to_same_existing_file( const std::string& a, const std::string& b ) noexcept
{
	if ( a.empty() || b.empty() ) {
		return false;
	}
	std::error_code ec;
	const bool same = std::filesystem::equivalent( std::filesystem::path( a ), std::filesystem::path( b ), ec );
	return !ec && same;
}

inline std::string normalize_path_key( const std::string& raw )
{
	if ( raw.empty() ) {
		return {};
	}
	std::error_code ec;
	std::filesystem::path norm = std::filesystem::weakly_canonical( std::filesystem::path( raw ), ec );
	if ( ec ) {
		ec.clear();
		norm = std::filesystem::absolute( std::filesystem::path( raw ), ec ).lexically_normal();
		if ( ec ) {
			norm = std::filesystem::path( raw ).lexically_normal();
		}
	}
	std::string key = norm.string();
#if defined(_WIN32) || defined(WIN32)
	for ( char& c : key ) {
		if ( c == '/' ) c = '\\';
		c = static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );
	}
#endif
	return key;
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
	const std::string fname = p.filename().string();
	if ( fname.empty() || ( fname.front() == '.' ) ) {
		return false;
	}
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
		const std::string s = dir_path.string();
		if ( paths_refer_to_same_existing_file( s, out_dir ) ) {
			return true;
		}
		return !norm_out_dir.empty() && ( normalize_path_key( s ) == norm_out_dir );
	};

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
				std::filesystem::recursive_directory_iterator it( p, opts, ec ), end;
				if ( !ec ) {
					while ( it != end ) {
						std::error_code entry_ec;
						const std::filesystem::path entry_path = it->path();
						const std::string entry_name = entry_path.filename().string();
						if ( it->is_directory( entry_ec ) ) {
							if ( ( !entry_name.empty() && entry_name.front() == '.' ) ||
							     is_excluded_out_subdir( entry_path ) ) {
								it.disable_recursion_pending();
							}
						}
						else if ( it->is_regular_file( entry_ec ) && is_supported_codec_file( entry_path ) ) {
							found.push_back( entry_path.string() );
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

		const std::string fname = p.filename().string();
		const std::filesystem::path parent = p.parent_path();
		if ( has_wildcard_chars( fname ) && !has_wildcard_chars( parent.string() ) ) {
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
							const std::string entry_name = entry_path.filename().string();
							if ( it->is_directory( entry_ec ) ) {
								if ( ( !entry_name.empty() && ( entry_name.front() == '.' ) && ( fname.front() != '.' ) ) ||
								     is_excluded_out_subdir( entry_path ) ) {
									it.disable_recursion_pending();
								}
							}
							else if ( it->is_regular_file( entry_ec ) &&
							          wildcard_match( fname, entry_name, kCaseInsensitiveGlob ) ) {
								matches.push_back( entry_path.string() );
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
							const std::string entry_name = it->path().filename().string();
							if ( it->is_regular_file( entry_ec ) &&
							     wildcard_match( fname, entry_name, kCaseInsensitiveGlob ) ) {
								const std::filesystem::path rel = parent.empty()
									? it->path().filename()
									: ( parent / it->path().filename() );
								matches.push_back( rel.string() );
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

