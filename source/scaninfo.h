#ifndef SCANINFO_H
#define SCANINFO_H

#include <vector>

// Information about the current SOS scan.
class ScanInfo {
public:
	std::vector<int> cmp; // Component numbers in the current scan.
	int from = 0; // begin - band of current scan ( inclusive )
	int to = 0; // end - band of current scan ( inclusive )
	int sah = 0; // successive approximation bit pos high
	int sal = 0; // successive approximation bit pos low
};

#endif