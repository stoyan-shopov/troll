#include "breakpoint-cache.hxx"

void BreakpointCache::updateBreakpointSets()
{
int i, j;
	enabledSourceCodeBreakpoints.clear();
	enabledMachineAddressBreakpoints.clear();
	disabledSourceCodeBreakpoints.clear();
	disabledMachineAddressBreakpoints.clear();
	
	for (i = 0; i < sourceCodeBreakpoints.size(); i ++)
	{
		SourceCodeBreakpoint & b(sourceCodeBreakpoints[i]);
		if (b.enabled)
		{
			enabledSourceCodeBreakpoints.insert(b);
			for (j = 0; j < b.addresses.size(); j ++)
				enabledMachineAddressBreakpoints.insert(b.addresses[j]);
		}
		else
		{
			disabledSourceCodeBreakpoints.insert(b);
			for (j = 0; j < b.addresses.size(); j ++)
				disabledMachineAddressBreakpoints.insert(b.addresses[j]);
		}
	}
	for (i = 0; i < machineAddressBreakpoints.size(); i ++)
	{
		MachineAddressBreakpoint & b(machineAddressBreakpoints[i]);
		if (b.enabled)
		{
			enabledSourceCodeBreakpoints.insert(b.inferred_breakpoint);
			enabledMachineAddressBreakpoints.insert(b.address);
		}
		else
		{
			disabledSourceCodeBreakpoints.insert(b.inferred_breakpoint);
			disabledMachineAddressBreakpoints.insert(b.address);
		}
	}
	disabledSourceCodeBreakpoints.subtract(disabledSourceCodeBreakpoints.intersect(enabledSourceCodeBreakpoints));
	disabledMachineAddressBreakpoints.subtract(disabledMachineAddressBreakpoints.intersect(enabledMachineAddressBreakpoints));
}
