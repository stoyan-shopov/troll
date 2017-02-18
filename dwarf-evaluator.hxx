#ifndef DWARFEXPRESSION_HXX
#define DWARFEXPRESSION_HXX

#include "sforth.hxx"
#include "util.hxx"

class DwarfEvaluator
{
private:
	Sforth	* sforh;
public:
	DwarfEvaluator(class Sforth * sforth);
};

#endif // DWARFEXPRESSION_HXX
