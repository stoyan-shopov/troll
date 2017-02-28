/*
Copyright (c) 2016-2017 stoyan shopov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef CORTEXM0_H
#define CORTEXM0_H

#include "sforth.hxx"
#include "util.hxx"

class CortexM0
{
private:
	static const int register_count, program_counter_register_number, stack_pointer_register_number,
	/*! \todo	extract these from the .debug_frame dwarf cie program, and make it a generic rule - not simply a register number */
		return_address_register_number,
		cfa_register_number;
	std::vector<uint32_t> registers;
	void readRawRegistersFromTarget(void);
public:
	CortexM0(Sforth * sforth_engine, class Target * target_controller);
	void setTargetController(class Target * target_controller);
	void primeUnwinder(void);
	bool unwindFrame(const QString & unwind_code, uint32_t start_address, uint32_t unwind_address);
	std::vector<uint32_t> getRegisters(void) { return registers; }
	uint32_t programCounter(void) { if (registers.size() <= program_counter_register_number) Util::panic(); return registers.at(program_counter_register_number)&~1; }
	uint32_t stackPointerValue(void) { if (registers.size() <= stack_pointer_register_number) Util::panic(); return registers.at(stack_pointer_register_number); }
	bool architecturalUnwind(void);
	static const int registerCount(void) { return register_count; }
};

#endif // CORTEXM0_H
