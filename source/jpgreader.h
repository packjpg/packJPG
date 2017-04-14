#ifndef JPGREADER_H
#define JPGREADER_H

#include <cstdint>
#include <memory>
#include <vector>

#include "bitops.h"

class JpgReader {
public:
	// Read in header and image data.
	void read();

private:
	void read_sos(const std::unique_ptr<abytewriter>& huffw, std::vector<std::uint8_t>& segment);
};

#endif