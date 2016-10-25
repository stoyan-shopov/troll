#ifndef CORTEXM0_H
#define CORTEXM0_H

#include "sforth.hxx"

class CortexM0
{
private:
	static const int register_count, program_counter_register_number, return_address_register_number, cfa_register_number;
	std::vector<uint32_t> registers;
public:
	CortexM0(Sforth * sforth_engine, class Target * target_controller);
	void primeUnwinder(void);
	bool unwindFrame(const QString & unwind_code, uint32_t start_address, uint32_t unwind_address);
	std::vector<uint32_t> unwoundRegisters(void) { return registers; }
};

#endif // CORTEXM0_H
