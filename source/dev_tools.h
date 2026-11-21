#ifndef DEV_TOOLS_H
#define DEV_TOOLS_H

#include "codec_pipeline.h"

#include <string>

void run_dev_action( CodecContext& ctx, const std::string& input_name, int collmode, const StageReporter* reporter );
bool dump_errfile( CodecContext& ctx, const std::string& input_name );

#endif
