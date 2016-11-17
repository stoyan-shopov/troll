#include <dwarf.h>
#include <stdint.h>
#include <map>
#include <vector>
#include <sstream>
#include <QDebug>

#define HEX(x) QString("$%1").arg(x, 8, 16, QChar('0'))

#define DEBUG_ENABLED			0
#define DEBUG_LINE_PROGRAMS_ENABLED	0
#define DEBUG_DIE_READ_ENABLED		0
#define DEBUG_ADDRESS_RANGE_ENABLED	0
#define UNWIND_DEBUG_ENABLED		0

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
	static uint32_t uleb128x(const uint8_t * & data)
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
		/*
		if (result > 0xffffffff)
			panic();
			*/
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
	static int skip_form_bytes(int form, const uint8_t * debug_info_bytes)
	{
		int bytes_to_skip;
		switch (form)
		{
		default:
			panic();
		case DW_FORM_addr:
			return 4;
		case DW_FORM_block2:
			return 2 + * (uint16_t *) debug_info_bytes;
		case DW_FORM_block4:
			return 4 + * (uint32_t *) debug_info_bytes;
		case DW_FORM_data2:
			return 2;
		case DW_FORM_data4:
			return 4;
		case DW_FORM_data8:
			return 8;
		case DW_FORM_string:
			return strlen((const char *) debug_info_bytes) + 1;
		case DW_FORM_block:
			panic();
		case DW_FORM_block1:
			return 1 + * debug_info_bytes;
		case DW_FORM_data1:
			return 1;
		case DW_FORM_flag:
			return 1;
		case DW_FORM_sdata:
			return sleb128(debug_info_bytes, & bytes_to_skip), bytes_to_skip;
		case DW_FORM_strp:
			return 4;
		case DW_FORM_udata:
			panic();
			return uleb128(debug_info_bytes, & bytes_to_skip), bytes_to_skip;
		case DW_FORM_ref_addr:
			return 4;
		case DW_FORM_ref1:
			return 1;
		case DW_FORM_ref2:
			return 2;
		case DW_FORM_ref4:
			return 4;
		case DW_FORM_ref8:
			return 8;
		case DW_FORM_ref_udata:
			return uleb128(debug_info_bytes, & bytes_to_skip), bytes_to_skip;
		case DW_FORM_indirect:
			panic();
		case DW_FORM_sec_offset:
			return 4;
		case DW_FORM_exprloc:
			return uleb128(debug_info_bytes, & bytes_to_skip) + bytes_to_skip;
		case DW_FORM_flag_present:
			return 0;
		case DW_FORM_ref_sig8:
			return 8;
		}
	}
	static uint32_t formConstant(uint32_t attribute_form, const uint8_t * debug_info_bytes)
	{
		switch (attribute_form)
		{
		case DW_FORM_addr:
		case DW_FORM_data4:
		case DW_FORM_sec_offset:
			return * (uint32_t *) debug_info_bytes;
		case DW_FORM_data1:
			return * (uint8_t *) debug_info_bytes;
		case DW_FORM_data2:
			return * (uint16_t *) debug_info_bytes;
		default:
			panic();
		}
	}
	static std::string formString(uint32_t attribute_form, const uint8_t * debug_info_bytes, const uint8_t * debug_str)
	{
		switch (attribute_form)
		{
		case DW_FORM_string:
			return (const char *) debug_info_bytes;
		case DW_FORM_strp:
			return (const char *) (debug_str + * (uint32_t *) debug_info_bytes);
		default:
			panic();
		}
	}

	static uint32_t fetchHighLowPC(uint32_t attribute_form, const uint8_t * debug_info_bytes, uint32_t relocation_address = 0)
	{
		switch (attribute_form)
		{
		case DW_FORM_addr:
			return * (uint32_t *) debug_info_bytes;
		case DW_FORM_data4:
			return (* (uint32_t *) debug_info_bytes) + relocation_address;
		default:
			panic();
		}
	}
	static bool isDataObject(uint32_t tag) { return tag == DW_TAG_variable; }
	static bool isSubprogram(uint32_t tag) { return tag == DW_TAG_subprogram; }
	static bool isLocationConstant(uint32_t location_attribute_form, const uint8_t * debug_info_bytes)
	{
		if (location_attribute_form != DW_FORM_exprloc)
			return false;
		uint32_t len;
		len = uleb128x(debug_info_bytes);
		return (len == 5 && * debug_info_bytes == DW_OP_addr) ? true : false;
	}
};

struct Die
{
	uint32_t	tag;
	/* offset of this die in the .debug_info section */
	uint32_t	offset;
	uint32_t	abbrev_offset;
	std::vector<struct Die> children;
	Die(uint32_t tag, uint32_t offset, uint32_t abbrev_offset){ this->tag = tag, this->offset = offset, this->abbrev_offset = abbrev_offset; }
};

/* !!! warning - this can generally be a circular graph !!! */
struct DwarfTypeNode
{
	struct Die	die;
	int		next;
	int		sibling;
	int		childlist;
        DwarfTypeNode(const struct Die & xdie) : die(xdie)	{ next = sibling = childlist = -1; }
};

struct Abbreviation
{
private:
	struct
	{
		const uint8_t	* attributes, * init_attributes;
		uint32_t code, tag;
		bool has_children;
	} s;
public:
	uint32_t tag(void){ return s.tag; }
	bool has_children(void){ return s.has_children; }
	/* first number is the attribute name, the second is the attribute form */
	std::pair<uint32_t, uint32_t> next_attribute(void)
	{
		const uint8_t * p = s.attributes;
		std::pair<uint32_t, uint32_t> a;
		a.first = DwarfUtil::uleb128x(p);
		a.second = DwarfUtil::uleb128x(p);
		if (a.first || a.second)
			/* not the last attribute - skip to the next one */
			s.attributes = p;
		return a;
	}

