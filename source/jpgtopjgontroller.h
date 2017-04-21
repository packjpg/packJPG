#ifndef JPGTOPJGCONTROLLER
#define JPGTOPJGCONTROLLER
#include "reader.h"
#include "writer.h"

class JpgToPjgController
{
public:
	JpgToPjgController(Reader& jpg_input, Writer& pjg_output);
	~JpgToPjgController();

	void execute();

private:
	Reader& jpg_input_;
	Writer& pjg_output_;
};

#endif