#ifndef CORTEXM0_H
#define CORTEXM0_H

#include "sforth.hxx"
#include "util.hxx"

class CortexM0
{
private:
	static const int register_count, program_counter_register_number, return_address_register_number, cfa_register_number;
	std::vector<uint32_t> registers;
	void readRegisters(void);
public:
	CortexM0(Sforth * sforth_engine, class Target * target_controller);
	void setTargetController(class Target * target_controller);
	void primeUnwinder(void);
	bool unwindFrame(const QString & unwind_code, uint32_t start_address, uint32_t unwind_address);
	std::vector<uint32_t> unwoundRegisters(void) { return registers; }
	uint32_t programCounter(void) { if (registers.size() <= program_counter_register_number) Util::panic(); return registers.at(program_counter_register_number)&~1; }
	bool architecturalUnwind(void);
};

#endif // CORTEXM0_H
