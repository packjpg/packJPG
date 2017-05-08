#ifndef PROGRAMINFO_H
#define PROGRAMINFO_H

#include <array>
#include <cstdint>
#include <string>

namespace program_info {
	const std::uint8_t appversion = 25;
	const std::string subversion = "k";
	const std::string apptitle = "packJPG";
	const std::string appname = "packjpg";
	const std::string versiondate = "01/22/2016";
	const std::array<std::uint8_t, 2> pjg_magic{ 'J', 'S' };

	const std::string author = "Matthias Stirner / Se";
	const std::string website = "http://packjpg.encode.ru/";
	const std::string copyright = "2006-2016 HTW Aalen University & Matthias Stirner";
	const std::string email = "packjpg (at) matthiasstirner.com";

	const std::string pjg_ext = "pjg";
	const std::string jpg_ext = "jpg";

	inline void display_program_info(FILE* output_stream) {
		fprintf(output_stream, "\n--> %s v%i.%i%s (%s) by %s <--\n",
		        program_info::apptitle.c_str(),
		        program_info::appversion / 10,
		        program_info::appversion % 10,
		        program_info::subversion.c_str(),
		        program_info::versiondate.c_str(),
		        program_info::author.c_str());
		fprintf(output_stream, "Copyright %s\nAll rights reserved\n\n", program_info::copyright.c_str());
	}
}

#endif