	Abbreviation(const uint8_t * abbreviation_data)
	{
		s.code = DwarfUtil::uleb128x(abbreviation_data);
		s.tag = DwarfUtil::uleb128x(abbreviation_data);
		s.has_children = (DwarfUtil::uleb128x(abbreviation_data) == DW_CHILDREN_yes);
		s.init_attributes = s.attributes = abbreviation_data;
	}
	/* the first number is the attribute form, the pointer is to the data in .debug_info for the attribute searched
	 * returns <0, 0> if the attribute with the searched name is not found */
	std::pair<uint32_t, const uint8_t *> dataForAttribute(uint32_t attribute_name, const uint8_t * debug_info_data_for_die)
	{
		s.attributes = s.init_attributes;
		std::pair<uint32_t, uint32_t> a;
		/* skip the die abbreviation code */
		DwarfUtil::uleb128x(debug_info_data_for_die);
		while (1)
		{
			a = next_attribute();
			if (!a.first)
				return std::pair<uint32_t, const uint8_t *> (0, 0);
			if (a.first == attribute_name)
				return std::pair<uint32_t, const uint8_t *> (a.second, debug_info_data_for_die);
			debug_info_data_for_die += DwarfUtil::skip_form_bytes(a.second, debug_info_data_for_die);
		}
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
	struct compilation_unit_range *ranges(void){return (struct compilation_unit_range *) (data+16);}
	debug_arange(const uint8_t * data) { this->data = data; }
	struct debug_arange & next(void) { data += unit_length() + sizeof unit_length(); return * this; }
};

struct compilation_unit_header
{
	const uint8_t		* data;
	uint32_t	unit_length(){return*(uint32_t*)(data+0);}
	uint16_t	version(){return*(uint16_t*)(data+4);}
	uint32_t	debug_abbrev_offset(){return*(uint32_t*)(data+6);}
	uint8_t		address_size(){return*(uint8_t*)(data+10);}
	compilation_unit_header(const uint8_t * data) { this->data = data; }
	struct compilation_unit_header & next(void) { data += unit_length() + sizeof unit_length(); return * this; }
};

struct StaticObject
{
	const char *	name;
	int		file, line;
	uint32_t	die_offset;
};

struct SourceCodeCoordinates
{
	const char	* filename;
	const char	* directoryname;
	uint32_t	address;
	uint32_t	line;
	SourceCodeCoordinates(void) { filename = "<<< unknown filename >>>", directoryname = 0, address = line = -1; }
};

class DebugLine
{
private:
	const uint8_t *	debug_line, * header;
	uint32_t	debug_line_len;
	
	uint32_t unit_length(void) { return * (uint32_t *) header; }
	uint16_t version(void) { return * (uint16_t *) (header + 4); }
	uint32_t header_length(void) { return * (uint32_t *) (header + 6); }
	uint8_t minimum_instruction_length(void) { return * (uint8_t *) (header + 10); }
	//uint8_t maximum_operations_per_instruction(void) { return * (uint8_t *) (header + 11); }
	uint8_t default_is_stmt(void) { return * (uint8_t *) (header + 11); }
	int8_t line_base(void) { return * (int8_t *) (header + 12); }
	uint8_t line_range(void) { return * (uint8_t *) (header + 13); }
	uint8_t opcode_base(void) { return * (uint8_t *) (header + 14); }
	const char * include_directories(void)
	{
		int i(opcode_base());
		const uint8_t * p(header + 15);
		while (-- i) DwarfUtil::uleb128x(p);
		return (const char *) p;
	}
	const char * file_names(void)
	{
		auto p = include_directories();
		size_t x;
		while ((x = strlen(p)))
			p += x + 1;
		return p + 1;
	}
	const uint8_t * line_number_program(void) { return header + sizeof unit_length() + sizeof version() + sizeof header_length() + header_length(); }
	struct line_state { uint32_t file, address, is_stmt; int line; } registers[2], * current, * prev;
	void init(void) { registers[0] = (struct line_state) { .file = 1, .address = 0, .is_stmt = default_is_stmt(), .line = 1, };
				registers[1] = (struct line_state) { .file = 1, .address = 0xffffffff, .is_stmt = 0, .line = -1, };
				current = registers, prev = registers + 1; }
	void swap(void) { struct line_state * x(current); current = prev; prev = x; }
public:
	struct lineAddress { uint32_t line, address, address_span; struct lineAddress * next; lineAddress(void) { line = address = address_span = -1; next = 0; } };
	DebugLine(const uint8_t * debug_line, uint32_t debug_line_len) { header = this->debug_line = debug_line, this->debug_line_len = debug_line_len; }
	void dump(void)
	{
		if (version() != 2) DwarfUtil::panic();
		const uint8_t * p(line_number_program()), op_base(opcode_base()), lrange(line_range());
		int lbase(line_base());
		uint32_t min_insn_length(minimum_instruction_length());
		int len, x;
		init();
		if (DEBUG_LINE_PROGRAMS_ENABLED)
		{
			qDebug() << "debug line for offset" << HEX(header - debug_line);
			qDebug() << "include directories table:";
			union { const char * s; const uint8_t * p; } x;
			x.s = include_directories();
			size_t l;
			while ((l = strlen(x.s)))
				qDebug() << QString(x.s), x.s += l + 1;
			qDebug() << "file names table:";
			x.s = file_names();
			while ((l = strlen(x.s)))
			{
				qDebug() << QString(x.s), x.s += l + 1;
				/* skip directory index, file time and file size */
				DwarfUtil::uleb128x(x.p), DwarfUtil::uleb128x(x.p), DwarfUtil::uleb128x(x.p);
			}
		}
		while (p < header + sizeof(uint32_t) + unit_length())
		{
			if (! * p)
			{
				/* extended opcodes */
				len = DwarfUtil::uleb128(++ p, & x);
				p += x;
				if (!len)
					DwarfUtil::panic();
				switch (* p ++)
				{
					default:
						DwarfUtil::panic();
					case DW_LNE_set_discriminator:
						if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set discriminator to" << DwarfUtil::uleb128(p, & x) << "!!! IGNORED !!!";
						if (len != x + 1) DwarfUtil::panic();
						p += x;
						break;
					case DW_LNE_end_sequence:
						if (len != 1) DwarfUtil::panic();
						init();
						if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "end of sequence";
						break;
					case DW_LNE_set_address:
						if (len != 5) DwarfUtil::panic();
						current->address = * (uint32_t *) p;
						p += sizeof current->address;
						if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "extended opcode, set address to" << HEX(current->address);
						break;
				}
			}
			else if (* p >= op_base)
			{
				/* special opcodes */
				uint8_t x = * p ++ - op_base;
				current->address += (x / lrange) * min_insn_length;
				current->line += lbase + x % lrange;
				if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "special opcode, set address to" << HEX(current->address) << "line to" << current->line;
				swap();
				* current = * prev;
			}
			/* standard opcodes */
			else switch (* p ++)
			{
				default:
					DwarfUtil::panic();
					break;
				case DW_LNS_copy:
				/*
					if (xaddr <= target_address && target_address < address)
						DwarfUtil::panic();*/
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "copy";
					swap();
					* current = * prev;
					break;
				case DW_LNS_advance_pc:
					current->address += DwarfUtil::uleb128(p, & len) * min_insn_length;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "advance pc to" << HEX(current->address);
					p += len;
					break;
				case DW_LNS_advance_line:
					current->line += DwarfUtil::sleb128(p, & len);
					p += len;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "advance line to" << current->line;
					break;
				case DW_LNS_const_add_pc:
					current->address += ((255 - op_base) / lrange) * min_insn_length;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "advance pc to" << HEX(current->address);
					break;
				case DW_LNS_set_file:
					current->file = DwarfUtil::uleb128(p, & len);
					p += len;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set file to" << current->file;
					break;
				case DW_LNS_set_column:
					DwarfUtil::panic();
					break;
				case DW_LNS_negate_stmt:
					current->is_stmt = ! current->is_stmt;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set is_stmt to " << current->is_stmt;
					break;
			}
		}
	}
	/* returns -1 if no line number was found */
	uint32_t lineNumberForAddress(uint32_t target_address, uint32_t statememnt_list_offset, uint32_t & file_number)
	{
		header = debug_line + statememnt_list_offset;
		if (version() != 2) DwarfUtil::panic();
		const uint8_t * p(line_number_program()), op_base(opcode_base()), lrange(line_range());
		int lbase(line_base());
		uint32_t min_insn_length(minimum_instruction_length());
		int len, x;
		init();
		if (DEBUG_LINE_PROGRAMS_ENABLED)
		{
			qDebug() << "debug line for offset" << HEX(header - debug_line);
			qDebug() << "include directories table:";
			union { const char * s; const uint8_t * p; } x;
			x.s = include_directories();
			size_t l;
			while ((l = strlen(x.s)))
				qDebug() << QString(x.s), x.s += l + 1;
			qDebug() << "file names table:";
			x.s = file_names();
			while ((l = strlen(x.s)))
			{
				qDebug() << QString(x.s), x.s += l + 1;
				/* skip directory index, file time and file size */
				DwarfUtil::uleb128x(x.p), DwarfUtil::uleb128x(x.p), DwarfUtil::uleb128x(x.p);
			}
		}
		while (p < header + sizeof(uint32_t) + unit_length())
		{
			if (! * p)
			{
				/* extended opcodes */
				len = DwarfUtil::uleb128(++ p, & x);
				p += x;
				if (!len)
					DwarfUtil::panic();
				switch (* p ++)
				{
					default:
						DwarfUtil::panic();
					case DW_LNE_set_discriminator:
						if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set discriminator to" << DwarfUtil::uleb128(p, & x) << "!!! IGNORED !!!";
						if (len != x + 1) DwarfUtil::panic();
						p += x;
						break;
					case DW_LNE_end_sequence:
						if (len != 1) DwarfUtil::panic();
						if (prev->address <= target_address && target_address < current->address)
							return file_number = prev->file, prev->line;
						init();
						if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "end of sequence";
						break;
					case DW_LNE_set_address:
						if (len != 5) DwarfUtil::panic();
						current->address = * (uint32_t *) p;
						p += sizeof current->address;
						if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "extended opcode, set address to" << HEX(current->address);
						break;
				}
			}
			else if (* p >= op_base)
			{
				/* special opcodes */
				uint8_t x = * p ++ - op_base;
				current->address += (x / lrange) * min_insn_length;
				current->line += lbase + x % lrange;
				if (prev->address <= target_address && target_address < current->address)
					return file_number = prev->file, prev->line;
				if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "special opcode, set address to" << HEX(current->address) << "line to" << current->line;
				swap();
				* current = * prev;
			}
			/* standard opcodes */
			else switch (* p ++)
			{
				default:
					DwarfUtil::panic();
					break;
				case DW_LNS_copy:
				/*
					if (xaddr <= target_address && target_address < address)
						DwarfUtil::panic();*/
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "copy";
					if (prev->address <= target_address && target_address < current->address)
						return file_number = prev->file, prev->line;
					swap();
					* current = * prev;
					break;
				case DW_LNS_advance_pc:
					current->address += DwarfUtil::uleb128(p, & len) * min_insn_length;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "advance pc to" << HEX(current->address);
					p += len;
					break;
				case DW_LNS_advance_line:
					current->line += DwarfUtil::sleb128(p, & len);
					p += len;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "advance line to" << current->line;
					break;
				case DW_LNS_const_add_pc:
					current->address += ((255 - op_base) / lrange) * min_insn_length;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "advance pc to" << HEX(current->address);
					break;
				case DW_LNS_set_file:
					current->file = DwarfUtil::uleb128(p, & len);
					p += len;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set file to" << current->file;
					break;
				case DW_LNS_set_column:
					DwarfUtil::panic();
					break;
				case DW_LNS_negate_stmt:
					current->is_stmt = ! current->is_stmt;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set is_stmt to " << current->is_stmt;
					break;
			}
		}
		return file_number = 0, -1;
	}

