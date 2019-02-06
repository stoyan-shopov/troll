/*
Copyright (c) 2016-2017 stoyan shopov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef __LIBTROLL_HXX__
#define __LIBTROLL_HXX__

#include <dwarf.h>
#include <stdint.h>
#include <map>
#include <vector>
#include <sstream>
#include <QDebug>

#define HEX(x) QString("$%1").arg(x, 8, 16, QChar('0'))

#define DEBUG_LINE_PROGRAMS_ENABLED	0
#define DEBUG_DIE_READ_ENABLED		0
#define DEBUG_ADDRESS_RANGE_ENABLED	0
#define UNWIND_DEBUG_ENABLED		0
#define DWARF_EXPRESSION_TESTS_DEBUG_ENABLED		0

#define STATS_ENABLED			1

class DwarfUtil
{
public:
	struct attribute_data
	{
		uint32_t	form;
		const uint8_t	* debug_info_bytes;
		/* Note: this is a pointer that points to the bytes immediately after the form bytes for this abbreviation.
		 * This is needed, because in dwarf 5 there is the new attribute form 'DW_FORM_implicit_const', which
		 * stores the constant value in the '.debug_abbrev' section itself, and not in the '.debug_info' section.
		 * The data bytes for attributes of form 'DW_FORM_implicit_const' are located immediately after the
		 * abbreviation form bytes in the '.debug_abbrev' section. */
		const uint8_t	* debug_abbrev_bytes;
	};
	static void panic(...) {*(int*)0=0;}
	static uint32_t uleb128(const uint8_t * data, int * decoded_len)
	{
		uint64_t result = 0;
		uint8_t x;
		int shift = * decoded_len = 0;
		do x = * data ++, result |= ((x & 0x7f) << shift), shift += 7, (* decoded_len) ++; while (x & 0x80);
		/*
		if (result > 0xffffffff)
			panic();
			*/
		return result;
	}
	static uint32_t uleb128(const uint8_t * data)
	{
		uint64_t result = 0;
		uint8_t x;
		int shift = 0;
		do x = * data ++, result |= ((x & 0x7f) << shift), shift += 7; while (x & 0x80);
		/*
		if (result > 0xffffffff)
			panic();
			*/
		return result;
	}
	static uint32_t uleb128x(const uint8_t * & data)
	{
		uint64_t result = 0;
		uint8_t x;
		int shift = 0;
		do x = * data ++, result |= ((x & 0x7f) << shift), shift += 7; while (x & 0x80);
		/*
		if (result > 0xffffffff)
			panic();
			*/
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
	static int32_t sleb128x(const uint8_t * & data)
	{
		uint64_t result = 0;
		uint8_t x;
		int shift = 0;
		do x = * data ++, result |= ((x & 0x7f) << shift), shift += 7; while (x & 0x80);
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
	static int skip_form_bytes(int form, const uint8_t * debug_info_bytes)
	{
		int bytes_to_skip;
		switch (form)
		{
		default:
			panic();
		case DW_FORM_implicit_const:
			return 0;
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
		{
			int x = uleb128(debug_info_bytes, & bytes_to_skip);
			return x + bytes_to_skip;
		}
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
	static uint32_t formConstant(const attribute_data & a)
	{
		switch (a.form)
		{
		case DW_FORM_implicit_const:
			return sleb128(a.debug_abbrev_bytes);
		case DW_FORM_udata:
			return uleb128(a.debug_info_bytes);
		case DW_FORM_sdata:
			return sleb128(a.debug_info_bytes);
		case DW_FORM_addr:
		case DW_FORM_data4:
		case DW_FORM_sec_offset:
			return * (uint32_t *) a.debug_info_bytes;
		case DW_FORM_data1:
			return * (uint8_t *) a.debug_info_bytes;
		case DW_FORM_data2:
			return * (uint16_t *) a.debug_info_bytes;
		default:
			panic();
		}
	}
	static uint32_t formReference(uint32_t attribute_form, const uint8_t * debug_info_bytes, uint32_t compilation_unit_header_offset)
	{
		switch (attribute_form)
		{
		case DW_FORM_ref_addr:
			qDebug() << "!!! referenced die is in another compilation unit; must update abbreviation cache !!!";
			return * (uint32_t *) debug_info_bytes;
		case DW_FORM_ref4:
			return * (uint32_t *) debug_info_bytes + compilation_unit_header_offset;
		default:
			panic();
		}
	}
	static const char * formString(uint32_t attribute_form, const uint8_t * debug_info_bytes, const uint8_t * debug_str)
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
	static bool isLocationConstant(uint32_t location_attribute_form, const uint8_t * debug_info_bytes, uint32_t & address)
	{
		switch (location_attribute_form)
		{
			int len;
			case DW_FORM_block1:
				len = * debug_info_bytes, debug_info_bytes ++; if (0)
			case DW_FORM_block2:
				len = * (uint16_t *) debug_info_bytes, debug_info_bytes += 2; if (0)
			case DW_FORM_block4:
				len = * (uint32_t *) debug_info_bytes, debug_info_bytes += 4; if (0)
			case DW_FORM_block:
			case DW_FORM_exprloc:
				len = DwarfUtil::uleb128x(debug_info_bytes);
				if (len == 5 && * debug_info_bytes == DW_OP_addr)
				{
					address = * (uint32_t *) (debug_info_bytes + 1);
					return true;
				}
		}
		return false;
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
	Die(){ tag = offset = abbrev_offset = 0; }
	bool isSubprogram(void) const { return tag == DW_TAG_subprogram || tag == DW_TAG_inlined_subroutine; }
	bool isInlinedSubprogram(void) const { return tag == DW_TAG_inlined_subroutine; }
	bool isNonInlinedSubprogram(void) const { return tag == DW_TAG_subprogram; }
	bool isDataObject(void) const { return tag == DW_TAG_variable || tag == DW_TAG_formal_parameter; }
	bool isLexicalBlock(void) const { return tag == DW_TAG_lexical_block; }
};

/* !!! warning - this can generally be a circular graph - beware of recursion when processing !!! */
struct DwarfTypeNode
{
	struct Die	die;
	int		next;
	/* flags to facilitate recursion when processing this data structure for various purposes */
	bool		processed;
	bool		type_print_recursion_flag;

	std::vector<uint32_t>	children;
	std::vector<uint32_t>	array_dimensions;
        DwarfTypeNode(const struct Die & xdie) : die(xdie)	{ next = -1; type_print_recursion_flag = processed = false; }
};

struct Abbreviation
{
private:
	/*! \todo These need not be in a structure, remove the structure. */
	struct
	{
		const uint8_t	* attributes, * init_attributes, * abbreviation_start;
		uint32_t code, tag;
		bool has_children;
		int byteSize = -1;
	} s;
public:
	uint32_t tag(void){ return s.tag; }
	bool has_children(void){ return s.has_children; }

	struct abbreviation_name_and_form
	{
		uint32_t	name;
		uint32_t	form;
		/* This points to the first byte after the bytes of the attribute form.
		 * This information is needed for properly handling some attribute forms,
		 * such as 'DW_FORM_implicit_const' (introduced in dwarf 5). */
		const uint8_t	* afterform_data;
	};

	abbreviation_name_and_form next_attribute(void)
	{
		const uint8_t * p = s.attributes;
		abbreviation_name_and_form a;
		a.name = DwarfUtil::uleb128x(p);
		a.form = DwarfUtil::uleb128x(p);
		/* !!! insidious special cases !!! */
		if (a.form == DW_FORM_indirect)
			DwarfUtil::panic();
		a.afterform_data = p;
		if (a.form == DW_FORM_implicit_const)
			/* skip the special third part present in the attribute specification */
			DwarfUtil::sleb128x(p);
		if (a.form || a.name)
			/* not the last attribute - skip to the next one */
			s.attributes = p;
		else
			/* Last attribute reached - at this point the size of this abbreviation is known,
			 * so update the size field. */
			s.byteSize = p - s.abbreviation_start;

		return a;
	}
	uint32_t code(void) { return s.code; }
	int byteSize(void) { return s.byteSize; }

	Abbreviation(const uint8_t * abbreviation_data)
	{
		s.abbreviation_start = abbreviation_data;
		s.code = DwarfUtil::uleb128x(abbreviation_data);
		s.tag = DwarfUtil::uleb128x(abbreviation_data);
		if (s.code)
		{
			s.has_children = (DwarfUtil::uleb128x(abbreviation_data) == DW_CHILDREN_yes);
			s.init_attributes = s.attributes = abbreviation_data;
		}
		else
			s.init_attributes = s.attributes = 0;
	}

	struct DwarfUtil::attribute_data dataForAttribute(uint32_t attribute_name, const uint8_t * debug_info_data_for_die)
	{
		s.attributes = s.init_attributes;
		abbreviation_name_and_form a;
		/* skip the die abbreviation code */
		DwarfUtil::uleb128x(debug_info_data_for_die);
		while (1)
		{
			a = next_attribute();
			if (!a.name)
				return (DwarfUtil::attribute_data) { 0, 0, 0, };
			if (a.name == attribute_name)
				return (DwarfUtil::attribute_data) { .form = a.form, .debug_info_bytes = debug_info_data_for_die, .debug_abbrev_bytes = a.afterform_data, };
			debug_info_data_for_die += DwarfUtil::skip_form_bytes(a.form, debug_info_data_for_die);
		}
	}
};

struct debug_arange
{
	const uint8_t	* data, * init_data;
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
	struct compilation_unit_range * ranges(void) const {return (struct compilation_unit_range *) (data+16);}
	debug_arange(const uint8_t * data) { this->data = init_data = data; }
	struct debug_arange & next(void) { data += unit_length() + sizeof unit_length(); return * this; }
	void rewind(void) { this->data = init_data; }
};

struct compilation_unit_header
{
	const uint8_t		* data;
	uint32_t	unit_length() const
	{
		uint32_t x = *(uint32_t*)(data+0);
		if (x >= 0xfffffff0) /* probably 64 bit elf, not supported at this time */
			DwarfUtil::panic();
		return x;
	}
	uint16_t	version() const {return*(uint16_t*)(data+4);}
	uint32_t	debug_abbrev_offset() const
	{
		switch (version())
		{
		default: DwarfUtil::panic();
		case 2: case 3: case 4:
			return*(uint32_t*)(data+6);
		case 5:
			return*(uint32_t*)(data+8);
		}
	}
	uint8_t		address_size() const
	{
		switch (version())
		{
		default: DwarfUtil::panic();
		case 2: case 3: case 4:
			return*(uint8_t*)(data+10);
		case 5:
			return*(uint8_t*)(data+7);
		}
	}
	uint8_t		unit_type() const { /* introduced in dwarf 5 */ if (version() != 5) DwarfUtil::panic(); return*(uint8_t*)(data+6);}
	uint32_t	header_length() const
	{
		switch (version())
		{
		default: DwarfUtil::panic();
		case 2: case 3: case 4:
			return 11;
		case 5:
			if (unit_type() == DW_UT_compile || unit_type() == DW_UT_partial)
			return 12;
		}
	}

	compilation_unit_header(const uint8_t * data) { this->data = data; validate(); }
	struct compilation_unit_header & next(void) { data += unit_length() + sizeof unit_length(); return * this; }
private:
	void validate(void) const
	{
		if (!data)
			/* allow the creation of null compilation unit headers */
			return;
		if (unit_length() >= 0xfffffff0)
			/* probably 64 bit elf, not supported at this time */
			DwarfUtil::panic();
		switch (version())
		{
		default: DwarfUtil::panic();
		case 2: case 3: case 4:
			/* dwarf 3 and 4 supported */
			break;
		case 5:
			/* dwarf 5 partially supported */
			if (unit_type() != DW_UT_compile)
				DwarfUtil::panic();
			break;
		}
	}
};

struct StaticObject
{
	const char *	name;
	int		file, line;
	uint32_t	die_offset;
	uint32_t	address;
};

struct SourceCodeCoordinates
{
	const char	* file_name, * call_file_name;
	const char	* directory_name, * call_directory_name;
	const char	* compilation_directory_name;
	uint32_t	address;
	uint32_t	line, call_line;
	SourceCodeCoordinates(void) { file_name = "<<< unknown filename >>>", call_file_name = directory_name = call_directory_name = compilation_directory_name = "<<< unknown >>>", address = line = call_line = -1; }
};

struct LocationList
{
	static const uint8_t * locationExpressionForAddress(const uint8_t * debug_loc, uint32_t debug_loc_offset,
		uint32_t compilation_unit_base_address, uint32_t address_for_location)
	{
		const uint32_t * p((const uint32_t *)(debug_loc + debug_loc_offset));
		while (* p || p[1])
		{
			if (* p == 0xffffffff)
			{
				compilation_unit_base_address = 1[p], p += 2;
				continue;
			}
			else if (p[0] + compilation_unit_base_address <= address_for_location && address_for_location < p[1] + compilation_unit_base_address)
				return (const uint8_t *) (p + 2);
			p += 2;
			p = (const uint32_t *)((uint8_t *) p + * (uint16_t *) p + 2);
		}
		return 0;
	}
};
struct DwarfExpression
{
	static std::string sforthCode(const uint8_t * dwarf_expression, uint32_t expression_len)
	{
		std::stringstream x;
		int bytes_to_skip, i, branch_counter = 0x10000;
		while (expression_len --)
		{
			bytes_to_skip = 0;
			switch (* dwarf_expression ++)
			{
				case DW_OP_addr:
					x << * ((uint32_t *) dwarf_expression) << " DW_OP_addr ";
					bytes_to_skip = sizeof(uint32_t);
					break;
				case DW_OP_fbreg:
					x << DwarfUtil::sleb128(dwarf_expression, & bytes_to_skip) << " DW_OP_fbreg ";
					break;
			{
				int register_number;
				case DW_OP_reg0: register_number = 0; if (0)
				case DW_OP_reg1: register_number = 1; if (0)
				case DW_OP_reg2: register_number = 2; if (0)
				case DW_OP_reg3: register_number = 3; if (0)
				case DW_OP_reg4: register_number = 4; if (0)
				case DW_OP_reg5: register_number = 5; if (0)
				case DW_OP_reg6: register_number = 6; if (0)
				case DW_OP_reg7: register_number = 7; if (0)
				case DW_OP_reg8: register_number = 8; if (0)
				case DW_OP_reg9: register_number = 9; if (0)
				case DW_OP_reg10: register_number = 10; if (0)
				case DW_OP_reg11: register_number = 11; if (0)
				case DW_OP_reg12: register_number = 12; if (0)
				case DW_OP_reg13: register_number = 13; if (0)
				case DW_OP_reg14: register_number = 14; if (0)
				case DW_OP_reg15: register_number = 15; if (0)
				case DW_OP_reg16: register_number = 16; if (0)
				case DW_OP_reg17: register_number = 17; if (0)
				case DW_OP_reg18: register_number = 18; if (0)
				case DW_OP_reg19: register_number = 19; if (0)
				case DW_OP_reg20: register_number = 20; if (0)
				case DW_OP_reg21: register_number = 21; if (0)
				case DW_OP_reg22: register_number = 22; if (0)
				case DW_OP_reg23: register_number = 23; if (0)
				case DW_OP_reg24: register_number = 24; if (0)
				case DW_OP_reg25: register_number = 25; if (0)
				case DW_OP_reg26: register_number = 26; if (0)
				case DW_OP_reg27: register_number = 27; if (0)
				case DW_OP_reg28: register_number = 28; if (0)
				case DW_OP_reg29: register_number = 29; if (0)
				case DW_OP_reg30: register_number = 30; if (0)
				case DW_OP_reg31: register_number = 31; if (0)
				case DW_OP_regx: register_number = DwarfUtil::uleb128(dwarf_expression, & bytes_to_skip);
					x << register_number << " DW_OP_regx ";
					break;
			}
			{
				int register_number;
				case DW_OP_breg0: register_number = 0; if (0)
				case DW_OP_breg1: register_number = 1; if (0)
				case DW_OP_breg2: register_number = 2; if (0)
				case DW_OP_breg3: register_number = 3; if (0)
				case DW_OP_breg4: register_number = 4; if (0)
				case DW_OP_breg5: register_number = 5; if (0)
				case DW_OP_breg6: register_number = 6; if (0)
				case DW_OP_breg7: register_number = 7; if (0)
				case DW_OP_breg8: register_number = 8; if (0)
				case DW_OP_breg9: register_number = 9; if (0)
				case DW_OP_breg10: register_number = 10; if (0)
				case DW_OP_breg11: register_number = 11; if (0)
				case DW_OP_breg12: register_number = 12; if (0)
				case DW_OP_breg13: register_number = 13; if (0)
				case DW_OP_breg14: register_number = 14; if (0)
				case DW_OP_breg15: register_number = 15; if (0)
				case DW_OP_breg16: register_number = 16; if (0)
				case DW_OP_breg17: register_number = 17; if (0)
				case DW_OP_breg18: register_number = 18; if (0)
				case DW_OP_breg19: register_number = 19; if (0)
				case DW_OP_breg20: register_number = 20; if (0)
				case DW_OP_breg21: register_number = 21; if (0)
				case DW_OP_breg22: register_number = 22; if (0)
				case DW_OP_breg23: register_number = 23; if (0)
				case DW_OP_breg24: register_number = 24; if (0)
				case DW_OP_breg25: register_number = 25; if (0)
				case DW_OP_breg26: register_number = 26; if (0)
				case DW_OP_breg27: register_number = 27; if (0)
				case DW_OP_breg28: register_number = 28; if (0)
				case DW_OP_breg29: register_number = 29; if (0)
				case DW_OP_breg30: register_number = 30; if (0)
				case DW_OP_breg31: register_number = 31; if (0)
				case DW_OP_bregx: register_number = DwarfUtil::uleb128(dwarf_expression, & bytes_to_skip);
					x << DwarfUtil::sleb128(dwarf_expression + bytes_to_skip, & i) << " " << register_number << " DW_OP_bregx ";
					bytes_to_skip += i;
					break;
			}
			{
				int literal;
				case DW_OP_lit0: literal = 0; if (0)
				case DW_OP_lit1: literal = 1; if (0)
				case DW_OP_lit2: literal = 2; if (0)
				case DW_OP_lit3: literal = 3; if (0)
				case DW_OP_lit4: literal = 4; if (0)
				case DW_OP_lit5: literal = 5; if (0)
				case DW_OP_lit6: literal = 6; if (0)
				case DW_OP_lit7: literal = 7; if (0)
				case DW_OP_lit8: literal = 8; if (0)
				case DW_OP_lit9: literal = 9; if (0)
				case DW_OP_lit10: literal = 10; if (0)
				case DW_OP_lit11: literal = 11; if (0)
				case DW_OP_lit12: literal = 12; if (0)
				case DW_OP_lit13: literal = 13; if (0)
				case DW_OP_lit14: literal = 14; if (0)
				case DW_OP_lit15: literal = 15; if (0)
				case DW_OP_lit16: literal = 16; if (0)
				case DW_OP_lit17: literal = 17; if (0)
				case DW_OP_lit18: literal = 18; if (0)
				case DW_OP_lit19: literal = 19; if (0)
				case DW_OP_lit20: literal = 20; if (0)
				case DW_OP_lit21: literal = 21; if (0)
				case DW_OP_lit22: literal = 22; if (0)
				case DW_OP_lit23: literal = 23; if (0)
				case DW_OP_lit24: literal = 24; if (0)
				case DW_OP_lit25: literal = 25; if (0)
				case DW_OP_lit26: literal = 26; if (0)
				case DW_OP_lit27: literal = 27; if (0)
				case DW_OP_lit28: literal = 28; if (0)
				case DW_OP_lit29: literal = 29; if (0)
				case DW_OP_lit30: literal = 30; if (0)
				case DW_OP_lit31: literal = 31;
					x << literal << " ";
					break;
			}
				case DW_OP_stack_value:
					x << "DW_OP_stack_value ";
					break;
				case DW_OP_GNU_entry_value:
					i = DwarfUtil::uleb128(dwarf_expression, & bytes_to_skip);
					x << "( disassembled entry value expression: DW_OP_GNU_entry_value-start " << sforthCode(dwarf_expression + bytes_to_skip, i) << " DW_OP_GNU_entry_value-end ) ";
					for (int j = 0; j < i; j ++)
						x << (unsigned) dwarf_expression[bytes_to_skip + j] << " ";
					x << i << " entry-value-expression-block ";
					bytes_to_skip += i;
					break;
				case DW_OP_call_frame_cfa:
					x << "DW_OP_call_frame_cfa ";
					break;
				case DW_OP_mul:
					x << "DW_OP_mul ";
					break;
				case DW_OP_and:
					x << "DW_OP_and ";
					break;
				case DW_OP_shl:
					x << "DW_OP_shl ";
					break;
				case DW_OP_shr:
					x << "DW_OP_shr ";
					break;
				case DW_OP_shra:
					x << "DW_OP_shra ";
					break;
				case DW_OP_ne:
					x << "DW_OP_ne ";
					break;
				case DW_OP_const1u:
					x << (unsigned)(* ((uint8_t *) dwarf_expression)) << " ";
					bytes_to_skip = sizeof(uint8_t);
					break;
				case DW_OP_const2u:
					x << * ((uint16_t *) dwarf_expression) << " ";
					bytes_to_skip = sizeof(uint16_t);
					break;
				case DW_OP_plus_uconst:
					x << DwarfUtil::uleb128(dwarf_expression, & bytes_to_skip);
					x << " DW_OP_plus_uconst ";
					break;
				case DW_OP_plus:
					x << "DW_OP_plus ";
					break;
				case DW_OP_minus:
					x << "DW_OP_minus ";
					break;
				case DW_OP_not:
					x << "DW_OP_not ";
					break;
				case DW_OP_deref:
					x << "DW_OP_deref ";
					break;
				case DW_OP_neg:
					x << "DW_OP_neg ";
					break;
				case DW_OP_dup:
					x << "DW_OP_dup ";
					break;
				case DW_OP_swap:
					x << "DW_OP_swap ";
					break;
				case DW_OP_over:
					x << "DW_OP_over ";
					break;
				case DW_OP_drop:
					x << "DW_OP_drop ";
					break;
				case DW_OP_or:
					x << "OR-UNSUPPORTED!!! ";
					break;
				case DW_OP_xor:
					x << "XOR-UNSUPPORTED!!! ";
					break;
				case DW_OP_lt:
					x << "DW_OP_lt ";
					break;
				case DW_OP_eq:
					x << "EQ-UNSUPPORTED!!! ";
					break;
				case DW_OP_mod:
					x << "MOD-UNSUPPORTED!!! ";
					break;
				case DW_OP_div:
					x << "DIV-UNSUPPORTED!!! ";
					break;
				case DW_OP_deref_size:
					x << "DEREF-SIZE-UNSUPPORTED!!! ";
					bytes_to_skip = sizeof(uint8_t);
					break;
				case DW_OP_const1s:
					x << (int32_t) * ((int8_t *) dwarf_expression) << " ";
					bytes_to_skip = sizeof(int8_t);
					break;
				case DW_OP_const2s:
					x << "CONST2S-UNSUPPORTED!!! ";
					bytes_to_skip = sizeof(uint16_t);
					break;
				case DW_OP_const4u:
					x << * ((uint32_t *) dwarf_expression) << " ";
					bytes_to_skip = sizeof(uint32_t);
					break;
				case DW_OP_GNU_parameter_ref:
				/* I found this:
				 *
				 * https://lists.fedorahosted.org/pipermail/elfutils-devel/2012-July/002389.html
				 * DW_OP_GNU_parameter_ref takes as operand a 4 byte CU relative reference
				 * to the abstract optimized away DW_TAG_formal_parameter.
				 */
					x << "GNU-PARAMETER-REF-UNSUPPORTED!!! ";
					bytes_to_skip = sizeof(uint32_t);
					break;
				case DW_OP_consts:
					x << "CONSTS-UNSUPPORTED!!! ";
					DwarfUtil::sleb128(dwarf_expression, & bytes_to_skip);
					break;
				case DW_OP_constu:
					x << "CONSTU-UNSUPPORTED!!! ";
					DwarfUtil::uleb128(dwarf_expression, & bytes_to_skip);
					break;
				case DW_OP_GNU_convert:
					x << "GNU-CONVERT-UNSUPPORTED!!! ";
					DwarfUtil::uleb128(dwarf_expression, & bytes_to_skip);
					break;
				case DW_OP_GNU_implicit_pointer:
					x << "GNU-IMPLICIT-POINTER-UNSUPPORTED!!! ";
					DwarfUtil::sleb128(dwarf_expression + sizeof(uint32_t), & bytes_to_skip);
					bytes_to_skip += sizeof(uint32_t);
					break;
				case DW_OP_GNU_regval_type:
					x << "GNU-REGVAL-TYPE-UNSUPPORTED!!! ";
					DwarfUtil::uleb128(dwarf_expression, & i);
					DwarfUtil::uleb128(dwarf_expression + i, & bytes_to_skip);
					bytes_to_skip += i;
					break;
				case DW_OP_piece:
					x << "PIECE-UNSUPPORTED!!! ";
					DwarfUtil::sleb128(dwarf_expression, & bytes_to_skip);
					break;
				case DW_OP_implicit_value:
					x << "IMPLICIT-VALUE-UNSUPPORTED!!! ";
					i = DwarfUtil::uleb128(dwarf_expression, & bytes_to_skip);
					bytes_to_skip += i;
					break;
				case DW_OP_bra:
					x << "0= [if] ";
					bytes_to_skip = sizeof(int16_t);
					branch_counter = * (int16_t *) dwarf_expression;
					if (branch_counter < 0)
						/* requires special handling, not yet encountered for testing */
						DwarfUtil::panic();
					branch_counter +=
						/* compensate for the subtractions below */ bytes_to_skip
						+ /* compensate for the subtraction for the current dwarf opcode below */ 1;
					break;
				default:
					DwarfUtil::panic();
			}
			dwarf_expression += bytes_to_skip;
			expression_len -= bytes_to_skip;
			branch_counter -= bytes_to_skip + /* one more byte for the dwarf opcode just processed */ 1;
			if (branch_counter < 0)
				DwarfUtil::panic();
			else if (!branch_counter)
				x << "[then] ", branch_counter = 0x10000;
		}
		return x.str();
	}
	static int registerNumberOfRegXOpcode(unsigned opcode) { return (DW_OP_reg0 <= opcode && opcode <= DW_OP_reg31) ? opcode - DW_OP_reg0 : -1;
	}
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
	/* Introduced in DWARF 4 */
	uint8_t maximum_operations_per_instruction(void) { if (version() < 4) DwarfUtil::panic(); return * (uint8_t *) (header + 11); }
	uint8_t default_is_stmt(void)
	{
		if (version() == 2 || version() == 3) return * (uint8_t *) (header + 11);
		else if (version() == 4) return * (uint8_t *) (header + 12);
		else DwarfUtil::panic();
	}
	int8_t line_base(void)
	{
		if (version() == 2 || version() == 3) return * (uint8_t *) (header + 12);
		else if (version() == 4) return * (uint8_t *) (header + 13);
		else DwarfUtil::panic();
	}
	uint8_t line_range(void)
	{
		if (version() == 2 || version() == 3) return * (uint8_t *) (header + 13);
		else if (version() == 4) return * (uint8_t *) (header + 14);
		else DwarfUtil::panic();
	}
	uint8_t opcode_base(void)
	{
		if (version() == 2 || version() == 3) return * (uint8_t *) (header + 14);
		else if (version() == 4) return * (uint8_t *) (header + 15);
		else DwarfUtil::panic();
	}
	/*! \todo	Field 'standard_opcode_lengths' is not handled at all! Add support for this! */
	int standard_opcode_lengths_field_offset(void)
	{
		if (version() == 2 || version() == 3) return 15;
		else if (version() == 4) return 16;
		else DwarfUtil::panic();
	}
	const char * include_directories(void)
	{
		int i(opcode_base());
		const uint8_t * p(header + standard_opcode_lengths_field_offset());
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
	struct line_state { uint32_t file, address, is_stmt; int line, column; } registers[2], * current, * prev;
	void init(void) { registers[0] = (struct line_state) { .file = 1, .address = 0, .is_stmt = default_is_stmt(), .line = 1, .column = 0, };
				registers[1] = (struct line_state) { .file = 1, .address = 0xffffffff, .is_stmt = 0, .line = -1, .column = -1, };
				current = registers, prev = registers + 1; }
	void swap(void) { struct line_state * x(current); current = prev; prev = x; }
	void validateHeader(void)
	{
		if (version() != 2 && version() != 3 && version() != 4) DwarfUtil::panic();
		if (version() == 4 && maximum_operations_per_instruction() != 1) DwarfUtil::panic();
	}
public:
	struct lineAddress { uint32_t line, address, address_span; struct lineAddress * next; lineAddress(void) { line = address = address_span = -1; next = 0; } 
	                   bool operator < (const struct lineAddress & rhs) const { return address < rhs.address; } };
	struct sourceFileNames { const char * file, * directory, * compilation_directory; };
	DebugLine(const uint8_t * debug_line, uint32_t debug_line_len)
	{ header = this->debug_line = debug_line, this->debug_line_len = debug_line_len; validateHeader(); }
	/*! \todo	refactor here, the same code is duplicated several times with minor differences */
	void dump(void)
	{
		validateHeader();
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
				{
						auto d = DwarfUtil::uleb128(p, & x);
						if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set discriminator to" << d << "!!! IGNORED !!!";
						if (len != x + 1) DwarfUtil::panic();
						p += x;
				}
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
				case DW_LNS_set_prologue_end:
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set prologue end to true";
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
					if (!current->file)
						DwarfUtil::panic();
					p += len;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set file to" << current->file;
					break;
				case DW_LNS_set_column:
					current->column = DwarfUtil::uleb128(p, & len);
					p += len;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set column to" << current->column;
					break;
				case DW_LNS_negate_stmt:
					current->is_stmt = ! current->is_stmt;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set is_stmt to " << current->is_stmt;
					break;
			}
		}
	}
	/* returns -1 if no line number was found */
	uint32_t lineNumberForAddress(uint32_t target_address, uint32_t statement_list_offset, uint32_t & file_number, bool & is_address_on_exact_line_number_boundary)
	{
		header = debug_line + statement_list_offset;
		validateHeader();
		const uint8_t * p(line_number_program()), op_base(opcode_base()), lrange(line_range());
		int lbase(line_base());
		uint32_t min_insn_length(minimum_instruction_length());
		int len, x;
		init();
		is_address_on_exact_line_number_boundary = false;
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
				{
						auto d = DwarfUtil::uleb128(p, & x);
						if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set discriminator to" << d << "!!! IGNORED !!!";
						if (len != x + 1) DwarfUtil::panic();
						p += x;
						break;
				}
					case DW_LNE_end_sequence:
						if (len != 1) DwarfUtil::panic();
						if (prev->address <= target_address && target_address < current->address)
						{
							if (prev->address == target_address)
								is_address_on_exact_line_number_boundary = true;
							return file_number = prev->file, prev->line;
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
				if (prev->address <= target_address && target_address < current->address)
				{
					if (prev->address == target_address)
						is_address_on_exact_line_number_boundary = true;
					return file_number = prev->file, prev->line;
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
				/*
					if (xaddr <= target_address && target_address < address)
						DwarfUtil::panic();*/
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "copy";
					if (prev->address <= target_address && target_address < current->address)
					{
						if (prev->address == target_address)
							is_address_on_exact_line_number_boundary = true;
						return file_number = prev->file, prev->line;
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
					current->column = DwarfUtil::uleb128(p, & len);
					p += len;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set column to" << current->column;
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
		validateHeader();
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
				{
						auto d = DwarfUtil::uleb128(p, & x);
						if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set discriminator to" << d << "!!! IGNORED !!!";
						if (len != x + 1) DwarfUtil::panic();
						p += x;
						break;
				}
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
					break;
				case DW_LNS_set_prologue_end:
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set prologue end to true";
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
					current->column = DwarfUtil::uleb128(p, & len);
					p += len;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set column to" << current->column;
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
	void stringsForFileNumber(uint32_t file_number, const char * & file_name, const char * & directory_name, const char * compilation_directory)
	{
		file_name = "<<< unknown file >>>";
		directory_name = "<<< unknown directory >>>";
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
		file_name = f.s, f.s += strlen(f.s) + 1, i = DwarfUtil::uleb128x(f.p);
		if (!i)
			/* use the default compilation directory */
			directory_name = compilation_directory;
		else
		{
			i --;
			f.s = include_directories();
			while (j != i && (l = strlen(f.s)))
				f.s += l + 1, j ++;
			if (i == j)
				directory_name = f.s;
		}
	}
	void getFileAndDirectoryNamesPointers(std::vector<struct sourceFileNames> & sources, const char * compilation_directory)
	{
		qDebug() << (uint32_t) (header - debug_line);
		std::vector<const char *> directories;
		union { const char * s; const uint8_t * p; } x;
		x.s = include_directories();
		size_t l;
		directories.push_back(compilation_directory);
		while ((l = strlen(x.s)))
			directories.push_back(x.s), x.s += l + 1;
		x.s = file_names();
		while ((l = strlen(x.s)))
		{
			const char * s = x.s;
			uint32_t index;
			x.s += l + 1;
			/* skip directory index, file time and file size */
			index = DwarfUtil::uleb128x(x.p), DwarfUtil::uleb128x(x.p), DwarfUtil::uleb128x(x.p);
			sources.push_back((struct sourceFileNames) { .file = s, .directory = directories.at(index), .compilation_directory = compilation_directory, });
		}
	}

	void rewind(void) { header = debug_line; }
	void skipToOffset(uint32_t offset) { header = debug_line + offset; validateHeader(); }
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

	struct debug_arange arange;
	/* cached for performance reasons */
	const uint8_t	* last_searched_arange;
	struct compilation_unit_header last_searched_compilation_unit;
	
	struct
	{
		unsigned total_dies;
		unsigned total_compilation_units;
		unsigned dies_read;
		unsigned compilation_unit_arange_hits;
		unsigned compilation_unit_arange_misses;
		unsigned compilation_unit_header_hits;
		unsigned compilation_unit_header_misses;
		unsigned abbreviation_hits;
		unsigned abbreviation_misses;
	}
	stats;
	struct DieFingerprint
	{
		uint32_t	offset;
		uint32_t	abbrev_offset;
	};
	std::vector<struct DieFingerprint> die_fingerprints;
	uint32_t abbreviationOffsetForDieOffset(uint32_t die_offset)
	{
		int l = 0, h = die_fingerprints.size() - 1, m;
		while (l <= h)
		{
			m = (l + h) >> 1;
			if (die_fingerprints.at(m).offset == die_offset)
				return die_fingerprints.at(m).abbrev_offset;
			if (die_fingerprints.at(m).offset < die_offset)
				l = m + 1;
			else
				h = m - 1;
		}
		DwarfUtil::panic();
	}
	
	/* first number is the abbreviation code, the second is the offset in .debug_abbrev */
	void getAbbreviationsOfCompilationUnit(uint32_t compilation_unit_offset, std::map<uint32_t, uint32_t> & abbreviations)
	{
		if (STATS_ENABLED) stats.abbreviation_misses ++;
		const uint8_t * debug_abbrev = this->debug_abbrev + compilation_unit_header((uint8_t *) debug_info + compilation_unit_offset) . debug_abbrev_offset(); 
		abbreviations.clear();
		Abbreviation a(debug_abbrev);

		while (a.code())
		{
			auto x = abbreviations.find(a.code());
			if (x != abbreviations.end())
				DwarfUtil::panic("duplicate abbreviation code");
			abbreviations.operator [](a.code()) = debug_abbrev - this->debug_abbrev;
			/* Skip to next abbreviation. */
			while (a.next_attribute().name)
				;
			a = Abbreviation(debug_abbrev += a.byteSize());
		}
	}

	void reapDieFingerprints(uint32_t & die_offset, std::map<uint32_t, uint32_t> & abbreviations, int depth = 0)
	{
		stats.total_dies ++;
		const uint8_t * p = debug_info + die_offset;
		int len;
		uint32_t code = DwarfUtil::uleb128(p, & len);
		p += len;

		/*! \note	some compilers (e.g. IAR) generate abbreviations in .debug_abbrev which specify that a die has
		 * children, while it actually does not - such a die actually considers a single null die child,
		 * which is explicitly permitted by the dwarf standard. Handle this at the condition check at the start of the loop */
		while (code)
		{
			auto x = abbreviations.find(code);
			if (x == abbreviations.end())
				DwarfUtil::panic("abbreviation code not found");
			struct Abbreviation a(debug_abbrev + x->second);
			
			die_fingerprints.push_back((struct DieFingerprint) { .offset = die_offset, .abbrev_offset = x->second});
			
			auto attr = a.next_attribute();
			while (attr.name)
			{
				p += DwarfUtil::skip_form_bytes(attr.form, p);
				attr = a.next_attribute();
			}
			die_offset = p - debug_info;
			if (a.has_children())
			{
				reapDieFingerprints(die_offset, abbreviations, depth + 1);
				p = debug_info + die_offset;
			}
			
			if (depth == 0)
				return;
				
			code = DwarfUtil::uleb128(p, & len);
			p += len;
		}
		die_offset = p - debug_info;
	}
public:
	DwarfData(const void * debug_aranges, uint32_t debug_aranges_len, const void * debug_info, uint32_t debug_info_len,
		  const void * debug_abbrev, uint32_t debug_abbrev_len, const void * debug_ranges, uint32_t debug_ranges_len,
		  const void * debug_str, uint32_t debug_str_len,
		  const void * debug_line, uint32_t debug_line_len,
		  const void * debug_loc, uint32_t debug_loc_len) : arange((const uint8_t *) debug_aranges), last_searched_compilation_unit((const uint8_t *) 0)
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
		
		verbose_type_printing_indentation_level = 0;

		last_searched_compilation_unit.data = this->debug_info;
		last_searched_arange = arange.data;
		memset(& stats, 0, sizeof stats);
		
		std::map<uint32_t, uint32_t> abbreviations;
		uint32_t cu;
		for (cu = 0; cu != -1; cu = next_compilation_unit(cu))
		{
			auto die_offset = cu + /* skip compilation unit header */ compilation_unit_header(this->debug_info + cu).header_length();
			stats.total_compilation_units ++;
			getAbbreviationsOfCompilationUnit(cu, abbreviations);
			reapDieFingerprints(die_offset, abbreviations);
		}
	}
	void dumpStats(void)
	{
		qDebug() << "total dies in .debug_info:" << stats.total_dies;
		qDebug() << "total compilation units in .debug_info:" << stats.total_compilation_units;
		qDebug() << "total dies read:" << stats.dies_read;
		qDebug() << "compilation unit address range search hits:" << stats.compilation_unit_arange_hits;
		qDebug() << "compilation unit address range search misses:" << stats.compilation_unit_arange_misses;
		qDebug() << "compilation unit die search hits:" << stats.compilation_unit_header_hits;
		qDebug() << "compilation unit die search misses:" << stats.compilation_unit_header_misses;
		qDebug() << "abbreviation fetch hits:" << stats.abbreviation_hits;
		qDebug() << "abbreviation fetch misses:" << stats.abbreviation_misses;
	}
private:
	/* returns -1 if the compilation unit is not found */
	uint32_t compilationUnitOffsetForOffsetInDebugInfo(uint32_t debug_info_offset)
	{
		if (last_searched_compilation_unit.data - debug_info <= debug_info_offset && debug_info_offset < last_searched_compilation_unit.data - debug_info + last_searched_compilation_unit.unit_length())
		{
			if (STATS_ENABLED) stats.compilation_unit_header_hits ++;
			return last_searched_compilation_unit.data - debug_info;
		}
		if (STATS_ENABLED) stats.compilation_unit_header_misses ++;
		compilation_unit_header h(debug_info);
		while (h.data - debug_info < debug_info_len)
			if (h.data - debug_info <= debug_info_offset && debug_info_offset < h.data - debug_info + h.unit_length())
			{
				last_searched_compilation_unit.data = h.data;
				return h.data - debug_info;
			}
			else h.next();
		return -1;
	}

	bool isAddressInCompilationUnitRange(uint32_t address, const struct debug_arange & arange)
	{
		int i = 0;
		const struct debug_arange::compilation_unit_range * r;
		do
		{
			r = arange.ranges() + i;
			if (r->start_address <= address && address < r->start_address + r->length)
				return true;
			i ++;
		}
		while (r->length || r->start_address);
		return false;
	}
	/* returns -1 if the compilation unit is not found */
	uint32_t	get_compilation_unit_debug_info_offset_for_address(uint32_t address)
	{
		struct debug_arange last(last_searched_arange);
		if (isAddressInCompilationUnitRange(address, last))
		{
			if (STATS_ENABLED) stats.compilation_unit_arange_hits ++;
			return last.compilation_unit_debug_info_offset();
		}
		if (STATS_ENABLED) stats.compilation_unit_arange_misses ++;
		arange.rewind();
		while (arange.data < debug_aranges + debug_aranges_len)
		{
			if (arange.address_size() != 4)
				DwarfUtil::panic();
			if (isAddressInCompilationUnitRange(address, arange))
			{
				last_searched_arange = arange.data;
				return arange.compilation_unit_debug_info_offset();
			}
			arange.next();
		}
		return -1;
	}
	uint32_t compilation_unit_base_address(const struct Die & compilation_unit_die)
	{
		struct Abbreviation a(debug_abbrev + compilation_unit_die.abbrev_offset);
		auto low_pc = a.dataForAttribute(DW_AT_low_pc, debug_info + compilation_unit_die.offset);
		if (!low_pc.form)
			DwarfUtil::panic();
		return DwarfUtil::fetchHighLowPC(low_pc.form, low_pc.debug_info_bytes);
	}
	bool isAddressInRange(const struct Die & die, uint32_t address, const struct Die & compilation_unit_die)
	{
		struct Abbreviation a(debug_abbrev + die.abbrev_offset);
		auto range = a.dataForAttribute(DW_AT_ranges, debug_info + die.offset);
		if (range.form)
		{
			auto base_address = compilation_unit_base_address(compilation_unit_die);
			const uint32_t * range_list = (const uint32_t *) (debug_ranges + DwarfUtil::formConstant(range));
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
		if (low_pc.form)
		{
			uint32_t x;
			auto hi_pc = a.dataForAttribute(DW_AT_high_pc, debug_info + die.offset);
			if (!hi_pc.form)
				return false;
			if (DEBUG_ADDRESS_RANGE_ENABLED) qDebug() << (x = DwarfUtil::fetchHighLowPC(low_pc.form, low_pc.debug_info_bytes));
			if (DEBUG_ADDRESS_RANGE_ENABLED) qDebug() << DwarfUtil::fetchHighLowPC(hi_pc.form, hi_pc.debug_info_bytes, x);
			x = DwarfUtil::fetchHighLowPC(low_pc.form, low_pc.debug_info_bytes);
			return x <= address && address < DwarfUtil::fetchHighLowPC(hi_pc.form, hi_pc.debug_info_bytes, x);
		}
		return false;
	}
	/*! \todo	the name of this function is misleading, it really reads a sequence of dies on a same die tree level; that
	 * 		is because of the dwarf die flattenned tree representation */
	std::vector<struct Die> debug_tree_of_die(uint32_t & die_offset, int depth = 0, int max_depth = -1)
	{
		if (STATS_ENABLED) stats.dies_read ++;
		std::vector<struct Die> dies;
		const uint8_t * p = debug_info + die_offset;
		int len;
		if (DEBUG_DIE_READ_ENABLED) qDebug() << "at offset " << QString("$%1").arg(die_offset, 0, 16);
		uint32_t code = DwarfUtil::uleb128(p, & len);
		p += len;
		/*! \note	some compilers (e.g. IAR) generate abbreviations in .debug_abbrev which specify that a die has
		 * children, while it actually does not - such a die actually considers a single null die child,
		 * which is explicitly permitted by the dwarf standard. Handle this at the condition check at the start of the loop */
		while (code)
		{
			auto x = abbreviationOffsetForDieOffset(die_offset);
			struct Abbreviation a(debug_abbrev + x);
			struct Die die(a.tag(), die_offset, x);
			
			auto attr = a.next_attribute();
			while (attr.name)
			{
				p += DwarfUtil::skip_form_bytes(attr.form, p);
				attr = a.next_attribute();
			}
			die_offset = p - debug_info;
			if (a.has_children())
			{
				if (depth + 1 == max_depth)
				{
					if (/* special case for reading a single die */ depth == 0)
						goto there;
					auto x = a.dataForAttribute(DW_AT_sibling, debug_info + die.offset);
					if (x.form)
					{
						die_offset = DwarfUtil::formReference(x.form, x.debug_info_bytes, compilationUnitOffsetForOffsetInDebugInfo(die.offset));
						goto there;
					}
				}
				die.children = debug_tree_of_die(die_offset, depth + 1, max_depth);
there:
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
public:
	struct Die read_die(uint32_t die_offset) { return debug_tree_of_die(die_offset, 0, 1).at(0); }
	uint32_t next_compilation_unit(uint32_t compilation_unit_offset)
	{
		uint32_t x = compilation_unit_header(debug_info + compilation_unit_offset).next().data - debug_info;
		if (x >= debug_info_len)
			x = -1;
		return x;
	}
	int compilation_unit_count(void)
	{
		int i;
		arange.rewind();
		for (i = 0; arange.data < debug_aranges + debug_aranges_len; arange.next(), i++)
			if (arange.address_size() != 4)
				DwarfUtil::panic();
		return i;
	}
	std::vector<struct Die> executionContextForAddress(uint32_t address)
	{
		std::vector<struct Die> context;
		auto cu_die_offset = get_compilation_unit_debug_info_offset_for_address(address);
		if (cu_die_offset == -1)
			return context;
		cu_die_offset += /* discard the compilation unit header */ compilation_unit_header(debug_info + cu_die_offset).header_length();
		auto compilation_unit_die = debug_tree_of_die(cu_die_offset, 0, 1);
		int i(0);
		std::vector<struct Die> * die_list(& compilation_unit_die);
		while (i < die_list->size())
			if (isAddressInRange(die_list->at(i), address, compilation_unit_die.at(0)))
			{
				uint32_t die_offset(die_list->at(i).offset);
				die_list->at(i).children = debug_tree_of_die(die_offset, /* read only immediate die children */ 0, 2).at(0).children;
				context.push_back(die_list->at(i));
				die_list = & die_list->at(i).children;
				i = 0;
			}
			else
				i ++;
		return context;
	}
	bool callSiteAtAddress(uint32_t address, struct Die & call_site, std::vector<struct Die> * execution_context = 0)
	{
		auto x = executionContextForAddress(address);
		if (!x.size())
			return false;
		auto d = x.back().children.begin();
		while (d != x.back().children.end())
		{
			if ((* d).tag == DW_TAG_GNU_call_site)
			{
				Abbreviation a(debug_abbrev + d->abbrev_offset);
				auto l = a.dataForAttribute(DW_AT_low_pc, debug_info + d->offset);
				if (l.form && DwarfUtil::fetchHighLowPC(l.form, l.debug_info_bytes) == address)
				{
					call_site = * d;
					/* make sure that the call site's children have been read */
					if (call_site.children.empty())
						call_site.children = debug_tree_of_die(call_site.offset, /* read only immediate die children */ 0, 2)[0].children;
					if (execution_context)
						* execution_context = x;
					return true;
				}
			}
			d ++;
		}
		return false;
	}
	std::vector<struct Die> inliningChainOfContext(const std::vector<struct Die> & context)
	{
		std::vector<struct Die> inlining_chain;
		int i;
		for (i = context.size() - 1; i >= 0 && !context.at(i).isNonInlinedSubprogram(); i --)
			if (context.at(i).isInlinedSubprogram())
				inlining_chain.push_back(context.at(i));
		return inlining_chain;
	}
	const struct Die & topLevelSubprogramOfContext(const std::vector<struct Die> & context)
	{
		int i;
		for (i = 0; i < context.size(); i++)
			if (context.at(i).isNonInlinedSubprogram())
				return context.at(i);
		DwarfUtil::panic();
	}
	std::vector<struct Die> localDataObjectsForContext(const std::vector<struct Die> & context)
	{
	std::vector<struct Die> locals;
	int i, j;
		for (i = context.size() - 1; i >= 0; i --)
			if (context.at(i).isSubprogram() || context.at(i).isLexicalBlock() )
				for (j = 0; j < context.at(i).children.size(); j ++)
					if (context.at(i).children.at(j).isDataObject())
						locals.push_back(context.at(i).children.at(j));
		return locals;
	}
	/* Note: for dwarf 3 and above, gcc usually sets the value of the 'DW_AT_frame_base' attribute
	 * for subprograms to 'DW_OP_call_frame_cfa'. In such a case, the value of the program counter
	 * is irrelevant.
	 *
	 * This is not the case, however, for dwarf 2. In the case of dwarf 2, gcc can emit location lists
	 * for the 'DW_AT_frame_base' attribute, and obviously, in such a case, the value of the program
	 * counter is absolutely necessary in order to locate the appropriate location list entry that
	 * contains the value for the 'DW_AT_frame_base' attribute. This is why an extra parameter for
	 * the program counter has been introduced for this function. */
	std::string sforthCodeFrameBaseForContext(const std::vector<struct Die> & context, uint32_t program_counter)
	{
		int i;
		std::string frame_base;
		for (i = context.size() - 1; i >= 0 && frame_base.empty(); frame_base = locationSforthCode(context.at(i --), context.at(0), program_counter, DW_AT_frame_base));
		qDebug() << QString::fromStdString(frame_base);
		return frame_base;
	}
	struct SourceCodeCoordinates sourceCodeCoordinatesForDieOffset(uint32_t die_offset)
	{
		SourceCodeCoordinates s;
		auto die = read_die(die_offset);
		Abbreviation a(debug_abbrev + die.abbrev_offset);
		auto file = a.dataForAttribute(DW_AT_decl_file, debug_info + die.offset);
		auto line = a.dataForAttribute(DW_AT_decl_line, debug_info + die.offset);
		auto call_file = a.dataForAttribute(DW_AT_call_file, debug_info + die.offset);
		auto call_line = a.dataForAttribute(DW_AT_call_line, debug_info + die.offset);
		if (!file.form || !line.form)
		{
			struct Die referred_die(die);
			if (hasAbstractOrigin(die, referred_die))
				s = sourceCodeCoordinatesForDieOffset(referred_die.offset);
		}
		auto cu_offset = compilationUnitOffsetForOffsetInDebugInfo(die.offset);
		cu_offset += /* skip compilation unit header */ compilation_unit_header(debug_info + cu_offset).header_length();

		auto compilation_unit_die = read_die(cu_offset);
		Abbreviation b(debug_abbrev + compilation_unit_die.abbrev_offset);
		auto statement_list = b.dataForAttribute(DW_AT_stmt_list, debug_info + compilation_unit_die.offset);
		auto compilation_directory = b.dataForAttribute(DW_AT_comp_dir, debug_info + compilation_unit_die.offset);
		if (!statement_list.form)
			return s;
		DebugLine l(debug_line, debug_line_len);
		const char * compilation_directory_string = 0;
		if (compilation_directory.form)
			compilation_directory_string = DwarfUtil::formString(compilation_directory.form, compilation_directory.debug_info_bytes, debug_str);
		l.skipToOffset(DwarfUtil::formConstant(statement_list));
		if (file.form)
			l.stringsForFileNumber(DwarfUtil::formConstant(file), s.file_name, s.directory_name, compilation_directory_string);
		if (call_file.form)
			l.stringsForFileNumber(DwarfUtil::formConstant(call_file), s.call_file_name, s.call_directory_name, compilation_directory_string);
		if (line.form)
			s.line = DwarfUtil::formConstant(line);
		if (call_line.form)
			s.call_line = DwarfUtil::formConstant(call_line);
		s.compilation_directory_name = compilation_directory_string;
		return s;
	}

	struct SourceCodeCoordinates sourceCodeCoordinatesForAddress(uint32_t address, bool * is_address_on_exact_line_number_boundary = 0)
	{
		SourceCodeCoordinates s;
		s.address = address;
		auto cu_die_offset = get_compilation_unit_debug_info_offset_for_address(address);
		if (cu_die_offset == -1)
			return s;
		cu_die_offset += /* skip compilation unit header */ compilation_unit_header(debug_info + cu_die_offset).header_length();

		auto compilation_unit_die = read_die(cu_die_offset);
		uint32_t file_number;
		if (compilation_unit_die.tag != DW_TAG_compile_unit)
			DwarfUtil::panic();
		Abbreviation a(debug_abbrev + compilation_unit_die.abbrev_offset);
		auto x = a.dataForAttribute(DW_AT_stmt_list, debug_info + compilation_unit_die.offset);
		if (!x.form)
			return s;
		class DebugLine l(debug_line, debug_line_len);
		bool dummy;
		s.line = l.lineNumberForAddress(address, DwarfUtil::formConstant(x), file_number, is_address_on_exact_line_number_boundary ? * is_address_on_exact_line_number_boundary : dummy);
		x = a.dataForAttribute(DW_AT_comp_dir, debug_info + compilation_unit_die.offset);
		if (x.form)
			s.compilation_directory_name = DwarfUtil::formString(x.form, x.debug_info_bytes, debug_str);
		l.stringsForFileNumber(file_number, s.file_name, s.directory_name, s.compilation_directory_name);
		return s;
	}

	struct TypePrintFlags
	{
		bool	verbose_printing		: 1;
		bool	print_dereferenced_types	: 1;
		bool	discard_typedefs		: 1;
		TypePrintFlags(void)	{ verbose_printing = print_dereferenced_types = discard_typedefs = false; }
	};

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
		case DW_FORM_ref_udata:
			return DwarfUtil::uleb128(debug_info_bytes) + compilation_unit_header_offset;
		case DW_FORM_ref_addr:
			return * (uint32_t *) debug_info_bytes;
		default:
			DwarfUtil::panic();
		}
	}
	bool hasAbstractOrigin(const struct Die & die, struct Die & referred_die)
	{
		struct Abbreviation a(debug_abbrev + die.abbrev_offset);
		auto x = a.dataForAttribute(DW_AT_abstract_origin, debug_info + die.offset);
		if (!x.form)
		{
			x = a.dataForAttribute(DW_AT_specification, debug_info + die.offset);
			if (!x.form)
				return false;
		}
		auto i = compilationUnitOffsetForOffsetInDebugInfo(die.offset);
		auto referred_die_offset = DwarfUtil::formReference(x.form, x.debug_info_bytes, i);
		{
			compilationUnitOffsetForOffsetInDebugInfo(referred_die_offset);
			referred_die = read_die(referred_die_offset);
		}
		return true;
	}
std::map<uint32_t, uint32_t> recursion_detector;

	int verbose_type_printing_indentation_level;
	std::string typeChainString(std::vector<struct DwarfTypeNode> & type, bool is_prefix_printed, int node_number, struct TypePrintFlags flags)
	{
		std::string type_string;
		if (node_number == -1)
		{
			if (is_prefix_printed)
				type_string += "void ";
			return type_string;
		}
		if (type.at(node_number).type_print_recursion_flag)
		{
			/* recursion detected - avoid infinite recursion by forcing non-verbose type printing */
			flags.verbose_printing = false;
		}
		const struct Die & die(type.at(node_number).die);

		switch (die.tag)
		{
			case DW_TAG_enumeration_type:
				if (is_prefix_printed) type_string += "enum"; if (0)
			case DW_TAG_union_type:
				if (is_prefix_printed) type_string += "union"; if (0)
			case DW_TAG_structure_type:
				if (is_prefix_printed) type_string += "struct"; if (0)
			case DW_TAG_class_type:
				if (is_prefix_printed) type_string += "class";
			if (is_prefix_printed)
			{
				std::string name = nameOfDie(die, true);
				if (!name.empty())
					/* prevent type printing recursion only for non-anonymous constructs */
					type.at(node_number).type_print_recursion_flag = true;
				type_string += " " + name + " ";
				if (flags.verbose_printing)
				{
					auto i = type.at(node_number).children.begin();
					type_string += "{\n";
					verbose_type_printing_indentation_level ++;
					while (i != type.at(node_number).children.end())
						type_string += std::string(verbose_type_printing_indentation_level, '\t') + typeString(type, * i, flags, false) + ((die.tag == DW_TAG_enumeration_type) ? "," : ";") + "\n", i ++;
					verbose_type_printing_indentation_level --;
					type_string += std::string(verbose_type_printing_indentation_level, '\t') + "}";
				}
			}
			break;
			case DW_TAG_formal_parameter:
				if (0)
			case DW_TAG_template_type_parameter:
				if (is_prefix_printed)
					type_string += "@template value parameter@ ";
			case DW_TAG_template_value_parameter:
				if (0)
			case DW_TAG_inheritance:
				if (is_prefix_printed)
					type_string += "@inherits@ ";
			case DW_TAG_member:
			if (is_prefix_printed)
			{
				type_string += typeChainString(type, true, type.at(node_number).next, flags);
				type_string += nameOfDie(die, true);
				type_string += typeChainString(type, false, type.at(node_number).next, flags);
			}
			else
			{
				Abbreviation a(debug_abbrev + die.abbrev_offset);
				auto x = a.dataForAttribute(DW_AT_bit_size, debug_info + die.offset);
				if (x.form)
					type_string += QString(" : %1").arg(DwarfUtil::formConstant(x)).toStdString();
			}
				break;
			case DW_TAG_volatile_type:
				if (is_prefix_printed)
					type_string += "volatile ";
				type_string += typeChainString(type, is_prefix_printed, type.at(node_number).next, flags);
				break;
			case DW_TAG_const_type:
				if (is_prefix_printed)
					type_string += "const ";
				type_string += typeChainString(type, is_prefix_printed, type.at(node_number).next, flags);
				break;
			case DW_TAG_typedef:
				if (is_prefix_printed && (!flags.discard_typedefs || !flags.verbose_printing))
				{
					if (flags.verbose_printing) type_string += "typedef ";
					type_string += std::string(nameOfDie(die)) + " ";
				}
				if (flags.verbose_printing)
					type_string += typeChainString(type, is_prefix_printed, type.at(node_number).next, flags);
				break;
			case DW_TAG_reference_type:
			case DW_TAG_rvalue_reference_type:
			case DW_TAG_ptr_to_member_type:
			case DW_TAG_pointer_type:
				if (is_prefix_printed)
				{
					type_string = typeChainString(type, is_prefix_printed, type.at(node_number).next, flags);
					if (isArrayType(type, type.at(node_number).next) || isSubroutineType(type, type.at(node_number).next))
						type_string += "(";
					type_string += ((die.tag == DW_TAG_pointer_type || die.tag == DW_TAG_ptr_to_member_type) ? "*" : "&");
				}
				else
				{
					if (isArrayType(type, type.at(node_number).next) || isSubroutineType(type, type.at(node_number).next))
						type_string += ")";
					type_string += typeChainString(type, is_prefix_printed, type.at(node_number).next, flags);
				}
				break;
			case DW_TAG_base_type:
			if (is_prefix_printed)
			{
				Abbreviation a(debug_abbrev + die.abbrev_offset);
				auto x = a.dataForAttribute(DW_AT_encoding, debug_info + die.offset);
				auto size = a.dataForAttribute(DW_AT_byte_size, debug_info + die.offset);
				if (x.form == 0 || size.form == 0)
					DwarfUtil::panic();
				switch (DwarfUtil::formConstant(x))
				{
					default:
						qDebug() << "unhandled encoding" << DwarfUtil::formConstant(x);
						DwarfUtil::panic();
					case DW_ATE_UTF:
						type_string += "utf ";
						break;
					case DW_ATE_float:
						type_string += "float ";
						break;
					case DW_ATE_boolean:
						type_string += "bool ";
						break;
					case DW_ATE_unsigned_char:
						type_string += "unsigned char ";
						break;
					case DW_ATE_signed_char:
						type_string += "signed char ";
						break;
					case DW_ATE_unsigned:
						switch (DwarfUtil::formConstant(size))
						{
							default:
								DwarfUtil::panic();
							case 1:
								type_string += "unsigned char ";
								break;
							case 2:
								type_string += "unsigned short ";
								break;
							case 4:
								type_string += "unsigned int ";
								break;
							case 8:
								type_string += "long long unsigned int ";
								break;
						}
						break;
					case DW_ATE_signed:
						switch (DwarfUtil::formConstant(size))
						{
							default:
								DwarfUtil::panic();
							case 1:
								type_string += "signed char ";
								break;
							case 2:
								type_string += "signed short ";
								break;
							case 4:
								type_string += "signed int ";
								break;
							case 8:
								type_string += "long long signed int ";
								break;
						}
				}
			}
				break;
			case DW_TAG_array_type:
			{
				int i;
				if (is_prefix_printed)
					type_string += typeChainString(type, is_prefix_printed, type.at(node_number).next, flags);
				else
				{
					if (die.children.size())
						for (i = 0; i < die.children.size(); i ++)
						{
							Abbreviation a(debug_abbrev + die.children.at(i).abbrev_offset);
							auto subrange = a.dataForAttribute(DW_AT_upper_bound, debug_info + die.children.at(i).offset);
							if (subrange.form == 0)
								type_string += "[]";
							else
								type_string += "[!" + std::to_string(DwarfUtil::formConstant(subrange) + 1) + "]";
						}
					else
						type_string += "[]";
					type_string += typeChainString(type, is_prefix_printed, type.at(node_number).next, flags);
				}
			}
				break;
			case DW_TAG_subprogram:
			case DW_TAG_subroutine_type:
				/*! \todo	it is sometimes better to force non-verbose printing of subroutine types - maybe provide this as an option as well */
				//flags.verbose_printing = false;
				if (!is_prefix_printed)
				{
				int i;
					type_string += nameOfDie(die, true);
					type_string += "(";
					if (!type.at(node_number).children.size())
						type_string += "void";
					else
					{
						for (i = 0; i < type.at(node_number).children.size(); i ++)
						{
							int j = type.at(node_number).children.at(i);
							if (type.at(j).die.tag == DW_TAG_formal_parameter)
								type_string += typeString(type, j, flags, false) + ", ";
							else if (type.at(j).die.tag == DW_TAG_unspecified_parameters)
								type_string += "...xx";
							else
								type_string += "<<< unknown subprogram type die child >>>";
						}
						type_string.pop_back(), type_string.pop_back();
					}
					type_string += ") ";
				}
				type_string += typeChainString(type, is_prefix_printed, type.at(node_number).next, flags);
				break;
			case DW_TAG_enumerator:
				if (is_prefix_printed)
				{
					type_string += nameOfDie(die, true);
					type_string += " = ";
					Abbreviation a(debug_abbrev + die.abbrev_offset);
					auto x = a.dataForAttribute(DW_AT_const_value, debug_info + die.offset);
					if (x.form)
						type_string += QString(" %1").arg(DwarfUtil::formConstant(x)).toStdString();
				}
				
				break;
			default:
				qDebug() << "unhandled tag" <<  type.at(node_number).die.tag << "in" << __func__;
				qDebug() << "die offset" << type.at(node_number).die.offset;
				DwarfUtil::panic();
		}
		if (die.tag != DW_TAG_structure_type && die.tag != DW_TAG_class_type)
		return type_string;
	}
public:
int readType(uint32_t die_offset, std::vector<struct DwarfTypeNode> & type_cache, bool reset_recursion_detector = true);
bool isPointerType(const std::vector<struct DwarfTypeNode> & type, int node_number = 0);
bool isArrayType(const std::vector<struct DwarfTypeNode> & type, int node_number = 0);
bool isSubroutineType(const std::vector<struct DwarfTypeNode> & type, int node_number = 0);

	std::string typeString(std::vector<struct DwarfTypeNode> & type, int node_number = 0, struct TypePrintFlags flags = TypePrintFlags(), bool reset_type_recursion_detection_flags = true);

	int sizeOf(const std::vector<struct DwarfTypeNode> & type, int node_number = 0)
	{
		if (node_number == -1)
		{
		/*! \todo	handle declarations here - for some reason they appear here */
			qDebug() << __func__ << "bad node index";
			return -1;
			DwarfUtil::panic();
		}
		Abbreviation a(debug_abbrev + type.at(node_number).die.abbrev_offset);
		auto x = a.dataForAttribute(DW_AT_byte_size, debug_info + type.at(node_number).die.offset);
		if (x.form)
			return DwarfUtil::formConstant(x);
		if (type.at(node_number).array_dimensions.size())
		{
			int i;
			uint32_t n = 1;
			for (i = 0; i < type.at(node_number).array_dimensions.size(); n *= type.at(node_number).array_dimensions.at(i ++));
			return n * sizeOf(type, type.at(node_number).next);
		}
		/* special case for compilers (e.g. the IAR compiler), which do not record an explicit
		 * DW_AT_byte_size value for pointers */
		if (type.at(node_number).die.tag == DW_TAG_pointer_type)
			return sizeof(uint32_t);
		/*! \todo	why is this here??? remove this!!!
		if (type.at(node_number).die.tag == DW_TAG_subprogram)
			return sizeof(uint32_t);
			*/
		
		return sizeOf(type, type.at(node_number).next);
	}
	
	struct DataNode
	{
		std::vector<std::string> data;
		uint32_t bytesize;
		uint32_t data_member_location;
		struct Die	enumeration_die;
		uint32_t	die_offset;
		struct
		{
			uint32_t is_pointer	: 1;
			uint32_t is_enumeration	: 1;
			uint32_t bitsize	: 6;
			uint32_t bitposition	: 6;
		};
		/* This is the base type encoding value from the dwarf standard. Set to -1 if this node is not a base type node. */
		uint32_t base_type_encoding;
		std::vector<uint32_t> array_dimensions;
		std::vector<struct DataNode> children;
	};
	void dataForType(std::vector<struct DwarfTypeNode> & type, struct DataNode & node, int type_node_number = 0, struct TypePrintFlags flags = TypePrintFlags())
	{
		struct Die die(type.at(type_node_number).die);
		Abbreviation a(debug_abbrev + die.abbrev_offset);
		node.bytesize = sizeOf(type, type_node_number);
		node.data_member_location = 0;
		node.is_pointer = node.is_enumeration = node.bitsize = node.bitposition = 0;
		node.die_offset = die.offset;
		node.base_type_encoding = -1;
		
		if (1 && type.at(type_node_number).processed)
		{
			qDebug() << "recursion detected in" << __func__;
			qDebug() << "node index" << type_node_number;
node.data.push_back("!!! recursion detected !!!");
			return;
		}
		type.at(type_node_number).processed = true;

		node.data.push_back(typeString(type, type_node_number, flags, false));

		switch (die.tag)
		{
			case DW_TAG_structure_type:
			case DW_TAG_union_type:
			case DW_TAG_class_type:
				int j;
				for (j = 0; j < type.at(type_node_number).children.size(); j ++)
				{
					DataNode n;
					dataForType(type, n, type.at(type_node_number).children.at(j), flags);
					node.children.push_back(n);
				}
				break;
			case DW_TAG_ptr_to_member_type:
			case DW_TAG_pointer_type:
			case DW_TAG_reference_type:
				node.is_pointer = 1;
				break;
			case DW_TAG_member:
			case DW_TAG_inheritance:
				dataForType(type, node, type.at(type_node_number).next, flags);
				{
					auto x = a.dataForAttribute(DW_AT_data_member_location, debug_info + die.offset);
					if (x.form) switch (x.form)
					{
					/* special cases for members */
					case DW_FORM_block:
						DwarfUtil::uleb128x(x.debug_info_bytes);
						if (0)
					case DW_FORM_block1:
						x.debug_info_bytes ++;
						if (* x.debug_info_bytes ++ == DW_OP_plus_uconst)
							node.data_member_location = DwarfUtil::uleb128x(x.debug_info_bytes);
						else
							DwarfUtil::panic();
						break;
					default:
						node.data_member_location = DwarfUtil::formConstant(x);
					}
					x = a.dataForAttribute(DW_AT_bit_size, debug_info + die.offset);
					if (x.form)
						node.bitsize = DwarfUtil::formConstant(x);
					x = a.dataForAttribute(DW_AT_bit_offset, debug_info + die.offset);
					if (x.form)
						node.bitposition = node.bytesize * 8 - DwarfUtil::formConstant(x) - node.bitsize;
				}
				break;
			case DW_TAG_volatile_type:
			case DW_TAG_const_type:
			case DW_TAG_typedef:
				dataForType(type, node, type.at(type_node_number).next, flags);
				break;
			case DW_TAG_base_type:
			{
				auto x = a.dataForAttribute(DW_AT_encoding, debug_info + die.offset);
				if (x.form)
					node.base_type_encoding = DwarfUtil::formConstant(x);
			}
				break;
			case DW_TAG_enumeration_type:
				node.enumeration_die = die;
				node.is_enumeration = 1;
				break;
			case DW_TAG_array_type:
				if (type.at(type_node_number).array_dimensions.size())
					node.array_dimensions = type.at(type_node_number).array_dimensions;
				node.children.push_back(DataNode()), dataForType(type, node.children.back(), type.at(type_node_number).next, flags);
				break;
			case DW_TAG_template_type_parameter:
			case DW_TAG_template_value_parameter:
			case DW_TAG_subprogram:
				break;
			default:
				qDebug() << "unhandled tag" << type.at(type_node_number).die.tag << "in" << __func__;
				DwarfUtil::panic();
		}
		type.at(type_node_number).processed = false;
	}
	/*! \todo	this will eventually need to be 64 bit... */
	std::string enumeratorNameForValue(uint32_t value, uint32_t enumeration_die_offset)
	{
		auto die = debug_tree_of_die(enumeration_die_offset).at(0);
		if (die.tag != DW_TAG_enumeration_type)
			DwarfUtil::panic();
		int i;
		for (i = 0; i < die.children.size(); i ++)
		{
			if (die.children.at(i).tag != DW_TAG_enumerator)
				DwarfUtil::panic();
			Abbreviation a(debug_abbrev + die.children.at(i).abbrev_offset);
			auto x = a.dataForAttribute(DW_AT_const_value, debug_info + die.children.at(i).offset);
			if (DwarfUtil::formConstant(x) == value)
			{
				auto x = a.dataForAttribute(DW_AT_name, debug_info + die.children.at(i).offset);
				return DwarfUtil::formString(x.form, x.debug_info_bytes, debug_str);
			}
		}
		return "<<< unknown enumerator value >>>";
	}

	const char * nameOfDie(const struct Die & die, bool is_empty_name_allowed = false)
	{
		struct Abbreviation a(debug_abbrev + die.abbrev_offset);
		auto x = a.dataForAttribute(DW_AT_name, debug_info + die.offset);
		if (!x.form)
		{
			struct Die referred_die(die);
			if (hasAbstractOrigin(die, referred_die))
				return nameOfDie(referred_die);
			else return is_empty_name_allowed ? "" : "<<< no name >>>";
		}
		return DwarfUtil::formString(x.form, x.debug_info_bytes, debug_str);
	}
	void dumpLines(void)
	{
		class DebugLine l(debug_line, debug_line_len);
		l.dump();
		while (l.next())
			l.dump();
	}
	/* the returned vector is sorted by increasing start address */
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
		int i;
		for (i = 0; i < line_addresses.size();)
			if (line_addresses.at(i).address == line_addresses.at(i).address_span)
				line_addresses.erase(line_addresses.begin() + i);
			else
				i ++;
		std::sort(line_addresses.begin(), line_addresses.end());
	}
	std::vector<uint32_t> unfilteredAddressesForFileAndLineNumber(const char * filename, int line_number)
	{
		std::vector<uint32_t> addresses;
		std::vector<struct DebugLine::lineAddress> line_addresses;
		addressesForFile(filename, line_addresses);
		int i;
		for (i = 0; i < line_addresses.size(); i ++)
			if (line_addresses.at(i).line == line_number && line_addresses.at(i).address_span - line_addresses.at(i).address)
				addresses.push_back(line_addresses.at(i).address);
		/* at this point, the addresses are already sorted in ascending order */
		return addresses;
				
	}
	std::vector<uint32_t> filteredAddressesForFileAndLineNumber(const char * filename, int line_number)
	{
		auto addresses = unfilteredAddressesForFileAndLineNumber(filename, line_number);
		std::vector<uint32_t> filtered_addresses;
		std::map<uint32_t /* context die offset */, int> contexts;
		int i;
		for (i = 0; i < addresses.size(); i ++)
		{
			auto x = executionContextForAddress(addresses.at(i));
			if (!x.size())
				filtered_addresses.push_back(addresses.at(i));
			else if (contexts.find(x.back().offset) == contexts.end())
				contexts.operator [](x.back().offset) = 1, filtered_addresses.push_back(addresses.at(i));
		}
		return filtered_addresses;
	}
	void getFileAndDirectoryNamesPointers(std::vector<struct DebugLine::sourceFileNames> & sources)
	{
		/*! \todo maybe add here the (described in the dwarf standard as implicit, if i undertand correctly) zero-indexed
		 * file and directory entries of compilation unit line number programs; these (if i understand correctly) are
		 * the file and directory, specified by the DW_AT_name and DW_AT_comp_dir attributes in a compilation unit die,
		 * respectively; i have not seen any dwarf debug information generated by known compilers that reference these
		 * zero-indexed entries, and some compilers (e.g. IAR) do not even bother emitting a DW_AT_comp_dir attribute,
		 * so these seem redundant, but who knows... */
		/*
		class DebugLine l(debug_line, debug_line_len);
		do
			l.getFileAndDirectoryNamesPointers(string_pointers);
		while (l.next());
		*/
		uint32_t cu;
		class DebugLine l(debug_line, debug_line_len);
		for (cu = 0; cu != -1; cu = next_compilation_unit(cu))
		{
			const char * filename = 0, * compilation_directory = 0;
			auto cu_die_offset = cu + /* skip compilation unit header */ compilation_unit_header(debug_info + cu).header_length();
			auto a = Abbreviation(debug_abbrev + abbreviationOffsetForDieOffset(cu_die_offset));
			auto x = a.dataForAttribute(DW_AT_stmt_list, debug_info + cu_die_offset);
			if (!x.form)
				DwarfUtil::panic();
			l.skipToOffset(DwarfUtil::formConstant(x));
			x = a.dataForAttribute(DW_AT_name, debug_info + cu_die_offset);
			if (x.form)
				filename = DwarfUtil::formString(x.form, x.debug_info_bytes, debug_str);
			x = a.dataForAttribute(DW_AT_comp_dir, debug_info + cu_die_offset);
			if (x.form)
				compilation_directory = DwarfUtil::formString(x.form, x.debug_info_bytes, debug_str);
			/*! \todo	is this line below necessary??? */
			//sources.push_back((struct DebugLine::sourceFileNames) { .file = filename, .directory = compilation_directory, .compilation_directory = compilation_directory, });
			l.getFileAndDirectoryNamesPointers(sources, compilation_directory);
		}
	}

private:
	void fillStaticObjectDetails(const struct Die & die, struct StaticObject & x)
	{
		struct Die referred_die(die);
		if (hasAbstractOrigin(die, referred_die))
		{
			fillStaticObjectDetails(referred_die, x);
			x.die_offset = die.offset;
			return;
		}
		Abbreviation a(debug_abbrev + die.abbrev_offset);
		auto t = a.dataForAttribute(DW_AT_decl_file, debug_info + die.offset);
		x.file = ((t.form) ? DwarfUtil::formConstant(t) : -1);
		t = a.dataForAttribute(DW_AT_decl_line, debug_info + die.offset);
		x.line = ((t.form) ? DwarfUtil::formConstant(t) : -1);
		t = a.dataForAttribute(DW_AT_name, debug_info + die.offset);
		if (t.form)
		{
			switch (t.form)
			{
			case DW_FORM_string:
				x.name = (const char *) t.debug_info_bytes;
				break;
			case DW_FORM_strp:
				x.name = (const char *) (debug_str + * (uint32_t *) t.debug_info_bytes);
				break;
			default:
				DwarfUtil::panic();
			}
		}
		else
			x.name = 0;
		x.name = nameOfDie(die);
		x.die_offset = die.offset;
	}
	void reapStaticObjects(std::vector<struct StaticObject> & data_objects,
	                       std::vector<struct StaticObject> & subprograms,
	                       const struct Die & die)
	{
		int i;
		if (die.isDataObject())
		{
			Abbreviation a(debug_abbrev + die.abbrev_offset);
			uint32_t address;
			auto x = a.dataForAttribute(DW_AT_location, debug_info + die.offset);
			if (x.form && DwarfUtil::isLocationConstant(x.form, x.debug_info_bytes, address))
			{
				StaticObject x;
				fillStaticObjectDetails(die, x);
				x.address = address;
				data_objects.push_back(x);
			}
		}
		else if (die.isSubprogram())
		{
			Abbreviation a(debug_abbrev + die.abbrev_offset);
			auto x = a.dataForAttribute(DW_AT_low_pc, debug_info + die.offset);
			if (x.form || a.dataForAttribute(DW_AT_ranges, debug_info + die.offset).form)
			{
				StaticObject x;
				fillStaticObjectDetails(die, x);
				subprograms.push_back(x);
			}
		}
		for (i = 0; i < die.children.size(); i ++)
			reapStaticObjects(data_objects, subprograms, die.children.at(i));
	}

public:
	void reapStaticObjects(std::vector<struct StaticObject> & data_objects, std::vector<struct StaticObject> & subprograms)
	{
		uint32_t cu;
		for (cu = 0; cu != -1; cu = next_compilation_unit(cu))
		{
			auto die_offset = cu + /* skip compilation unit header */ compilation_unit_header(debug_info + cu).header_length();
			auto dies = debug_tree_of_die(die_offset);
			reapStaticObjects(data_objects, subprograms, dies.at(0));
		}
	}
	std::string locationSforthCode(const struct Die & die, const struct Die & compilation_unit_die, uint32_t address_for_location = -1, uint32_t location_attribute = DW_AT_location)
	{
		Abbreviation a(debug_abbrev + die.abbrev_offset);
		auto x(a.dataForAttribute(location_attribute, debug_info + die.offset));
		if (!x.form)
			return "";
		qDebug() << "processing die offset" << die.offset;
		if (location_attribute == DW_AT_const_value)
		{
			std::stringstream result;
			result << DwarfUtil::formConstant(x) << " " << "DW_AT_const_value";
			return result.str();
		}
		switch (x.form)
		{
			{
				int len;
			case DW_FORM_block1:
				len = * x.debug_info_bytes, x.debug_info_bytes ++; if (0)
			case DW_FORM_block2:
				len = * (uint16_t *) x.debug_info_bytes, x.debug_info_bytes += 2; if (0)
			case DW_FORM_block4:
				len = * (uint32_t *) x.debug_info_bytes, x.debug_info_bytes += 4; if (0)
			case DW_FORM_block:
			case DW_FORM_exprloc:
				len = DwarfUtil::uleb128x(x.debug_info_bytes);
				return DwarfExpression::sforthCode(x.debug_info_bytes, len);
			}
			case DW_FORM_data4:
			case DW_FORM_sec_offset:
			qDebug() << "location list offset:" << * (uint32_t *) x.debug_info_bytes;
				auto l = LocationList::locationExpressionForAddress(debug_loc, * (uint32_t *) x.debug_info_bytes,
					compilation_unit_base_address(compilation_unit_die), address_for_location);
				return l ? DwarfExpression::sforthCode(l + 2, * (uint16_t *) l) : "";
				
				break;
		}

		DwarfUtil::panic();
	}
	std::pair<int /* dwarf expression block length */, const uint8_t * /* dwarf expression bytes */> callSiteValueDwarfExpressionForRegisterNumber(const struct Die & call_site, int register_number)
	{
		std::pair<int, const uint8_t *> x(0, 0);
		auto i = call_site.children.begin();
		while (i != call_site.children.end())
		{
			if (i->tag != DW_TAG_GNU_call_site_parameter)
			{
				i ++;
				continue;
			}
			Abbreviation a(debug_abbrev + i->abbrev_offset);
			auto l = a.dataForAttribute(DW_AT_location, debug_info + i->offset);
			switch (l.form)
			{
				default: DwarfUtil::panic();
				case DW_FORM_block:
				case DW_FORM_exprloc:
					if (DwarfUtil::uleb128x(l.debug_info_bytes) == 1 && * l.debug_info_bytes == DW_OP_reg0 + register_number)
					{
						auto l = a.dataForAttribute(DW_AT_GNU_call_site_value, debug_info + i->offset);
						if (!l.form)
							continue;
						switch (l.form)
						{
							default: DwarfUtil::panic();
							case DW_FORM_block:
							case DW_FORM_exprloc:
								x.first = DwarfUtil::uleb128x(l.debug_info_bytes), x.second = l.debug_info_bytes;
								return x;
						}
					}
					break;
			}
			i ++;
		}
		return x;
	}
	void runTests(void)
	{
		int i, test_count = 0;
		for (i = 0; i < die_fingerprints.size(); i ++)
		{
			Abbreviation a(debug_abbrev + die_fingerprints[i].abbrev_offset);
			auto x = a.dataForAttribute(DW_AT_location, debug_info + die_fingerprints[i].offset);
			switch (x.form)
			{
				{
					int len;
				case DW_FORM_block1:
					len = * x.debug_info_bytes, x.debug_info_bytes ++; if (0)
				case DW_FORM_block2:
					len = * (uint16_t *) x.debug_info_bytes, x.debug_info_bytes += 2; if (0)
				case DW_FORM_block4:
					len = * (uint32_t *) x.debug_info_bytes, x.debug_info_bytes += 4; if (0)
				case DW_FORM_block:
				case DW_FORM_exprloc:
					len = DwarfUtil::uleb128x(x.debug_info_bytes);
					DwarfExpression::sforthCode(x.debug_info_bytes, len);
					test_count ++;
					break;
				}
				case DW_FORM_data4:
				case DW_FORM_sec_offset:
				{
if (DWARF_EXPRESSION_TESTS_DEBUG_ENABLED) qDebug() << "location list at offset" << QString("$%1").arg(* (uint32_t *) x.debug_info_bytes, 0, 16);
					const uint32_t * p((const uint32_t *)(debug_loc + * (uint32_t *) x.debug_info_bytes));
					while (* p || p[1])
					{
						p += 2;
						if (* p != 0xffffffff)
							DwarfExpression::sforthCode((uint8_t *) p + 2, * (uint16_t *) p), p = (uint32_t *)((uint8_t *) p + * (uint16_t *) p + 2), test_count ++;
					}
					break;
				}
			}
		}
		qDebug() << "executed dwarf expression decoding tests, total tests executed:" << test_count;
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
			if (isCIE() && version() > 3)
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
					case DW_CFA_remember_state:
						result << "DW_CFA_remember_state" << " ";
						break;
					case DW_CFA_restore_state:
						result << "DW_CFA_restore_state" << " ";
						break;
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
					case DW_CFA_advance_loc1:
						result << (uint32_t) (* insn ++) << " " << "DW_CFA_advance_loc1" << " ";
						i --;
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
				<< (cie.ciefde_sforth_code() + " initial-cie-instructions-defined " + fde.ciefde_sforth_code()) << " unwinding-rules-defined ";
		return std::pair<std::string, uint32_t>(sfcode.str(), fde.initial_location());
	}
};

#endif /* __LIBTROLL_HXX__ */
