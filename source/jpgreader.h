#ifndef JPGREADER_H
#define JPGREADER_H

#include <cstdint>
#include <memory>
#include <vector>

#include "bitops.h"

class JpgReader {
public:
	// Read in header and image data.
	void read(const std::unique_ptr<Reader>& str_in);

private:
	void read_sos(const std::unique_ptr<Reader>& str_in, const std::unique_ptr<MemoryWriter>& huffw, std::vector<std::uint8_t>& segment);

	int scan_count_ = 0; // Count of scans.
};

#endif