	void addressesForFile(uint32_t file_number, std::vector<struct lineAddress> & line_addresses)
	{
		if (version() != 2) DwarfUtil::panic();
		const uint8_t * p(line_number_program()), op_base(opcode_base()), lrange(line_range());
		int lbase(line_base());
		uint32_t min_insn_length(minimum_instruction_length());
		int len, x;
		init();
		struct lineAddress line_data;
		if (DEBUG_LINE_PROGRAMS_ENABLED)
		{
			qDebug() << "debug line for offset" << HEX(header - debug_line);
			qDebug() << "include directories table:";
			union { const char * s; const uint8_t * p; } x;
			x.s = include_directories();
			size_t l;
			while ((l = strlen(x.s)))
				qDebug() << QString(x.s), x.s += l + 1;
			qDebug() << "file names table:";
			x.s = file_names();
			while ((l = strlen(x.s)))
			{
				qDebug() << QString(x.s), x.s += l + 1;
				/* skip directory index, file time and file size */
				DwarfUtil::uleb128x(x.p), DwarfUtil::uleb128x(x.p), DwarfUtil::uleb128x(x.p);
			}
		}
		while (p < header + sizeof(uint32_t) + unit_length())
		{
			if (! * p)
			{
				/* extended opcodes */
				len = DwarfUtil::uleb128(++ p, & x);
				p += x;
				if (!len)
					DwarfUtil::panic();
				switch (* p ++)
				{
					default:
						DwarfUtil::panic();
					case DW_LNE_set_discriminator:
						if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set discriminator to" << DwarfUtil::uleb128(p, & x) << "!!! IGNORED !!!";
						if (len != x + 1) DwarfUtil::panic();
						p += x;
						break;
					case DW_LNE_end_sequence:
						if (len != 1) DwarfUtil::panic();
						if (current->file == file_number)
						{
							line_data.address = prev->address;
							line_data.line = current->line;
							line_data.address_span = current->address;
							line_addresses.push_back(line_data);
						}
						init();
						if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "end of sequence";
						break;
					case DW_LNE_set_address:
						if (len != 5) DwarfUtil::panic();
						current->address = * (uint32_t *) p;
						p += sizeof current->address;
						if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "extended opcode, set address to" << HEX(current->address);
						break;
				}
			}
			else if (* p >= op_base)
			{
				/* special opcodes */
				uint8_t x = * p ++ - op_base;
				current->address += (x / lrange) * min_insn_length;
				current->line += lbase + x % lrange;
				if (prev->file == file_number)
				{
					line_data.address = prev->address;
					line_data.line = prev->line;
					line_data.address_span = current->address;
					line_addresses.push_back(line_data);
				}
				if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "special opcode, set address to" << HEX(current->address) << "line to" << current->line;
				swap();
				* current = * prev;
			}
			/* standard opcodes */
			else switch (* p ++)
			{
				default:
					DwarfUtil::panic();
					break;
				case DW_LNS_copy:
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "copy";
					if (prev->file == file_number)
					{
						line_data.address = prev->address;
						line_data.line = prev->line;
						line_data.address_span = current->address;
						line_addresses.push_back(line_data);
					}
					swap();
					* current = * prev;
					break;
				case DW_LNS_advance_pc:
					current->address += DwarfUtil::uleb128(p, & len) * min_insn_length;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "advance pc to" << HEX(current->address);
					p += len;
					break;
				case DW_LNS_advance_line:
					current->line += DwarfUtil::sleb128(p, & len);
					p += len;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "advance line to" << current->line;
					break;
				case DW_LNS_const_add_pc:
					current->address += ((255 - op_base) / lrange) * min_insn_length;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "advance pc to" << HEX(current->address);
					break;
				case DW_LNS_set_file:
					current->file = DwarfUtil::uleb128(p, & len);
					p += len;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set file to" << current->file;
					break;
				case DW_LNS_set_column:
					DwarfUtil::panic();
					break;
				case DW_LNS_negate_stmt:
					current->is_stmt = ! current->is_stmt;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set is_stmt to " << current->is_stmt;
					break;
			}
		}
	}
	/* returns 0 if the file is not found in the file name table of the current line number program */
	uint32_t fileNumber(const char * filename)
	{
		int i(0), len;
		union { const char * s; const uint8_t * p; } x;
		x.s = file_names();
		while ((len = strlen(x.s)))
		{
			if (!strcmp(filename, x.s))
				return i + 1;
			x.s += len + 1, i ++;
			/* skip directory index, file time and file size */
			DwarfUtil::uleb128x(x.p), DwarfUtil::uleb128x(x.p), DwarfUtil::uleb128x(x.p);
		}
		return 0;
		
	}
	void stringsForFileNumber(uint32_t file_number, const char * & file_name, const char * & directory_name)
	{
		file_name = "<<< unknown file >>>";
		directory_name = "<<< unknown directory >>>";
		if (!file_name)
			return;
		size_t l;
		union { const char * s; const uint8_t * p; } f;
		f.s = file_names();
		int i, j(0);
		file_number --;
		for (i = 0; i < file_number; i ++)
		{
			if (!(l = strlen(f.s)))
				return;
			f.s += l + 1;
			/* skip directory index, file time and file size */
			DwarfUtil::uleb128x(f.p), DwarfUtil::uleb128x(f.p), DwarfUtil::uleb128x(f.p);
		}
		file_name = f.s, f.s += strlen(f.s) + 1, i = DwarfUtil::uleb128x(f.p) - 1;
		f.s = include_directories();
		while (j != i && (l = strlen(f.s)))
			f.s += l + 1, j ++;
		if (i == j)
			directory_name = f.s;
	}

