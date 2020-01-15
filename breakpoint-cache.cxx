#include "breakpoint-cache.hxx"

uint qHash(const BreakpointCache::SourceCodeBreakpoint & key)
{ return qHash(key.source_filename) ^ qHash(key.directory_name) ^ qHash(key.compilation_directory) ^ qHash(key.line_number); }

uint qHash(const BreakpointCache::MachineAddressBreakpoint & key) { return qHash(key.address); }

const QString BreakpointCache::test_drive_base_directory = "troll-test-drive-files/";

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
	disabledSourceCodeBreakpoints.subtract(enabledSourceCodeBreakpoints);
	disabledMachineAddressBreakpoints.subtract(enabledMachineAddressBreakpoints);
}
