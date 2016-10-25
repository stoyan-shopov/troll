#include "cortexm0.hxx"
#include "util.hxx"
#include "target.hxx"

static Target		* target;

extern "C"
{
void do_target_fetch(void)	{ print_str("cortexm0 support\n"); }
void do_panic(void)		{ Util::panic(); }
}

CortexM0::CortexM0(Sforth * sforth, Target *target_controller)
{
	this->sforth = sforth;
	target = target_controller;
	Util::panic();
}