	void rewind(void) { header = debug_line; }
	bool next(void) { return (header += ((header != debug_line + debug_line_len) ? sizeof unit_length() + unit_length() : 0)) != debug_line + debug_line_len; }
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

	const uint8_t * debug_ranges;
	uint32_t	debug_ranges_len;
	
	const uint8_t * debug_str;
	uint32_t	debug_str_len;
	
	const uint8_t * debug_line;
	uint32_t	debug_line_len;
	
	const uint8_t * debug_loc;
	uint32_t	debug_loc_len;
public:
	DwarfData(const void * debug_aranges, uint32_t debug_aranges_len, const void * debug_info, uint32_t debug_info_len,
		  const void * debug_abbrev, uint32_t debug_abbrev_len, const void * debug_ranges, uint32_t debug_ranges_len,
		  const void * debug_str, uint32_t debug_str_len,
		  const void * debug_line, uint32_t debug_line_len,
		  const void * debug_loc, uint32_t debug_loc_len)
	{
		this->debug_aranges = (const uint8_t *) debug_aranges;
		this->debug_aranges_len = debug_aranges_len;
		this->debug_info = (const uint8_t *) debug_info;
		this->debug_info_len = debug_info_len;
		this->debug_abbrev = (const uint8_t *) debug_abbrev;
		this->debug_abbrev_len = debug_abbrev_len;
		this->debug_ranges = (const uint8_t *) debug_ranges;
		this->debug_ranges_len = debug_ranges_len;
		this->debug_str = (const uint8_t *) debug_str;
		this->debug_str_len = debug_str_len;
		this->debug_line = (const uint8_t *) debug_line;
		this->debug_line_len = debug_line_len;
		this->debug_loc = (const uint8_t *) debug_loc;
		this->debug_loc_len = debug_loc_len;
	}
	/* returns -1 if the compilation unit is not found */
	uint32_t compilationUnitOffsetForOffsetInDebugInfo(uint32_t debug_info_offset)
	{
		compilation_unit_header h(debug_info);
		while (h.data - debug_info < debug_info_len)
			if (h.data - debug_info <= debug_info_offset && debug_info_offset < h.data - debug_info + h.unit_length())
				return h.data - debug_info;
			else h.next();
		return -1;
	}

