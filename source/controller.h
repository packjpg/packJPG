#ifndef CONTROLLER_H
#define CONTROLLER_H

class Controller {
public:
	virtual ~Controller() = default;
	virtual void execute() = 0;
};

#endif