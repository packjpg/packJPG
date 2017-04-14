#ifndef ACTION_H
#define ACTION_H

enum class Action {
	A_COMPRESS = 1,
	A_SPLIT_DUMP = 2,
	A_COLL_DUMP = 3,
	A_FCOLL_DUMP = 4,
	A_ZDST_DUMP = 5,
	A_TXT_INFO = 6,
	A_DIST_INFO = 7,
	A_PGM_DUMP = 8
};

#endif