	/* returns -1 if the compilation unit is not found */
	uint32_t	get_compilation_unit_debug_info_offset_for_address(uint32_t address)
	{
		int i;
		struct debug_arange arange(debug_aranges);
		while (arange.data < debug_aranges + debug_aranges_len)
		{
			struct debug_arange::compilation_unit_range r;
			if (arange.address_size() != 4)
				DwarfUtil::panic();
			i = 0;
			do
			{
				r = arange.ranges()[i];
				if (r.start_address <= address && address < r.start_address + r.length)
					return arange.compilation_unit_debug_info_offset();
				i ++;
			}
			while (r.length || r.start_address);
			arange.next();
		}
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
	uint32_t next_compilation_unit(uint32_t compilation_unit_offset)
	{
		uint32_t x = compilation_unit_header(debug_info + compilation_unit_offset).next().data - debug_info;
		if (x >= debug_info_len)
			x = -1;
		return x;
	}
	/* first number is the abbreviation code, the second is the offset in .debug_abbrev */
	void get_abbreviations_of_compilation_unit(uint32_t compilation_unit_offset, std::map<uint32_t, uint32_t> & abbreviations)
	{
		const uint8_t * debug_abbrev = this->debug_abbrev + compilation_unit_header((uint8_t *) debug_info + compilation_unit_offset) . debug_abbrev_offset(); 
		uint32_t code, tag, has_children, name, form;
		int len;
		abbreviations.clear();
		while ((code = DwarfUtil::uleb128(debug_abbrev, & len)))
		{
			auto x = abbreviations.find(code);
			if (x != abbreviations.end())
				DwarfUtil::panic("duplicate abbreviation code");
			abbreviations.operator [](code) = debug_abbrev - this->debug_abbrev;
			debug_abbrev += len;
			tag = DwarfUtil::uleb128x(debug_abbrev);
			has_children = DwarfUtil::uleb128x(debug_abbrev);
			do
			{
				name = DwarfUtil::uleb128x(debug_abbrev);
				form = DwarfUtil::uleb128x(debug_abbrev);
			}
			while (name || form);
		}
	}
	/*! \todo	the name of this function is misleading, it really reads a sequence of dies on a same die tree level; that
	 * 		is because of the dwarf die flattenned tree representation */
	std::vector<struct Die> debug_tree_of_die(uint32_t & die_offset, std::map<uint32_t, uint32_t> & abbreviations, int depth = 0)
	{
		std::vector<struct Die> dies;
		const uint8_t * p = debug_info + die_offset;
		int len;
		uint32_t code = DwarfUtil::uleb128(p, & len);
		if (DEBUG_DIE_READ_ENABLED) qDebug() << "at offset " << QString("$%1").arg(p - debug_info, 0, 16);
		p += len;
		if (!code)
			DwarfUtil::panic("null die requested");
		while (code)
		{
			auto x = abbreviations.find(code);
			if (x == abbreviations.end())
				DwarfUtil::panic("abbreviation code not found");
			struct Abbreviation a(debug_abbrev + x->second);
			struct Die die(a.tag(), die_offset, x->second);
			
			std::pair<uint32_t, uint32_t> attr = a.next_attribute();
			while (attr.first)
			{
				p += DwarfUtil::skip_form_bytes(attr.second, p);
				attr = a.next_attribute();
			}
			die_offset = p - debug_info;
			if (a.has_children())
			{
				die.children = debug_tree_of_die(die_offset, abbreviations, depth + 1);
				p = debug_info + die_offset;
			}
			dies.push_back(die);
			
			if (depth == 0)
				return dies;
				
			code = DwarfUtil::uleb128(p, & len);
			if (DEBUG_DIE_READ_ENABLED) qDebug() << "at offset " << QString("$%1").arg(p - debug_info, 0, 16);
			p += len;
		}
		die_offset = p - debug_info;

		return dies;
	}
	uint32_t compilation_unit_base_address(const struct Die & compilation_unit_die)
	{
		struct Abbreviation a(debug_abbrev + compilation_unit_die.abbrev_offset);
		auto low_pc = a.dataForAttribute(DW_AT_low_pc, debug_info + compilation_unit_die.offset);
		if (!low_pc.first)
			DwarfUtil::panic();
		return DwarfUtil::fetchHighLowPC(low_pc.first, low_pc.second);
	}
	bool isAddressInRange(const struct Die & die, uint32_t address, const struct Die & compilation_unit_die)
	{
		struct Abbreviation a(debug_abbrev + die.abbrev_offset);
		auto range = a.dataForAttribute(DW_AT_ranges, debug_info + die.offset);
		if (range.first)
		{
			auto base_address = compilation_unit_base_address(compilation_unit_die);
			const uint32_t * range_list = (const uint32_t *) (debug_ranges + DwarfUtil::formConstant(range.first, range.second));
			while (range_list[0] && range_list[1])
			{
				if (range_list[0] == -1)
					base_address = range_list[1];
				else if (range_list[0] + base_address <= address && address < range_list[1] + base_address)
					return true;
				range_list += 2;
			}
		}
		auto low_pc = a.dataForAttribute(DW_AT_low_pc, debug_info + die.offset);
		if (low_pc.first)
		{
			uint32_t x;
			auto hi_pc = a.dataForAttribute(DW_AT_high_pc, debug_info + die.offset);
			if (!hi_pc.first)
				return false;
			if (DEBUG_ADDRESS_RANGE_ENABLED) qDebug() << (x = DwarfUtil::fetchHighLowPC(low_pc.first, low_pc.second));
			if (DEBUG_ADDRESS_RANGE_ENABLED) qDebug() << DwarfUtil::fetchHighLowPC(hi_pc.first, hi_pc.second, x);
			x = DwarfUtil::fetchHighLowPC(low_pc.first, low_pc.second);
			return x <= address && address < DwarfUtil::fetchHighLowPC(hi_pc.first, hi_pc.second, x);
		}
		return false;
	}
	/*! \todo	!!! this is *gross* inefficient - read and process only immediate die children instead !!! */
	std::vector<struct Die> executionContextForAddress(uint32_t address)
	{
		std::vector<struct Die> context;
		auto cu_die_offset = get_compilation_unit_debug_info_offset_for_address(address);
		std::map<uint32_t, uint32_t> abbreviations;
		get_abbreviations_of_compilation_unit(cu_die_offset, abbreviations);
		if (cu_die_offset == -1)
			return context;
		cu_die_offset += /* discard the compilation unit header */ 11;
		auto debug_tree = debug_tree_of_die(cu_die_offset, abbreviations);
		int i(0);
		std::vector<struct Die> & die_list(debug_tree);
		while (i < die_list.size())
			if (isAddressInRange(die_list.at(i), address, debug_tree.at(0)))
			{
				context.push_back(die_list.at(i));
				i = 0;
				die_list = die_list.at(i).children;
			}
			else
				i ++;
		return context;
	}
	std::vector<struct Die> localDataObjectsForContext(const std::vector<struct Die> & context)
	{
	std::vector<struct Die> locals;
	int i, j, tag;
		for (i = context.size() - 1; i >= 0; i --)
			if ((tag = context.at(i).tag == DW_TAG_subprogram) || tag == DW_TAG_lexical_block)
				for (j = 0; j < context.at(i).children.size(); j ++)
					switch (context.at(i).children.at(j).tag)
					{
						case DW_TAG_variable:
						case DW_TAG_formal_parameter:
							locals.push_back(context.at(i).children.at(j));
					}
		return locals;
	}
	struct SourceCodeCoordinates sourceCodeCoordinatesForAddress(uint32_t address, const struct Die compilation_unit_die)
	{
		SourceCodeCoordinates s;
		uint32_t file_number;
		if (compilation_unit_die.tag != DW_TAG_compile_unit)
			DwarfUtil::panic();
		Abbreviation a(debug_abbrev + compilation_unit_die.abbrev_offset);
		auto x = a.dataForAttribute(DW_AT_stmt_list, debug_info + compilation_unit_die.offset);
		if (!x.first)
			return s;
		class DebugLine l(debug_line, debug_line_len);
		s.line = l.lineNumberForAddress(address, DwarfUtil::formConstant(x.first, x.second), file_number);
		l.stringsForFileNumber(file_number, s.filename, s.directoryname);
		return s;
	}

private:
	uint32_t readTypeOffset(uint32_t attribute_form, const uint8_t * debug_info_bytes, uint32_t compilation_unit_header_offset)
	{
		switch (attribute_form)
		{
		case DW_FORM_addr:
		case DW_FORM_data4:
		case DW_FORM_sec_offset:
			return * (uint32_t *) debug_info_bytes;
		case DW_FORM_ref4:
			return * (uint32_t *) debug_info_bytes + compilation_unit_header_offset;
		default:
			DwarfUtil::panic();
		}
	}
public:
#if 0
	/* the first node in the vector is the head of the type graph
 	 * returns the position in the vector at which the new node is placed */
	int readType(uint32_t die_offset, std::map<uint32_t, uint32_t> & abbreviations, std::vector<struct DwarfTypeNode> & type_cache)
	{
		int index;
		uint32_t saved_die_offset(die_offset);
		qDebug() << "reading type die at offset" << die_offset;
		if (die_offset == 76)
		{
			die_offset = 76;
		}
		auto type = debug_tree_of_die(die_offset, abbreviations);
		if (type.size() != 1)
			DwarfUtil::panic();
		struct DwarfTypeNode node(type.at(0));
		type_cache.push_back(node);
		index = type_cache.size() - 1;
		//struct DwarfTypeNode * pnode = & type_cache.at(index);
		struct Abbreviation a(debug_abbrev + type.at(0).abbrev_offset);
		auto t = a.dataForAttribute(DW_AT_type, debug_info + type_cache.at(index).die.offset);
		if (t.first)
		{
			uint32_t cu_offset(compilationUnitOffsetForOffsetInDebugInfo(saved_die_offset));
			qDebug() << "read type die at index " << (type_cache.at(index).next = readType(readTypeOffset(t.first, t.second, cu_offset), abbreviations, type_cache));
		}
		if (type_cache.at(index).die.children.size())
		{
			int i, * x = & type_cache.at(index).childlist;
			qDebug() << "aggregate type, child count" << type_cache.at(index).die.children.size();
			for (i = 0; i < type_cache.at(index).die.children.size(); i ++)
			{
				qDebug() << "reading type child" << i << ", offset is" << type_cache.at(index).die.children.at(i).offset;
				* x = readType(type_cache.at(index).die.children.at(i).offset, abbreviations, type_cache), x = & type_cache.at(* x).sibling;
			}
		}
		return index;
	}
#endif
int readType(uint32_t die_offset, std::map<uint32_t, uint32_t> & abbreviations, std::vector<struct DwarfTypeNode> & type_cache);
	std::string typeString(const std::vector<struct DwarfTypeNode> & type, bool short_type_print = true, int node_number = 0)
	{
		std::string type_string;
		const struct Die & die(type.at(node_number).die);

		switch (die.tag)
		{
			case DW_TAG_structure_type:
				int j;
				type_string = "struct " + nameOfDie(die) + " ";
				if (!short_type_print)
				{
					type_string += "{\n";
					for (j = type.at(node_number).childlist; j != -1; j = type.at(j).sibling)
						type_string += typeString(type, short_type_print, j);
					type_string += "\n};";
				}
			break;
			case DW_TAG_member:
				type_string = typeString(type, short_type_print, type.at(node_number).next) + nameOfDie(die);
				if (!short_type_print)
					type_string += ";\n";
				break;
			case DW_TAG_volatile_type:
				type_string = "volatile " + typeString(type, short_type_print, type.at(node_number).next);
				break;
			case DW_TAG_typedef:
				if (!short_type_print) type_string += "typedef ";
				type_string += nameOfDie(die) + " ";
				if (!short_type_print)
					type_string += typeString(type, short_type_print, type.at(node_number).next);
				break;
			case DW_TAG_base_type:
			{
				Abbreviation a(debug_abbrev + die.abbrev_offset);
				auto x = a.dataForAttribute(DW_AT_encoding, debug_info + die.offset);
				auto size = a.dataForAttribute(DW_AT_byte_size, debug_info + die.offset);
				if (x.first == 0 || size.first == 0)
					DwarfUtil::panic();
				switch (DwarfUtil::formConstant(x.first, x.second))
				{
					default:
						qDebug() << "unhandled encoding" << DwarfUtil::formConstant(x.first, x.second);
						DwarfUtil::panic();
					case DW_ATE_unsigned:
						switch (DwarfUtil::formConstant(size.first, size.second))
						{
							default:
								DwarfUtil::panic();
							case 1:
								type_string = "unsigned char ";
								break;
							case 2:
								type_string = "unsigned short ";
								break;
							case 4:
								type_string = "unsigned int ";
								break;
						}
						break;
				}
			}
				break;
			case DW_TAG_array_type:
			{
				type_string = typeString(type, short_type_print, type.at(node_number).next) + " ";// + nameOfDie(type.at(node_number).die);
				int i;
				if (die.children.size())
					for (i = 0; i < die.children.size(); i ++)
					{
						Abbreviation a(debug_abbrev + die.children.at(i).abbrev_offset);
						auto subrange = a.dataForAttribute(DW_AT_upper_bound, debug_info + die.children.at(i).offset);
						if (subrange.first == 0)
							continue;
						type_string += "[" + std::to_string(DwarfUtil::formConstant(subrange.first, subrange.second) + 1) + "]";
					}
				else
					type_string += "[]";
			}
				break;
			default:
				qDebug() << "unhandled tag" <<  type.at(node_number).die.tag;
				DwarfUtil::panic();
		}
		return type_string;
	}
	int sizeOf(const std::vector<struct DwarfTypeNode> & type, int node_number = 0)
	{
		Abbreviation a(debug_abbrev + type.at(node_number).die.abbrev_offset);
		auto x = a.dataForAttribute(DW_AT_byte_size, debug_info + type.at(node_number).die.offset);
		if (x.first)
			return DwarfUtil::formConstant(x.first, x.second);
		return sizeOf(type, type.at(node_number).next);
	}
	
	struct DataNode
	{
		std::vector<std::string> data;
		std::vector<struct DataNode> children;
	};
	void dataForType(const std::vector<struct DwarfTypeNode> & type, struct DataNode & node, bool short_type_print = true, int type_node_number = 0)
	{
		struct Die die(type.at(type_node_number).die);
		node.data.push_back(typeString(type, short_type_print, type_node_number));
		switch (die.tag)
		{
			case DW_TAG_structure_type:
				int j;
				for (j = type.at(type_node_number).childlist; j != -1; j = type.at(j).sibling)
					node.children.push_back(DataNode()), dataForType(type, node.children.back(), short_type_print, j);
			break;
			case DW_TAG_member:
				break;
			case DW_TAG_volatile_type:
				break;
			case DW_TAG_typedef:
				break;
			case DW_TAG_base_type:
			
				break;
			case DW_TAG_array_type:
				break;
			default:
				qDebug() << "unhandled tag" <<  type.at(type_node_number).die.tag;
				DwarfUtil::panic();
		}
	}

	std::string nameOfDie(const struct Die & die)
	{
		struct Abbreviation a(debug_abbrev + die.abbrev_offset);
		auto x = a.dataForAttribute(DW_AT_name, debug_info + die.offset);
		if (!x.first)
			return "<<< no name >>>";
		return DwarfUtil::formString(x.first, x.second, debug_str);
	}
	void dumpLines(void)
	{
		class DebugLine l(debug_line, debug_line_len);
		l.dump();
		while (l.next())
			l.dump();
	}
	void addressesForFile(const char * filename, std::vector<struct DebugLine::lineAddress> & line_addresses)
	{
		class DebugLine l(debug_line, debug_line_len);
		do
		{
			auto x(l.fileNumber(filename));
			if (x)
				l.addressesForFile(x, line_addresses);
		}
		while (l.next());
	}

private:
	void fillStaticObjectDetails(const struct Die & die, struct StaticObject & x)
	{
		Abbreviation a(debug_abbrev + die.abbrev_offset);
		auto t = a.dataForAttribute(DW_AT_decl_file, debug_info + die.offset);
		x.file = ((t.first) ? DwarfUtil::formConstant(t.first, t.second) : -1);
		t = a.dataForAttribute(DW_AT_decl_line, debug_info + die.offset);
		x.line = ((t.first) ? DwarfUtil::formConstant(t.first, t.second) : -1);
		t = a.dataForAttribute(DW_AT_name, debug_info + die.offset);
		if (t.first)
		{
			switch (t.first)
			{
			case DW_FORM_string:
				x.name = (const char *) t.second;
				break;
			case DW_FORM_strp:
				x.name = (const char *) (debug_str + * (uint32_t *) t.second);
				break;
			default:
				DwarfUtil::panic();
			}
		}
		else
			x.name = 0;
		x.die_offset = die.offset;
	}
	void reapStaticObjects(std::vector<struct StaticObject> & data_objects,
	                       std::vector<struct StaticObject> & subprograms,
	                       const struct Die & die,
	                       const std::map<uint32_t, uint32_t> & abbreviations)
	{
		int i;
		if (DwarfUtil::isDataObject(die.tag))
		{
			Abbreviation a(debug_abbrev + die.abbrev_offset);
			auto x = a.dataForAttribute(DW_AT_location, debug_info + die.offset);
			if (x.first && DwarfUtil::isLocationConstant(x.first, x.second))
			{
				StaticObject x;
				fillStaticObjectDetails(die, x);
				data_objects.push_back(x);
			}
		}
		else if (DwarfUtil::isSubprogram(die.tag))
		{
			Abbreviation a(debug_abbrev + die.abbrev_offset);
			auto x = a.dataForAttribute(DW_AT_low_pc, debug_info + die.offset);
			if (x.first || a.dataForAttribute(DW_AT_ranges, debug_info + die.offset).first)
			{
				StaticObject x;
				fillStaticObjectDetails(die, x);
				subprograms.push_back(x);
			}
		}
		for (i = 0; i < die.children.size(); i ++)
			reapStaticObjects(data_objects, subprograms, die.children.at(i), abbreviations);
	}

public:
	void reapStaticObjects(std::vector<struct StaticObject> & data_objects, std::vector<struct StaticObject> & subprograms)
	{
		uint32_t cu;
		for (cu = 0; cu != -1; cu = next_compilation_unit(cu))
		{
			auto die_offset = cu + 11;
			std::map<uint32_t, uint32_t> abbreviations;
			get_abbreviations_of_compilation_unit(cu, abbreviations);
			auto dies = debug_tree_of_die(die_offset, abbreviations);
			reapStaticObjects(data_objects, subprograms, dies.at(0), abbreviations);
		}
	}
};

class DwarfUnwinder
{
private:
	const uint8_t	* debug_frame;
	uint32_t	debug_frame_len;
	struct CIEFDE
	{
	private:
		const uint8_t * data, * debug_frame;
		uint32_t debug_frame_len;
	public:
		bool isFDE(void) { return * (uint32_t *) (data + 4) != 0xffffffff; }
		bool isCIE(void) { return * (uint32_t *) (data + 4) == 0xffffffff; }
		uint32_t length(void) { return * (uint32_t *) data; }
		uint32_t CIE_pointer(void) { return * (uint32_t *) (data + 4); }
		uint32_t initial_location(void) { return * (uint32_t *) (data + 8); }
		uint32_t address_range(void) { return * (uint32_t *) (data + 12); }
		uint8_t version(void) { return data[8]; }
		
