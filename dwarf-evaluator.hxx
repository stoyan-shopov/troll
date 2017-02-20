#ifndef DWARFEXPRESSION_HXX
#define DWARFEXPRESSION_HXX

#include "sforth.hxx"
#include "util.hxx"

class DwarfEvaluator
{
private:
	Sforth	* sforth;
public:
	/*! \warning	these constants must match the dwarf expression type constants in file 'dwarf-evaluator.fs' */
	enum DwarfExpressionType
	{
		INVALID		= 0,
		CONSTANT	= 1,
		MEMORY_ADDRESS	= 2,
		REGISTER_NUMBER	= 3,
	};
	struct DwarfExpressionValue
	{
		uint32_t			value;
		enum DwarfExpressionType	type;
	};
	DwarfEvaluator(class Sforth * sforth);
	struct DwarfExpressionValue evaluateLocation(uint32_t cfa_value, const QString & frameBaseSforthCode, const QString & locationSforthCode);
};

#endif // DWARFEXPRESSION_HXX
