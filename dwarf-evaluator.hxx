#ifndef DWARFEXPRESSION_HXX
#define DWARFEXPRESSION_HXX

#include "sforth.hxx"
#include "util.hxx"

class DwarfEvaluator
{
private:
	Sforth	* sforth;
public:
	DwarfEvaluator(class Sforth * sforth);
	void evaluateLocation(uint32_t cfa_value, const QString & frameBaseSforthCode, const QString & locationSforthCode);
};

#endif // DWARFEXPRESSION_HXX