		/* pointer to the initial instructions, and bytecount in instruction block */
		std::pair<const uint8_t *, int> fde_instructions(void) { return std::pair<const uint8_t *, int>(data + 16, length() - 12); }
		
		const char * augmentation(void) { return (const char *) data + 9; }
		CIEFDE(const uint8_t * debug_frame, uint32_t debug_frame_len, uint32_t debug_frame_offset)
		{
			this->debug_frame = debug_frame;
			this->debug_frame_len = debug_frame_len;
			data = debug_frame + debug_frame_offset;
			if (isCIE() && version() != 1)
				DwarfUtil::panic("unsupported .debug_frame version");
			if (isCIE() && strlen(augmentation()))
				DwarfUtil::panic("unsupported .debug_frame CIE augmentation");
		}
		uint32_t code_alignment_factor(void)
		{
			if (!isCIE())
				DwarfUtil::panic("");
			return DwarfUtil::uleb128(data + 9 + strlen(augmentation()) + 1);
		}
		int32_t data_alignment_factor(void)
		{
			int len, offset;
			if (!isCIE())
				DwarfUtil::panic("");
			return DwarfUtil::uleb128(data + (offset = 9 + strlen(augmentation()) + 1), & len), DwarfUtil::sleb128(data + offset + len);
		}
		uint8_t return_address_register(void)
		{
			int len, offset;
			if (!isCIE())
				DwarfUtil::panic("");
			return DwarfUtil::uleb128(data + (offset = 9 + strlen(augmentation()) + 1), & len)
					, DwarfUtil::sleb128(data + (offset += len), & len)
					, data[offset + len];
		}
		/* pointer to the initial instructions, and bytecount in instruction block */
		std::pair<const uint8_t *, int> initial_instructions(void)
		{
			std::pair<const uint8_t *, int> x;
			int len, offset;
			if (!isCIE())
				DwarfUtil::panic("");
			x = std::pair<const uint8_t *, int> (
			data + (offset = (DwarfUtil::uleb128(data + (offset = 9 + strlen(augmentation()) + 1), & len)
					, DwarfUtil::sleb128(data + (offset += len), & len)
					, offset + len)
					+ 1),
				0
				);
			x.second = length() + sizeof(uint32_t) - offset;
			return x;
		}
		/* pointer to the initial instructions, and bytecount in instruction block */
		std::pair<const uint8_t *, int> instructions(void) { return isCIE() ? initial_instructions() : fde_instructions(); }
		std::string ciefde_sforth_code(void)
		{
			const uint8_t * insn = instructions().first;
			int i = instructions().second, len;
			uint8_t dwopcode;
			uint32_t op;
			std::stringstream result;
			while (i --)
			{
				switch (((dwopcode = * insn ++) >> 6))
				{
					case 0:
						break;
					case 1:
						result << (dwopcode & ((1 << 6) - 1)) << " " << "DW_CFA_advance_loc" << " ";
						continue;
					case 2:
						op = DwarfUtil::uleb128(insn, & len);
						insn += len;
						i -= len;
						result << (dwopcode & ((1 << 6) - 1)) << " " << op << " " << "DW_CFA_offset" << " ";
						continue;
					case 3:
						result << (dwopcode & ((1 << 6) - 1)) << " " << "DW_CFA_restore" << " ";
						continue;
					default:
					DwarfUtil::panic();
				}

				switch (dwopcode)
				{
					default:
					DwarfUtil::panic();
					case DW_CFA_def_cfa:
						op = DwarfUtil::uleb128(insn, & len);
						insn += len;
						i -= len;
						result << op << " ";
						op = DwarfUtil::uleb128(insn, & len);
						insn += len;
						i -= len;
						result << op << " ";
						result << "DW_CFA_def_cfa" << " ";
						break;
					case DW_CFA_def_cfa_offset:
						op = DwarfUtil::uleb128(insn, & len);
						insn += len;
						i -= len;
						result << op << " " << "DW_CFA_def_cfa_offset" << " ";
						break;
					case DW_CFA_def_cfa_register:
						op = DwarfUtil::uleb128(insn, & len);
						insn += len;
						i -= len;
						result << op << " " << "DW_CFA_def_cfa_register" << " ";
						break;
					case DW_CFA_nop:
						break;
				}
			}
			return result.str();
		}

