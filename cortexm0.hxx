#ifndef CORTEXM0_H
#define CORTEXM0_H

#include "sforth.hxx"

class CortexM0
{
private:
	Sforth	* sforth;
	static const int register_count;
	std::vector<uint32_t> registers;
public:
	CortexM0(Sforth * sforth, class Target * target_controller);
	void primeUnwinder(void);
	bool unwindFrame(const QString & unwind_code, uint32_t start_address, uint32_t unwind_address);
	std::vector<uint32_t> unwoundRegisters(void) { return registers; }
};

#endif // CORTEXM0_H
