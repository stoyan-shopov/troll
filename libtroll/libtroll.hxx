#include <stdint.h>
#include <map>

class DwarfUtil
{
public:
	static void panic(...) {*(int*)0=0;}
	static uint32_t uleb128(const uint8_t * data, int * decoded_len)
	{
		uint64_t result = 0;
		uint8_t x;
		int shift = * decoded_len = 0;
		do x = * data ++, result |= ((x & 0x7f) << shift), shift += 7, (* decoded_len) ++; while (x & 0x80);
		if (result > 0xffffffff)
			panic();
		return result;
	}
	static uint32_t uleb128(const uint8_t * data)
	{
		uint64_t result = 0;
		uint8_t x;
		int shift = 0;
		do x = * data ++, result |= ((x & 0x7f) << shift), shift += 7; while (x & 0x80);
		if (result > 0xffffffff)
			panic();
		return result;
	}
	static int32_t sleb128(const uint8_t * data, int * decoded_len)
	{
		uint64_t result = 0;
		uint8_t x;
		int shift = * decoded_len = 0;
		do x = * data ++, result |= ((x & 0x7f) << shift), shift += 7, (* decoded_len) ++; while (x & 0x80);
		if (result > 0xffffffff)
			panic();
		/* handle sign */
		if (x & 0x40)
			/* propagate sign bit */
			result |= - 1 << (shift - 1);
		return result;
	}
	static int32_t sleb128(const uint8_t * data)
	{
		uint64_t result = 0;
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

struct debug_arange
{
	const uint8_t	* data;
	uint32_t	unit_length(){return*(uint32_t*)(data+0);}
	uint16_t	version(){return*(uint16_t*)(data+4);}
	uint32_t	compilation_unit_debug_info_offset(){return*(uint32_t*)(data+6);}
	uint8_t		address_size(){return*(uint8_t*)(data+10);}
	uint8_t		segment_size(){return*(uint8_t*)(data+11);}
	struct compilation_unit_range
	{
		uint32_t	start_address;
		uint32_t	length;
	};
	struct compilation_unit_range (*ranges(void))[]{return 0;}
	debug_arange(const uint8_t * data) { this->data = data; }
	void next(void) { data += unit_length() + sizeof unit_length(); }
};

struct compilation_unit_header
{
	const uint8_t		* data;
	uint32_t	unit_length(){return*(uint32_t*)(data+0);}
	uint16_t	version(){return*(uint16_t*)(data+4);}
	uint32_t	debug_abbrev_offset(){return*(uint32_t*)(data+6);}
	uint8_t		address_size(){return*(uint8_t*)(data+10);}
	compilation_unit_header(const uint8_t * data) { this->data = data; }
	void next(void) { data += unit_length() + sizeof unit_length(); }
};


class DwarfData
{
private:
	const uint8_t * debug_info;
	uint32_t	debug_info_len;
	const uint8_t	* debug_abbrev;
	uint32_t	debug_abbrev_len;

	const uint8_t * debug_aranges;
	uint32_t	debug_aranges_len;
public:
	DwarfData(const void * debug_aranges, uint32_t debug_aranges_len, const void * debug_info, uint32_t debug_info_len,
		  const void * debug_abbrev, uint32_t debug_abbrev_len)
	{
		this->debug_aranges = (const uint8_t *) debug_aranges;
		this->debug_aranges_len = debug_aranges_len;
		this->debug_info = (const uint8_t *) debug_info;
		this->debug_info_len = debug_info_len;
		this->debug_abbrev = (const uint8_t *) debug_abbrev;
		this->debug_abbrev_len = debug_abbrev_len;
	}
	/* returns -1 if the compilation unit is not found */
	uint32_t	get_compilation_unit_debug_info_offset_for_address(uint32_t address)
	{
#if 0
		int i;
		uint32_t start, len;
		const struct debug_arange * arange = first_arange();
		while (arange)
		{
			if (arange->address_size != 4)
				DwarfUtil::panic();
			i = 0;
			while ((start = arange->ranges[i].start_address) && (len = arange->ranges[i].length))
				if (start <= address && address < start + len)
					return arange->compilation_unit_debug_info_offset;
			arange = next(arange);
		}
#endif
		return -1;
	}
	int compilation_unit_count(void)
	{
		int i;
		struct debug_arange arange(debug_aranges);
		for (i = 0; arange.data < debug_aranges + debug_aranges_len; arange.next(), i++)
			if (arange.address_size() != 4)
				DwarfUtil::panic();
		return i;
	}
	/* first number is the abbreviation code, the second is the offset in .debug_abbrev */
	std::map<uint32_t, uint32_t> abbreviations_of_compilation_unit(uint32_t compilation_unit_offset)
	{
		const uint8_t * debug_abbrev = this->debug_abbrev + compilation_unit_header((uint8_t *) debug_info + compilation_unit_offset) . debug_abbrev_offset(); 
		uint32_t code, tag, has_children, name, form;
		int len;
		std::map<uint32_t, uint32_t> abbrevs;
		while ((code = DwarfUtil::uleb128(debug_abbrev, & len)))
		{
			auto x = abbrevs.find(code);
			if (x != abbrevs.end())
				DwarfUtil::panic("duplicate abbreviation code");
			abbrevs.operator [](code) = debug_abbrev - this->debug_abbrev;
			debug_abbrev += len;
			tag = DwarfUtil::uleb128(debug_abbrev, & len);
			debug_abbrev += len;
			has_children = DwarfUtil::uleb128(debug_abbrev, & len);
			debug_abbrev += len;
			do
			{
				name = DwarfUtil::uleb128(debug_abbrev, & len);
				debug_abbrev += len;
				form = DwarfUtil::uleb128(debug_abbrev, & len);
				debug_abbrev += len;
			}
			while (name || form);
		}
		return abbrevs;
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