		void dump(void)
		{
			if (UNWIND_DEBUG_ENABLED) qDebug() << (isCIE() ? "CIE" : "FDE");
			if (isCIE())
			{
				if (UNWIND_DEBUG_ENABLED) qDebug() << "version " << version();
				if (UNWIND_DEBUG_ENABLED) qDebug() << "code alignment factor " << code_alignment_factor();
				if (UNWIND_DEBUG_ENABLED) qDebug() << "data alignment factor " << data_alignment_factor();
				if (UNWIND_DEBUG_ENABLED) qDebug() << "return address register " << return_address_register();
				auto insn = initial_instructions();
				//qDebug() << "initial instructions " << QByteArray((const char *) insn.first, insn.second).toHex() << "count" << insn.second;
				if (UNWIND_DEBUG_ENABLED) qDebug() << "initial instructions " << QString().fromStdString(ciefde_sforth_code());
			}
			else
			{
				if (UNWIND_DEBUG_ENABLED) qDebug() << "initial address " << QString("$%1").arg(initial_location(), 0, 16);
				if (UNWIND_DEBUG_ENABLED) qDebug() << "range " << address_range();
				if (UNWIND_DEBUG_ENABLED) qDebug() << "instructions " << QString().fromStdString(ciefde_sforth_code());
			}
		}
		void next(void) { if (data != debug_frame + debug_frame_len) data += sizeof(uint32_t) + length(); }
		bool atEnd(void) { return (data == debug_frame + debug_frame_len) ? true : false; }
		void rewind(void) { data = debug_frame; }
		/* return -1 if a fde for the given address is not found */
		uint32_t fdeForAddress(uint32_t address)
		{
			rewind();
			while (!atEnd())
				if (isFDE() && initial_location() <= address && address < initial_location() + address_range())
					return data - debug_frame;
				else
					next();
			return -1;
		}
	};
	
