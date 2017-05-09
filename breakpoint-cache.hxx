#ifndef BREAKPOINTCACHE_H
#define BREAKPOINTCACHE_H

#include <QSet>
#include <QVector>
#include <QHash>
#include <QDir>

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
			return line_number == other.line_number && QDir::toNativeSeparators(source_filename) == QDir::toNativeSeparators(other.source_filename)
					&& QDir::toNativeSeparators(directory_name) == QDir::toNativeSeparators(other.directory_name)
					&& QDir::toNativeSeparators(compilation_directory) == QDir::toNativeSeparators(other.compilation_directory);
		}
		/* returns -1 if the breakpoint is not inside the source code file */
		int breakpointedLineNumberForSourceCode(const QString & source_filename, const QString & directory_name, const QString & compilation_directory) const
		{
			return (QDir::toNativeSeparators(source_filename) == QDir::toNativeSeparators(this->source_filename)
					&& QDir::toNativeSeparators(directory_name) == QDir::toNativeSeparators(this->directory_name)
					&& QDir::toNativeSeparators(compilation_directory) == QDir::toNativeSeparators(this->compilation_directory))
				? line_number : -1;
		}
	};
	struct MachineAddressBreakpoint
	{
		uint32_t	address;
		struct SourceCodeBreakpoint inferred_breakpoint;
		bool		enabled;
		bool operator == (const struct MachineAddressBreakpoint & other) const { return address == other.address; }
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
	int /* returns the index at which the breakpoint was placed */ addMachineAddressBreakpoint(const struct MachineAddressBreakpoint & breakpoint)
	{ machineAddressBreakpoints.push_back(breakpoint); updateBreakpointSets(); return machineAddressBreakpoints.size() - 1; }
	void removeMachineAddressBreakpointAtIndex(int breakpoint_index) { machineAddressBreakpoints.removeAt(breakpoint_index); updateBreakpointSets(); }
	
	void toggleMachineBreakpointAtIndex(int breakpoint_index) { machineAddressBreakpoints[breakpoint_index].enabled = ! machineAddressBreakpoints[breakpoint_index].enabled; updateBreakpointSets(); }
	void setMachineBreakpointAtIndexEnabled(int breakpoint_index, bool is_enabled) { machineAddressBreakpoints[breakpoint_index].enabled = is_enabled; updateBreakpointSets(); }
	void toggleSourceBreakpointAtIndex(int breakpoint_index) { sourceCodeBreakpoints[breakpoint_index].enabled = ! sourceCodeBreakpoints[breakpoint_index].enabled; updateBreakpointSets(); }
	void setSourceBreakpointAtIndexEnabled(int breakpoint_index, bool is_enabled) { sourceCodeBreakpoints[breakpoint_index].enabled = is_enabled; updateBreakpointSets(); }
};

uint qHash(const BreakpointCache::SourceCodeBreakpoint & key);
uint qHash(const BreakpointCache::MachineAddressBreakpoint & key);

#endif // BREAKPOINTCACHE_H
