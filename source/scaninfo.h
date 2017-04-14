#ifndef SCANINFO_H
#define SCANINFO_H

#include <array>

// Information about the current SOS scan.
struct ScanInfo {
	int cmpc = 0; // component count in current scan
	std::array<int, 4> cmp{}; // component numbers  in current scan
	int from = 0; // begin - band of current scan ( inclusive )
	int to = 0; // end - band of current scan ( inclusive )
	int sah = 0; // successive approximation bit pos high
	int sal = 0; // successive approximation bit pos low
};

#endif