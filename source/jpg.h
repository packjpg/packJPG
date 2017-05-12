#ifndef JPG_H
#define JPG_H

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
		if (csc >= scan_info.cmp.size()) {
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
}

#endif
