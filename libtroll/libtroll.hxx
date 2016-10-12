#include <stdint.h>

class DwarfUtil
{
public:
	static void panic(void) {*(int*)0=0;}
	static uint32_t uleb128(const uint8_t * data)
	{
		uint64_t result;
		uint8_t x;
		int shift = 0;
		do x = * data ++, result |= ((x & 0x7f) << shift), shift += 7; while (x & 0x80);
		if (result > 0xffffffff)
			panic();
		return result;
	}
	static int32_t sleb128(const uint8_t * data)
	{
		uint64_t result;
		uint8_t x;
		int shift = 0;
		do x = * data ++, result |= ((x & 0x7f) << shift), shift += 7; while (x & 0x80);
		if (result > 0xffffffff)
			panic();
		/* handle sign */
		if (x & 0x40)
			/* propagate sign bit */
			result |= - 1 << (shift - 1);
		return result;
	}
};

class DwarfData
{
private:
	const struct debug_arange
	{
		uint32_t	unit_length;
		uint16_t	version;
		uint32_t	compilation_unit_debug_info_offset;
		uint8_t		address_size;
		uint8_t		segment_size;
		struct compilation_unit_range
		{
			uint32_t	start_address;
			uint32_t	length;
		}
		ranges[0];
	} __attribute__((packed))
	* debug_aranges;
	uint32_t	debug_aranges_len;
public:
	DwarfData(const void * debug_aranges, uint32_t debug_aranges_len)
	{
		this->debug_aranges = (const struct debug_arange *) debug_aranges;
		this->debug_aranges_len = debug_aranges_len;
	}
	/* returns -1 if the compilation unit is not found */
	uint32_t	get_compilation_unit_debug_info_offset_for_address(uint32_t address)
	{
		int i;
		uint32_t start, len;
		const struct debug_arange * arange = debug_aranges;
		while ((uint32_t) arange < (uint32_t) debug_aranges + debug_aranges_len)
		{
			if (arange->address_size != 4)
				DwarfUtil::panic();
			i = 0;
			while ((start = arange->ranges[i].start_address) && (len = arange->ranges[i].length))
				if (start <= address && address < start + len)
					return arange->compilation_unit_debug_info_offset;
		}
		return -1;
	}
};

class Die
{
protected:
	uint32_t	debug_info_offset;
	class Die	* sibling;
	class Die	* children;
};

class CompilationUnit : Die
{
public:
};

