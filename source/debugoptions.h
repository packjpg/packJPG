#ifndef DEBUGOPTIONS_H
#define DEBUGOPTIONS_H

class DebugOptions {
public:
	int collmode = 0; // write mode for collections: 0 -> std, 1 -> dhf, 2 -> squ, 3 -> unc
	bool coll_dump = false;
	bool fcoll_dump = false;
	bool split_dump = false;
	bool zdst_dump = false;
	bool txt_info = false;
	bool dist_info = false;
	bool pgm_dump = false;
};

#endif