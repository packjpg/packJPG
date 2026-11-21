#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <filesystem>
#include <string>
#include <system_error>

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

#endif
