#ifndef DEV_TOOLS_H
#define DEV_TOOLS_H

#include "codec_pipeline.h"

#include <string>

void run_dev_action( CodecContext& ctx, const std::string& input_name, int collmode, bool overwrite, const StageReporter* reporter );
bool dump_errfile( CodecContext& ctx, const std::string& input_name, bool overwrite );

#endif

