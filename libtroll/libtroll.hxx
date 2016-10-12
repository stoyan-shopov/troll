#include <stdint.h>

class DwarfUtil
{
	static uint32_t uleb128(const uint8_t * data)
	{
		uint64_t result;
		uint8_t x;
		int shift = 0;
		do x = * data ++, result |= ((x & 0x7f) << shift), shift += 7; while (x & 0x80);
		if (result > 0xffffffff)
			*(int*)0=0;
		return result;
	}
	static int32_t sleb128(const uint8_t * data)
	{
		uint64_t result;
		uint8_t x;
		int shift = 0;
		do x = * data ++, result |= ((x & 0x7f) << shift), shift += 7; while (x & 0x80);
		if (result > 0xffffffff)
			*(int*)0=0;
		/* handle sign */
		if (x & 0x40)
			/* propagate sign bit */
			result |= - 1 << (shift - 1);
		return result;
	}
};

class DwarfData
{
	struct
	{
		uint32_t	unit_length;
		uint16_t	version;
		uint32_t	compilation_unit_debug_info_offset;
		uint8_t		address_size;
		uint8_t		segment_size;
		struct
		{
			uint32_t	start_address;
			uint32_t	length;
		}
		ranges[0];
	}
	* debug_aranges;
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

