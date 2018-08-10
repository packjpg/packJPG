#include "jfif.h"

#include "bitops.h"

CodingStatus jfif::increment_counts(const FrameInfo& frame_info, const ScanInfo& scan_info, const Component& component, int rsti, int& mcu, int& component_id, int& csc, int& sub, int& rstw) {
	sub++;
	if (sub >= component.mbs) {
		sub = 0;
		csc++;
		if (csc >= std::int32_t(scan_info.cmp.size())) {
			csc = 0;
			component_id = scan_info.cmp[0];
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
			component_id = scan_info.cmp[csc];
		}
	}
	return CodingStatus::OKAY;
}
