#ifndef JFIF_H
#define JFIF_H

#include "component.h"
#include "frameinfo.h"
#include "scaninfo.h"

namespace jfif {
	// Increments all counts where needed.
	CodingStatus increment_counts(const FrameInfo& frame_info, const ScanInfo& scan_info, int rsti, int& mcu, int& component, int& csc, int& sub, int& rstw);
}

#endif