#ifndef CORTEXM0_H
#define CORTEXM0_H

#include "sforth.hxx"

class CortexM0
{
private:
	Sforth	* sforth;
public:
	CortexM0(Sforth * sforth);
};

#endif // CORTEXM0_H
