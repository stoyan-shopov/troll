#include <QMessageBox>

#include "cortexm0.hxx"
#include "target.hxx"

static Target		* target;
static Sforth		* sforth;

extern "C"
{
void do_target_fetch(void)	{ auto x = sforth->getResults(1); if (x.size() != 1) Util::panic(); sforth->push(target->readWord(x.at(0))); }
void do_panic(void)		{ Util::panic(); }
}

const int CortexM0::register_count = 16, CortexM0::program_counter_register_number = 15,
	CortexM0::return_address_register_number = /*! \todo	extract this from the .debug_frame dwarf cie program */ 14,
	CortexM0::cfa_register_number = /*! \todo	extract this from the .debug_frame dwarf cie program */ 13;

void CortexM0::readRegisters()
{
	int i;
	registers.clear();
	sforth->evaluate("unwound-registers\n");
	auto r = sforth->getResults(1);
	if (r.size() != 1)
		return;
	for (i = 0; i < register_count; registers.push_back(*(uint32_t*)(r.at(0) + i ++ * sizeof(uint32_t))));
}

CortexM0::CortexM0(Sforth * sforth_engine, Target *target_controller)
{
	sforth = sforth_engine;
	target = target_controller;
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
	for (i = 0; i < register_count; registers.push_back(target->readRegister(i++)));
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
	if (r.size() != 1 || r.at(0) != 0xffffffff)
		return false;
	sforth->evaluate("cfa-value\n");
	if ((r = sforth->getResults(1)).size() != 1)
		return false;
	cfa = r.at(0);
	readRegisters();
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
	if (r.size() != 1 || r.at(0) != 0xffffffff)
		return false;
	readRegisters();
	return true;
}
