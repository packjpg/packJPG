#ifndef JPG_H
#define JPG_H

#include <memory>

#include "codingstatus.h"
#include "component.h"
#include "frameinfo.h"
#include "scaninfo.h"

namespace jpg {

// increment all counts where needed
inline CodingStatus increment_counts(const FrameInfo& frame_info, const ScanInfo& scan_info, int rsti, int& mcu, int& component, int& csc, int& sub, int& rstw) {
	sub++;
	if (sub >= frame_info.components[component].mbs) {
		sub = 0;
		csc++;
		if (csc >= scan_info.cmpc) {
			csc = 0;
			component = scan_info.cmp[0];
			mcu++;
			if (mcu >= frame_info.mcu_count) {
				return CodingStatus::DONE;
			} else if (rsti > 0) {
				rstw--;
				if (rstw == 0) {
					return CodingStatus::RESTART;
				}
			}
		} else {
			component = scan_info.cmp[csc];
		}
	}
	return CodingStatus::OKAY;
}

// Calculates next position for MCU.
inline int next_mcupos(const FrameInfo& frame_info, int mcu, int cmp, int sub) {
	// get correct position in image ( x & y )
	int dpos;
	if (frame_info.components[cmp].sfh > 1) { // to fix mcu order
		dpos = (mcu / frame_info.mcu_width) * frame_info.components[cmp].sfh + (sub / frame_info.components[cmp].sfv);
		dpos *= frame_info.components[cmp].bch;
		dpos += (mcu % frame_info.mcu_width) * frame_info.components[cmp].sfv + (sub % frame_info.components[cmp].sfv);
	} else if (frame_info.components[cmp].sfv > 1) {
		// simple calculation to speed up things if simple fixing is enough
		dpos = (mcu * frame_info.components[cmp].mbs) + sub;
	} else {
		// no calculations needed without subsampling
		dpos = mcu;
	}
	return dpos;
}

// Calculates next position (non interleaved).
inline CodingStatus next_mcuposn(const Component& component, int rsti, int& dpos, int& rstw) {
	// increment position
	dpos++;

	// fix for non interleaved mcu - horizontal
	if (component.bch != component.nch) {
		if (dpos % component.bch == component.nch) {
			dpos += (component.bch - component.nch);
		}
	}

	// fix for non interleaved mcu - vertical
	if (component.bcv != component.ncv) {
		if (dpos / component.bch == component.ncv) {
			dpos = component.bc;
		}
	}

	// check position
	if (dpos >= component.bc) {
		return CodingStatus::DONE;
	} else if (rsti > 0) {
		if (--rstw == 0) {
			return CodingStatus::RESTART;
		}
	}

	return CodingStatus::OKAY;
}
}

#endif