	struct CIEFDE ciefde;

public:
	DwarfUnwinder(const void * debug_frame, uint32_t debug_frame_len) : ciefde((const uint8_t *) debug_frame, debug_frame_len, 0) { this->debug_frame = (const uint8_t *) debug_frame, this->debug_frame_len = debug_frame_len; }
	void dump(void) { ciefde.dump(); }
	void next(void) { ciefde.next(); }
	bool at_end(void) { return ciefde.atEnd(); }
	void rewind(void) { ciefde.rewind(); }
	/* the string in the pair is the sforth dwarf unwind code, the integer is the base address for the unwind code */
	std::pair<std::string, uint32_t> sforthCodeForAddress(uint32_t address)
	{
		uint32_t fde_offset(ciefde.fdeForAddress(address));
		if (fde_offset == -1) 
			return std::pair<std::string, uint32_t>("abort", -1);
		CIEFDE fde(debug_frame, debug_frame_len, fde_offset), cie(debug_frame, debug_frame_len, fde.CIE_pointer());

		std::stringstream sfcode;
		sfcode
				<< cie.code_alignment_factor() << " to code-alignment-factor "
				<< cie.data_alignment_factor() << " to data-alignment-factor "
				<< (cie.ciefde_sforth_code() + fde.ciefde_sforth_code()) << " unwinding-rules-defined ";
		return std::pair<std::string, uint32_t>(sfcode.str(), fde.initial_location());
	}
};

class CompilationUnit : Die
{
public:
};

