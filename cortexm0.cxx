#include "cortexm0.hxx"
#include "util.hxx"
#include "target.hxx"

static Target		* target;

extern "C"
{
void do_target_fetch(void)	{ print_str("cortexm0 support\n"); }
void do_panic(void)		{ Util::panic(); }
}

const int CortexM0::register_count = 16;

CortexM0::CortexM0(Sforth * sforth, Target *target_controller)
{
	this->sforth = sforth;
	target = target_controller;
	QFile f(":/sforth/unwinder.fs");
	f.open(QFile::ReadOnly);
	sforth->push(register_count);
	sforth->evaluate(f.readAll() + '\n');
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
	for (i = 0; i < registers.size(); sforth->push(registers.at(i++)));
	sforth->evaluate(QString("%1 to start-address %2 to unwind-address ").arg(start_address).arg(unwind_address)
				+ "init-unwinder-round " + unwind_code + '\n');
	registers.clear();
	auto r = sforth->getResults(1);
	if (r.size() != 1)
		return false;
	sforth->evaluate("unwound-registers\n");
	if ((r = sforth->getResults(1)).size() != 1)
		return false;
	for (i = 0; i < register_count; registers.push_back(*(uint32_t*)(r.at(0) + i ++ * sizeof(uint32_t))));
	return true;
}
