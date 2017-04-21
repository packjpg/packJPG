#ifndef JPG_H
#define JPG_H

#include <memory>

#include "codingstatus.h"
#include "component.h"
#include "frameinfo.h"
#include "scaninfo.h"

namespace jpg {

// Calculates next position for MCU.
inline CodingStatus next_mcupos(const ScanInfo& scan_info, const FrameInfo& frame_info, int rsti, int* mcu, int* cmp, int* csc, int* sub, int* dpos, int* rstw) {
	CodingStatus status = CodingStatus::OKAY;

	// increment all counts where needed
	if ((++(*sub)) >= frame_info.components[(*cmp)].mbs) {
		*sub = 0;

		if ((++(*csc)) >= scan_info.cmpc) {
			*csc = 0;
			*cmp = scan_info.cmp[0];
			(*mcu)++;
			if (*mcu >= frame_info.mcu_count) {
				status = CodingStatus::DONE;
			} else if (rsti > 0) {
				if (--(*rstw) == 0) {
					status = CodingStatus::RESTART;
				}
			}
		} else {
			*cmp = scan_info.cmp[(*csc)];
		}
	}

	// get correct position in image ( x & y )
	if (frame_info.components[(*cmp)].sfh > 1) { // to fix mcu order
		*dpos = ((*mcu) / frame_info.mcu_width) * frame_info.components[(*cmp)].sfh + ((*sub) / frame_info.components[(*cmp)].sfv);
		*dpos *= frame_info.components[(*cmp)].bch;
		*dpos += ((*mcu) % frame_info.mcu_width) * frame_info.components[(*cmp)].sfv + ((*sub) % frame_info.components[(*cmp)].sfv);
	} else if (frame_info.components[(*cmp)].sfv > 1) {
		// simple calculation to speed up things if simple fixing is enough
		*dpos = ((*mcu) * frame_info.components[(*cmp)].mbs) + (*sub);
	} else {
		// no calculations needed without subsampling
		*dpos = *mcu;
	}

	return status;
}

// Calculates next position (non interleaved).
inline CodingStatus next_mcuposn(const Component& cmpt, int rsti, int* dpos, int* rstw) {
	// increment position
	(*dpos)++;

	// fix for non interleaved mcu - horizontal
	if (cmpt.bch != cmpt.nch) {
		if (*dpos % cmpt.bch == cmpt.nch) {
			*dpos += (cmpt.bch - cmpt.nch);
		}
	}

	// fix for non interleaved mcu - vertical
	if (cmpt.bcv != cmpt.ncv) {
		if (*dpos / cmpt.bch == cmpt.ncv) {
			*dpos = cmpt.bc;
		}
	}

	// check position
	if (*dpos >= cmpt.bc) {
		return CodingStatus::DONE;
	} else if (rsti > 0) {
		if (--(*rstw) == 0) {
			return CodingStatus::RESTART;
		}
	}

	return CodingStatus::OKAY;
}
}

#endif
