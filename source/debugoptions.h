#ifndef DEBUGOPTIONS_H
#define DEBUGOPTIONS_H
#include "collectionmode.h"

class DebugOptions {
public:
	CollectionMode collmode = CollectionMode::STANDARD;
	bool coll_dump = false;
	bool fcoll_dump = false;
	bool split_dump = false;
	bool zdst_dump = false;
	bool txt_info = false;
	bool dist_info = false;
	bool pgm_dump = false;
};

#endif