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
#include <QMessageBox>
#include <QFile>

#include "cortexm0.hxx"
#include "target.hxx"

static Target		* target;
static Sforth		* sforth;
static RegisterCache	* register_cache;

extern "C"
{
void do_target_fetch(void)
{
	auto x = sforth->getResults(1); if (x.size() != 1) Util::panic();
	try
	{
		sforth->push(target->readWord(x.at(0)));
	}
	catch (enum TARGET_ERROR_ENUM error)
	{
		if (error == MEMORY_READ_ERROR)
		{
			QMessageBox::critical(0, "cannot read target memory", QString("failed to read memory at address $%1").arg(x.at(0), 8, 16, QChar('0')));
			do_abort();
		}
		Util::panic();
	}
}
void do_target_register_fetch(void)	{ sf_push(register_cache->readCachedRegister(sf_pop())); }
void do_panic(void)		{ Util::panic(); }
}

const int CortexM0::register_count = 16,
	CortexM0::program_counter_register_number = 15,
	CortexM0::stack_pointer_register_number = 13,
	CortexM0::return_address_register_number = /*! \todo	extract this from the .debug_frame dwarf cie program */ 14,
	CortexM0::cfa_register_number = /*! \todo	extract this from the .debug_frame dwarf cie program */ 13;

void CortexM0::readRawRegistersFromTarget()
{
	int i;
	registers.clear();
	sforth->evaluate("unwound-registers\n");
	auto r = sforth->getResults(1);
	if (r.size() != 1)
		return;
        for (i = 0; i < register_count; registers.push_back(*(cell *)(r.at(0) + i ++ * sizeof(cell))));
}

CortexM0::CortexM0(Sforth * sforth_engine, Target *target_controller, RegisterCache *registers)
{
	sforth = sforth_engine;
	target = target_controller;
	register_cache = registers;
	QFile f(":/sforth/unwinder.fs");
	f.open(QFile::ReadOnly);
	sforth->push(register_count);
	sforth->evaluate(f.readAll() + '\n');
}

void CortexM0::setTargetController(Target *target_controller)
{
	target = target_controller;
}

void CortexM0::primeUnwinder()
{
int i;
	registers.clear();
	for (i = 0; i < register_count; registers.push_back(target->readRawUncachedRegister(i++)));
}

bool CortexM0::unwindFrame(const QString & unwind_code, uint32_t start_address, uint32_t unwind_address)
{
int i;
uint32_t cfa;
	for (i = 0; i < registers.size(); sforth->push(registers.at(i++)));
	sforth->evaluate("init-unwinder-round\n");
	sforth->evaluate(QString("%1 to current-address %2 to unwind-address ").arg(start_address).arg(unwind_address) + unwind_code + '\n');
	auto r = sforth->getResults(1);
	if (r.size() == 0)
	{
		/* evaluation probably aborted due to unsupported unwinding rules */
		QMessageBox::critical(0, "register frame unwinding aborted", "register frame unwinding aborted\nsee the sforth execution log for more details");
		return false;
	}
        if (r.size() != 1 || r.at(0) != C_TRUE)
		return false;
	sforth->evaluate("fetch-cfa-value\n");
	if ((r = sforth->getResults(1)).size() != 1)
		return false;
	cfa = r.at(0);
	readRawRegistersFromTarget();
	if (registers.size() != register_count)
		return false;
	registers.at(program_counter_register_number) = registers.at(return_address_register_number);
	registers.at(cfa_register_number) = cfa;
	return true;
}

bool CortexM0::architecturalUnwind()
{
	sforth->evaluate("architectural-unwind\n");
	auto r = sforth->getResults(1);
        if (r.size() != 1 || r.at(0) != C_TRUE)
		return false;
	readRawRegistersFromTarget();
	return true;
}
