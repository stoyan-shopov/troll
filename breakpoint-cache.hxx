#ifndef BREAKPOINTCACHE_H
#define BREAKPOINTCACHE_H

#include <QSet>
#include <QVector>
#include <QHash>

class BreakpointCache
{
private:
	void updateBreakpointSets(void);
public:
	BreakpointCache(void){}

	struct SourceCodeBreakpoint
	{
		QString source_filename, directory_name, compilation_directory;
		bool enabled;
		int line_number;
		QVector<uint32_t> addresses;
		bool operator == (const struct SourceCodeBreakpoint & other) const
		{
			return line_number == other.line_number && source_filename == other.source_filename
					&& directory_name == other.directory_name && compilation_directory == other.compilation_directory;
		}
	};
	struct MachineAddressBreakpoint
	{
		uint32_t	address;
		struct SourceCodeBreakpoint inferred_breakpoint;
		bool		enabled;
	};

	QVector<struct MachineAddressBreakpoint> machineAddressBreakpoints;
	QVector<struct SourceCodeBreakpoint> sourceCodeBreakpoints;

	int machineBreakpointIndex(uint32_t address)
	{
		int i;
		for (i = 0; i < machineAddressBreakpoints.size(); i ++)
			if (machineAddressBreakpoints[i].address == address)
				return i;
		return -1;
	}
	int inferredBreakpointIndex(const struct SourceCodeBreakpoint & breakpoint)
	{
		int i;
		for (i = 0; i < machineAddressBreakpoints.size(); i ++)
			if (machineAddressBreakpoints[i].inferred_breakpoint == breakpoint)
				return i;
		return -1;
	}

	int sourceBreakpointIndex(const struct SourceCodeBreakpoint & breakpoint)
	{
		int i;
		for (i = 0; i < sourceCodeBreakpoints.size(); i ++)
			if (sourceCodeBreakpoints[i] == breakpoint)
				return i;
		return -1;
	}
	
	QSet<struct SourceCodeBreakpoint> enabledSourceCodeBreakpoints, disabledSourceCodeBreakpoints;
	QSet<uint32_t> enabledMachineAddressBreakpoints, disabledMachineAddressBreakpoints;
	
	void addSourceCodeBreakpoint(const struct SourceCodeBreakpoint & breakpoint) { sourceCodeBreakpoints.push_back(breakpoint); updateBreakpointSets(); }
	void removeSourceCodeBreakpointAtIndex(int breakpoint_index) { sourceCodeBreakpoints.removeAt(breakpoint_index); updateBreakpointSets(); }
	void addMachineAddressBreakpoint(const struct MachineAddressBreakpoint & breakpoint) { machineAddressBreakpoints.push_back(breakpoint); updateBreakpointSets(); }
	void removeMachineAddressBreakpointAtIndex(int breakpoint_index) { machineAddressBreakpoints.removeAt(breakpoint_index); updateBreakpointSets(); }
};

#endif // BREAKPOINTCACHE_H
