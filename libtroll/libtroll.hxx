/*
Copyright (c) 2016-2017, 2019-2020 Stoyan Shopov

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
#include <functional>

#include <QDebug>
#include <QMessageBox>

/*! \todo	Make this a function */
#define HEX(x) QString("$%1").arg(x, 8, 16, QChar('0'))

#define DEBUG_ADDRESSES_FOR_FILE_ENABLED	0
#define DEBUG_LINE_PROGRAMS_ENABLED	0
#define DEBUG_DIE_READ_ENABLED		0
#define UNWIND_DEBUG_ENABLED		0
#define DWARF_EXPRESSION_TESTS_DEBUG_ENABLED		0

#define STATS_ENABLED			1

enum
{
	/*
	 * HACK HACK HACK
	 *
	 *  Some notes on using compilation unit base addresses.
	 * The Dwarf standard says:
	The base address of a compilation unit is defined as the value of the
	DW_AT_low_pc attribute, if present; otherwise, it is undefined. If the base
	address is undefined, then any DWARF entry or structure defined in terms of the
	base address of that compilation unit is not valid.
	 *
	 * The following cases have been observed:
	 * - a compilation unit which is spans a single contiguous memory region of machine code;
	 * such a compilation unit has both a DW_AT_low_pc and a DW_AT_high_pc attributes, and its base
	 * address is the value of the DW_AT_low_pc
	 * - a compilation unit which spans multiple non-contiguous regions of memory, containing its machine code;
	 * such a compilation unit has a DW_AT_ranges attribute, that describes the compilation unit ranges;
	 * in this case, the gcc compiler still defines a DW_AT_low_pc attribute for the compilation unit
	 * (usually with a value of 0), and it is taken as the base address of the compilation unit
	 * - a compilation unit with neither a DW_AT_ranges attribute, nor a DW_AT_low_pc attributes defined;
	 * this can be the case, for example, for a source code file that does not contain machine code,
	 * but rather only data variable definitions, which are normally of static storage duration, and
	 * reside at constant addresses, i.e. - no location lists are normally needed in such compilation
	 * units, and therefore a compilation unit base address need not be defined
	 * - a compilation unit with a DW_AT_ranges attribute defined, but no defined DW_AT_low_pc;
	 * this has been observed for compilation unit DIEs generated from assembler source code;
	 * in this case, the base address of the compilation unit is undefined;
	 * however, a defined base address is necessary in order to handle range lists correctly;
	 * in this particular case, however, it has been observed that the first entry in the
	 * range list is typically a base address definition entry, and so from then on, there
	 * is a defined base address in order to handle the rest of the range list entries properly
	 *
	 * To handle the cases where an invalid compilation unit base address may be undefined, but still
	 * needed, the special address value (-1) is used, and is checked whenever a valid compilation
	 * unit base address is needed. Strictly speaking, this is a HACK, but it looks reasonably safe
	 * at this time.
	 *
	 * HACK HACK HACK
	 *
	 */
	UNDEFINED_COMPILATION_UNIT_BASE_ADDRESS		= -1,
};


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
	/*! \todo	refactor - switch the 'decoded_len' argument to a reference */
	static uint32_t uleb128(const uint8_t * data, int * decoded_len)
	{
		uint64_t result = 0;
		uint8_t x;
		int shift = * decoded_len = 0;
		do x = * data ++, result |= ((x & 0x7f) << shift), shift += 7, (* decoded_len) ++; while (x & 0x80);
		if ((int64_t) result != (int32_t) result)
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
		if ((int64_t) result != (int32_t) result)
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
		if ((int64_t) result != (int32_t) result)
			panic();
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
		{
			int x;
			return x = uleb128(debug_info_bytes, & bytes_to_skip), bytes_to_skip + skip_form_bytes(x, debug_info_bytes + bytes_to_skip);
		}
		/* DWARF4 introduced forms. */
		case DW_FORM_exprloc:
			return uleb128(debug_info_bytes, & bytes_to_skip) + bytes_to_skip;
		case DW_FORM_flag_present:
			return 0;
		case DW_FORM_sec_offset:
			return 4;
		case DW_FORM_ref_sig8:
			return 8;
		/* DWARF5 introduced forms. */
		case DW_FORM_strx:
			return uleb128(debug_info_bytes, & bytes_to_skip), bytes_to_skip;
		case DW_FORM_addrx:
			return uleb128(debug_info_bytes, & bytes_to_skip), bytes_to_skip;
		//case DW_FORM_ref_sup4:
		//case DW_FORM_strp_sup:
		case DW_FORM_data16:
			return 16;
		//case DW_FORM_line_strp:
		//case DW_FORM_ref_sig8:
		//case DW_FORM_implicit_const:
		case DW_FORM_loclistx:
			return uleb128(debug_info_bytes, & bytes_to_skip), bytes_to_skip;
		case DW_FORM_rnglistx:
			return uleb128(debug_info_bytes, & bytes_to_skip), bytes_to_skip;
		//case DW_FORM_ref_sup8:
		case DW_FORM_strx1:
			return 1;
		case DW_FORM_strx2:
			return 2;
		case DW_FORM_strx3:
			return 3;
		case DW_FORM_strx4:
			return 4;
		case DW_FORM_addrx1:
			return 1;
		case DW_FORM_addrx2:
			return 2;
		case DW_FORM_addrx3:
			return 3;
		case DW_FORM_addrx4:
			return 4;
		}
	}
	/* This function is first introduced for the needs of obtaining constant data for attribute
	 * 'DW_AT_const_value'. It returns a raw array of bytes that hold the constant value data
	 * for this attribute */
	static QByteArray data_block_for_constant_value(const attribute_data & a)
	{
		uint32_t t;
		switch (a.form)
		{
		case DW_FORM_implicit_const:
			t = sleb128(a.debug_abbrev_bytes); if (0)
		case DW_FORM_udata:
			t = uleb128(a.debug_info_bytes); if (0)
		case DW_FORM_sdata:
			t = sleb128(a.debug_info_bytes);
			return QByteArray((const char *) & t, sizeof t);
		case DW_FORM_addr:
		case DW_FORM_data4:
		case DW_FORM_sec_offset:
			return QByteArray((const char *) a.debug_info_bytes, sizeof(uint32_t));
		case DW_FORM_data1:
			return QByteArray((const char *) a.debug_info_bytes, sizeof(uint8_t));
		case DW_FORM_data2:
			return QByteArray((const char *) a.debug_info_bytes, sizeof(uint16_t));
		case DW_FORM_block:
		{
			int bytes_to_skip;
			int x = uleb128(a.debug_info_bytes, & bytes_to_skip);
			return QByteArray((const char *) a.debug_info_bytes + bytes_to_skip, x);
		}
		case DW_FORM_block1:
			return QByteArray((const char *) a.debug_info_bytes + 1, * a.debug_info_bytes);
		case DW_FORM_block2:
			return QByteArray((const char *) a.debug_info_bytes + 2, * (uint16_t *) a.debug_info_bytes);
		case DW_FORM_block4:
			return QByteArray((const char *) a.debug_info_bytes + 4, * (uint32_t *) a.debug_info_bytes);
		default:
			panic();
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
		case DW_FORM_rnglistx:
			return uleb128(a.debug_info_bytes);
		default:
			panic();
		}
	}
	static uint32_t formReference(uint32_t attribute_form, const uint8_t * debug_info_bytes, uint32_t compilation_unit_header_offset)
	{
		/*! \todo handle all reference forms here, including signatures */
		switch (attribute_form)
		{
		case DW_FORM_ref_addr:
			return * (uint32_t *) debug_info_bytes;
		case DW_FORM_ref4:
			return * (uint32_t *) debug_info_bytes + compilation_unit_header_offset;
		case DW_FORM_ref_udata:
			return DwarfUtil::uleb128(debug_info_bytes) + compilation_unit_header_offset;
		default:
			panic();
		}
	}
	static const char * formString(const attribute_data & a, const uint8_t * debug_str, const std::function<const char * (unsigned x)> & resolve_strx)
	{
		uint32_t x;
		switch (a.form)
		{
		case DW_FORM_string:
			return (const char *) a.debug_info_bytes;
		case DW_FORM_strp:
			return (const char *) (debug_str + * (uint32_t *) a.debug_info_bytes);
		case DW_FORM_strx1:
			x = * a.debug_info_bytes;
			return resolve_strx(x);
		default:
			panic();
		}
	}

	static uint32_t fetchHighLowPC(const attribute_data & a, const std::function<uint32_t (unsigned x)> & addrx_resolver, uint32_t relocation_address = 0)
	{
		switch (a.form)
		{
		case DW_FORM_addr:
			return * (uint32_t *) a.debug_info_bytes;
		case DW_FORM_data4:
			return (* (uint32_t *) a.debug_info_bytes) + relocation_address;
		case DW_FORM_addrx:
			return addrx_resolver(DwarfUtil::uleb128(a.debug_info_bytes));
		default:
			panic();
		}
	}
	static bool isLocationConstant(const attribute_data & a, uint32_t & address)
	{
		const uint8_t * debug_info_bytes = a.debug_info_bytes;
		switch (a.form)
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

	/*! \todo	Maybe remove these functions, they are not currently used. */
	/* Routines for determining the dwarf 'class' of an attribute form */
	static bool isClassAddress(uint32_t attribute_form)
	{
		switch (attribute_form)
		{
			case DW_FORM_addr: case DW_FORM_addrx: case DW_FORM_addrx1: case DW_FORM_addrx2:
			case DW_FORM_addrx3: case DW_FORM_addrx4:
				return true;
		default:
			return false;
		}
	}
	static bool isClassAddrptr(uint32_t attribute_form)
	{
		switch (attribute_form)
		{
			case DW_FORM_sec_offset:
				return true;
		default:
			return false;
		}
	}
	static bool isClassBlock(uint32_t attribute_form)
	{
		switch (attribute_form)
		{
			case DW_FORM_block: case DW_FORM_block1: case DW_FORM_block2: case DW_FORM_block4:
				return true;
		default:
			return false;
		}
	}
	static bool isClassConstant(uint32_t attribute_form)
	{
		switch (attribute_form)
		{
			case DW_FORM_data1: case DW_FORM_data2: case DW_FORM_data4: case DW_FORM_data8: case DW_FORM_data16:
			case DW_FORM_sdata: case DW_FORM_udata: case DW_FORM_implicit_const:
				return true;
		default:
			return false;
		}
	}
	static bool isClassExprloc(uint32_t attribute_form)
	{
		switch (attribute_form)
		{
			case DW_FORM_exprloc:
				return true;
		default:
			return false;
		}
	}
	static bool isClassFlag(uint32_t attribute_form)
	{
		switch (attribute_form)
		{
			case DW_FORM_flag: case DW_FORM_flag_present:
				return true;
		default:
			return false;
		}
	}
	static bool isClassLineptr(uint32_t attribute_form)
	{
		switch (attribute_form)
		{
			case DW_FORM_sec_offset:
				return true;
		default:
			return false;
		}
	}
	static bool isClassLoclist(uint32_t attribute_form)
	{
		switch (attribute_form)
		{
			case DW_FORM_sec_offset: case DW_FORM_loclistx:
				return true;
		default:
			return false;
		}
	}
	static bool isClassLoclistptr(uint32_t attribute_form)
	{
		switch (attribute_form)
		{
			case DW_FORM_sec_offset:
				return true;
		default:
			return false;
		}
	}
	static bool isClassMacptr(uint32_t attribute_form)
	{
		switch (attribute_form)
		{
		        case DW_FORM_sec_offset:
				return true;
		default:
			return false;
		}
	}
	static bool isClassRnglist(uint32_t attribute_form)
	{
		switch (attribute_form)
		{
			case DW_FORM_sec_offset: case DW_FORM_rnglistx:
				return true;
		default:
			return false;
		}
	}
	static bool isClassRnglistptr(uint32_t attribute_form)
	{
		switch (attribute_form)
		{
			case DW_FORM_sec_offset:
				return true;
		default:
			return false;
		}
	}
	static bool isClassReference(uint32_t attribute_form)
	{
		switch (attribute_form)
		{
			case DW_FORM_ref1: case DW_FORM_ref2: case DW_FORM_ref4: case DW_FORM_ref8: case DW_FORM_ref_udata:
			case DW_FORM_ref_addr: case DW_FORM_ref_sig8: case DW_FORM_ref_sup4: case DW_FORM_ref_sup8:
				return true;
		default:
			return false;
		}
	}
	static bool isClassString(uint32_t attribute_form)
	{
		switch (attribute_form)
		{
			case DW_FORM_string:
			case DW_FORM_strp:
			case DW_FORM_line_strp:
			case DW_FORM_strp_sup:
			case DW_FORM_strx:
			case DW_FORM_strx1:
			case DW_FORM_strx2:
			case DW_FORM_strx3:
			case DW_FORM_strx4:
				return true;
		default:
			return false;
		}
	}
	static bool isClassStroffsetptr(uint32_t attribute_form)
	{
		switch (attribute_form)
		{
			case DW_FORM_sec_offset:
				return true;
		default:
			return false;
		}
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

/* !!! Warning - this can generally be a graph containing cycles - beware of recursion when processing !!! */
struct DwarfTypeNode
{
	struct Die	die;
	/* A value of -1 denotes an invalid index. */
	int		next;
	/* flags to facilitate recursion when processing this data structure for various purposes */
	bool		processed;
	bool		type_print_recursion_flag;

	std::vector<uint32_t>	children;
	/* This is needed to support variable length arrays.
	 * The actual length of such arrays usually can only be computed
	 * at runtime, and to do this, a value of the program counter
	 * may be needed, so it is the responsibility of the caller to
	 * compute the actual dimension value when needed. */
	struct array_dimension
	{
		/* If this is nonzero, then the array dimension value needs
		 * to be calculated from the 'subrange_die_offset',
		 * and the constant_value should be ignored. */
		uint32_t	subrange_die_offset;
		uint32_t	constant_value;
		array_dimension(uint32_t constant_value = 0, uint32_t upper_bound_die_offset = 0)
		{ this->constant_value = constant_value, this->subrange_die_offset = upper_bound_die_offset; }
	};
	std::vector<array_dimension>	array_dimensions;

	DwarfTypeNode(const struct Die & xdie) : die(xdie)	{ next = -1; type_print_recursion_flag = processed = false; }
};

struct DwarfBaseType
{
	uint32_t bytesize, dwarfEncoding;
	DwarfBaseType(uint32_t size, uint32_t encoding) : bytesize(size), dwarfEncoding(encoding) {}
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
		/*
		if (a.form == DW_FORM_indirect)
			DwarfUtil::panic();
			*/
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
	uint8_t		dwarf5_unit_type() const { /* introduced in dwarf 5 */ if (version() != 5) DwarfUtil::panic(); return*(uint8_t*)(data+6);}
	uint32_t	header_length() const
	{
		switch (version())
		{
		default: DwarfUtil::panic();
		case 2: case 3: case 4:
			return 11;
		case 5:
			switch (dwarf5_unit_type())
			{
				case DW_UT_compile:
				case DW_UT_partial:
					return 12;
				case DW_UT_type:
					return 24;
				case DW_UT_skeleton:
				case DW_UT_split_compile:
				case DW_UT_split_type:
					return 20;
			}
		}
	}

	/* Note that it is the caller's responsibility to make sure that this is indeed a type unit for
	 * dwarf versions prior to 5, because in dwarf versions prior to 5, the only way to discriminate
	 * between compilation units and type units is to know in which section ('.debug_info' or .'debug_type),
	 * the unit resides, and the code here does not know that. */ 
	uint64_t	type_unit_type_signature(void) const
	{
		switch (version())
		{
			default:
				DwarfUtil::panic();
			case 4:	
				return*(uint64_t*)(data+11);
			case 5:	
				if (dwarf5_unit_type() == DW_UT_type)
					return*(uint64_t*)(data+12);
				else	
					DwarfUtil::panic();
		}
	}
	uint64_t	type_unit_type_offset() const
	{
		switch (version())
		{
			default:
				DwarfUtil::panic();
			case 4:	
				return*(uint32_t*)(data+19);
			case 5:	
				if (dwarf5_unit_type() == DW_UT_type)
					return*(uint32_t*)(data+22);
				else	
					DwarfUtil::panic();
		}
	}
	uint32_t	dwarf4_type_unit_header_length() const { return 23; }

	compilation_unit_header(const uint8_t * data) { this->data = data; validate(); }
	struct compilation_unit_header & next(void) { data += unit_length() + sizeof(uint32_t); return * this; }
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
			if (dwarf5_unit_type() != DW_UT_compile)
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
			else
			{
				if (compilation_unit_base_address == UNDEFINED_COMPILATION_UNIT_BASE_ADDRESS)
					DwarfUtil::panic();
				if (p[0] + compilation_unit_base_address <= address_for_location && address_for_location < p[1] + compilation_unit_base_address)
				return (const uint8_t *) (p + 2);
			}
			p += 2;
			p = (const uint32_t *)((uint8_t *) p + * (uint16_t *) p + 2);
		}
		return 0;
	}
};

/*! \todo	!!! Test all of the unsupported codes. Explicitly document them as tested/not tested !!! This is extremely important !!! */
struct DwarfExpression
{
	static std::string sforthCode(const uint8_t * dwarf_expression, uint32_t expression_len,
		/* This parameter has been introduced because some dwarf opcodes (e.g. 'DW_OP_regval_type')
		 * take as arguments offset values that are relative to the compilation unit that
		 * contains the dwarf expression being processed. It is simpler to turn the relative
		 * offset into an absolute offset here, than to hadle this relocation at some later point.
		 * In case the compilation unit header offset is needed, this parameter is used to obtain it. */
		uint32_t compilation_unit_header_offset,
		bool dwarf_test_mode_active = false)
	{
		std::stringstream x;
		int bytes_to_skip, i, branch_counter = 0x10000;
		while (expression_len --)
		{
			bytes_to_skip = 0;
			/* IMPORTANT: Never modify the 'dwarf_expression' pointer in the 'switch' statement below!
			 * Only make sure to set properly the 'bytes_to_skip' count. The 'dwarf_expression'
			 * pointer will then be properly updated after the 'switch' statement */
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
					x << "( disassembled entry value expression: DW_OP_GNU_entry_value-start " << sforthCode(dwarf_expression + bytes_to_skip, i, compilation_unit_header_offset, dwarf_test_mode_active) << " DW_OP_GNU_entry_value-end ) ";
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
					x << "DW_OP_or ";
					break;
				case DW_OP_xor:
					x << "DW_OP_xor ";
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
				/*!	\todo	Test this! */
					x << "GNU-PARAMETER-REF-UNSUPPORTED!!! ";
					bytes_to_skip = sizeof(uint32_t);
					break;
				case DW_OP_consts:
					x << "CONSTS-UNSUPPORTED!!! ";
					DwarfUtil::sleb128(dwarf_expression, & bytes_to_skip);
					break;
				case DW_OP_constu:
					x << DwarfUtil::uleb128(dwarf_expression, & bytes_to_skip) << " ( dw_op_constu ) ";
					break;
				case DW_OP_GNU_convert:
				case DW_OP_convert:
				{
					uint32_t base_type_die_offset;
					base_type_die_offset = DwarfUtil::uleb128(dwarf_expression, & bytes_to_skip);
					/* The die offset just read is an offset from the compilation unit that the die resides in.
					 * Turn it into an absolute die offset, i.e. an offset from the start of the '.debug_info' section.
					 * The value zero is a special case, and denotes the, so-called, 'generic' type */
					x << (base_type_die_offset ? base_type_die_offset + compilation_unit_header_offset : 0)<< " ";
					x << "DW_OP_convert ";
					break;
				}
				case DW_OP_GNU_implicit_pointer:
					x << "GNU-IMPLICIT-POINTER-UNSUPPORTED!!! ";
					DwarfUtil::sleb128(dwarf_expression + sizeof(uint32_t), & bytes_to_skip);
					bytes_to_skip += sizeof(uint32_t);
					break;
				case DW_OP_GNU_regval_type:
				case DW_OP_regval_type:
				{
					uint32_t base_type_die_offset;
					x << DwarfUtil::uleb128(dwarf_expression, & i) << " ";
					base_type_die_offset = DwarfUtil::uleb128(dwarf_expression + i, & bytes_to_skip);
					/* The die offset just read is an offset from the compilation unit that the die resides in.
					 * Turn it into an absolute die offset, i.e. an offset from the start of the '.debug_info' section */
					x << base_type_die_offset + compilation_unit_header_offset << " ";
					x << "DW_OP_regval_type ";
					bytes_to_skip += i;
					break;
				}
				case DW_OP_piece:
					x << DwarfUtil::uleb128(dwarf_expression, & bytes_to_skip) << " ";
					x << "DW_OP_piece ";
					break;
				case DW_OP_bit_piece:
				{
					int bitsize = DwarfUtil::uleb128(dwarf_expression, & i);
					int bitoffset = DwarfUtil::uleb128(dwarf_expression + i, & bytes_to_skip);
					bytes_to_skip += i;
					x << bitoffset << " " << bitsize << " " << "DW_OP_bit_piece ";
					break;
				}
				case DW_OP_implicit_value:
				{
					int size = DwarfUtil::uleb128(dwarf_expression, & bytes_to_skip);
					QByteArray data_bytes((const char *) dwarf_expression + bytes_to_skip, size);
					bytes_to_skip += size;
					for (i = size - 1; i >= 0; -- i)
						x << (unsigned) (uint8_t) data_bytes.at(i) << " ";
					x << size << " ";
					x << "DW_OP_implicit_value ";
					break;
				}
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
				case DW_OP_GNU_const_type:
				case DW_OP_const_type:
				{
					uint32_t base_type_die_offset, size;
					base_type_die_offset = DwarfUtil::uleb128(dwarf_expression, & bytes_to_skip);
					/* The die offset just read is an offset from the compilation unit that the die resides in.
					 * Turn it into an absolute die offset, i.e. an offset from the start of the '.debug_info' section */
					size = dwarf_expression[bytes_to_skip];
					++ bytes_to_skip;
					QByteArray data_bytes((const char *) dwarf_expression + bytes_to_skip, size);
					bytes_to_skip += size;
					for (i = size - 1; i >= 0; -- i)
						x << (unsigned) (uint8_t) data_bytes.at(i) << " ";
					x << size << " ";
					x << base_type_die_offset + compilation_unit_header_offset << " ";
					x << "DW_OP_const_type ";
					break;
				}
				case DW_OP_GNU_deref_type:
				case DW_OP_deref_type:
				{
					uint32_t base_type_die_offset, size;
					size = * dwarf_expression;
					bytes_to_skip = 1;
					base_type_die_offset = DwarfUtil::uleb128(dwarf_expression + 1, & i);
					bytes_to_skip += i;
					/* The die offset just read is an offset from the compilation unit that the die resides in.
					 * Turn it into an absolute die offset, i.e. an offset from the start of the '.debug_info' section */
					x << size << " ";
					x << base_type_die_offset + compilation_unit_header_offset << " ";
					x << "DW_OP_deref_type ";
					break;
				}
				default:
					if (dwarf_test_mode_active)
					{
						QString s = QString("unsupported dwarf opcode: $%1\n").arg(dwarf_expression[-1], 0, 16);
						x << s.toStdString();
						qDebug() << s;
						goto abort;
					}
					DwarfUtil::panic();
			}
			dwarf_expression += bytes_to_skip;
			expression_len -= bytes_to_skip;
			if (expression_len < 0)
				DwarfUtil::panic();
			branch_counter -= bytes_to_skip + /* one more byte for the dwarf opcode just processed */ 1;
			if (branch_counter < 0)
				DwarfUtil::panic();
			else if (!branch_counter)
				x << "[then] ", branch_counter = 0x10000;
		}
abort:
		return x.str();
	}
	static int registerNumberOfRegXOpcode(unsigned opcode) { return (DW_OP_reg0 <= opcode && opcode <= DW_OP_reg31) ? opcode - DW_OP_reg0 : -1;
	}
};

/*! \todo	Line number debug information is broken and unchecked for dwarf versions 2 and 3 - fix and test this. */
/*! \todo	There is much room for optimizations (even trivial) here. */
class DebugLine
{
	/*
Dwarf3 line number program header layout

offset	name					size
0	unit_length 				4
4	version					2
6	header_length				4
10	minimum_instruction_length		1
11	default_is_stmt				1
12	line_base				1
13	line_range				1
14	opcode_base				1
15	standard_opcode_lengths			variable: n = opcode_base - 1
15 + n	include_directories			sequence of path names, variable
						last entry is followed by a single
						null byte
x	file_names				sequence of file entries;
						each entry has the following format:
						- null terminated string, file name
						- uleb128 number - directorry index for
						the file name
						- uleb128 number - time of last modification, 0 if unavailable
						- uleb128 number - file size in bytes, 0 if unavailable


Dwarf4 line number program header layout

offset	name					size
0	unit_length 				4
4	version					2
6	header_length				4
10	minimum_instruction_length		1
11	maximum_operations_per_instruction	1
12	default_is_stmt				1
13	line_base				1
14	line_range				1
15	opcode_base				1
16	standard_opcode_lengths			variable: n = opcode_base - 1
16 + n	include_directories			sequence of path names, variable
						last entry is followed by a single
						null byte
x	file_names				sequence of file entries;
						each entry has the following format:
						- null terminated string, file name
						- uleb128 number - directorry index for
						the file name
						- uleb128 number - time of last modification, 0 if unavailable
						- uleb128 number - file size in bytes, 0 if unavailable
Dwarf5 line number program header layout

offset	name					size
0	unit_length 				4
4	version					2
6	address_size				1
7	segment_selector_size			1
8	header_length				4
12	minimum_instruction_length		1
13	maximum_operations_per_instruction	1
14	default_is_stmt				1
15	line_base				1
16	line_range				1
17	opcode_base				1
18	standard_opcode_lengths			variable: n = opcode_base - 1
18 + n	directory_entry_format_count		1
... finish this
*/
private:

	const uint8_t *	debug_line, * header;
	uint32_t	debug_line_len;
	
	uint32_t unit_length(void) { return * (uint32_t *) header; }
	uint16_t version(void) { return * (uint16_t *) (header + 4); }
	uint32_t header_length(void)
	{
		if (version() < 5) return * (uint32_t *) (header + 6);
		else if (version() == 5) return * (uint32_t *) (header + 8);
		else DwarfUtil::panic();
	}
	uint8_t minimum_instruction_length(void)
	{
		switch (version())
		{
		default: DwarfUtil::panic();
		case 3: return * (uint8_t *) (header + 10);
		case 4: return * (uint8_t *) (header + 10);
		case 5: return * (uint8_t *) (header + 12);
		}
	}
	/* Introduced in DWARF 4 */
	uint8_t maximum_operations_per_instruction(void)
	{
		switch (version())
		{
		default: DwarfUtil::panic();
		case 4: return * (uint8_t *) (header + 11);
		case 5: return * (uint8_t *) (header + 13);
		}
	}
	uint8_t default_is_stmt(void)
	{
		switch (version())
		{
		default: DwarfUtil::panic();
		case 3: return * (uint8_t *) (header + 11);
		case 4: return * (uint8_t *) (header + 12);
		case 5: return * (uint8_t *) (header + 14);
		}
	}
	int8_t line_base(void)
	{
		switch (version())
		{
		default: DwarfUtil::panic();
		case 3: return * (uint8_t *) (header + 12);
		case 4: return * (uint8_t *) (header + 13);
		case 5: return * (uint8_t *) (header + 15);
		}
	}
	uint8_t line_range(void)
	{
		switch (version())
		{
		default: DwarfUtil::panic();
		case 3: return * (uint8_t *) (header + 13);
		case 4: return * (uint8_t *) (header + 14);
		case 5: return * (uint8_t *) (header + 16);
		}
	}
	uint8_t opcode_base(void)
	{
		switch (version())
		{
		default: DwarfUtil::panic();
		case 3: return * (uint8_t *) (header + 14);
		case 4: return * (uint8_t *) (header + 15);
		case 5: return * (uint8_t *) (header + 17);
		}
	}
	/*! \todo	Field 'standard_opcode_lengths' is not handled at all! Add support for this! */
	int standard_opcode_lengths_field_offset(void)
	{
		switch (version())
		{
		default: DwarfUtil::panic();
		case 3: return 15;
		case 4: return 16;
		case 5: return 18;
		}
	}
	const char * include_directories(void)
	{
		return (const char *) (header + standard_opcode_lengths_field_offset() + opcode_base() - 1);
	}
	const char * file_names(void)
	{
		auto p = include_directories();
		size_t x;
		while ((x = strlen(p)))
			p += x + 1;
		return p + 1;
	}
#endif
	const uint8_t * line_number_program(void) { return header + sizeof unit_length() + sizeof version() + sizeof header_length() + header_length(); }
	struct line_state { uint32_t file, address, is_stmt; int line, column; } registers[2], * current_line_state, * prev_line_state;
	void initLineState(void) { registers[0] = (struct line_state) { .file = 1, .address = 0, .is_stmt = default_is_stmt(), .line = 1, .column = 0, };
				registers[1] = (struct line_state) { .file = 1, .address = 0xffffffff, .is_stmt = 0, .line = -1, .column = -1, };
				current_line_state = registers, prev_line_state = registers + 1; }
	void swap_line_states(void) { struct line_state * x(current_line_state); current_line_state = prev_line_state; prev_line_state = x; }
	void validateHeader(void)
	{
		if (version() != 2 && version() != 3 && version() != 4) DwarfUtil::panic();
		if (version() == 4 && maximum_operations_per_instruction() != 1) DwarfUtil::panic();
	}

	/* Decodes the line number program, executing the supplied visitor function each time a new row is appended to the line number matrix.
	 * If the visitor function returns true, continue decoding the line number program, otherwise return. */
	void decode_line_number_program(std::function<bool (const struct line_state * current_line_state, const struct line_state * prev_line_state)>& visitor)
	{
		validateHeader();
		const uint8_t * p(line_number_program()), op_base(opcode_base()), lrange(line_range());
		const uint8_t * opcode_lengths = standard_opcode_lengths();
		int lbase(line_base());
		uint32_t min_insn_length(minimum_instruction_length());
		int len, x;
		initLineState();
#if XXX
		BROKEN!!! FIX THIS!!!
		if (DEBUG_LINE_PROGRAMS_ENABLED)
		{
			DwarfUtil::panic("broken for dwarf 5");
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
#endif
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
						if (!visitor(current_line_state, prev_line_state))
							return;
						initLineState();
						if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "end of sequence";
						break;
					case DW_LNE_set_address:
						if (len != 5) DwarfUtil::panic();
						current_line_state->address = * (uint32_t *) p;
						p += sizeof current_line_state->address;
						if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "extended opcode, set address to" << HEX(current_line_state->address);
						break;
				}
			}
			else if (* p >= op_base)
			{
				/* special opcodes */
				uint8_t x = * p ++ - op_base;
				current_line_state->address += (x / lrange) * min_insn_length;
				current_line_state->line += lbase + x % lrange;
				if (!visitor(current_line_state, prev_line_state))
					return;
				if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "special opcode, set address to" << HEX(current_line_state->address) << "line to" << current_line_state->line;
				swap_line_states();
				* current_line_state = * prev_line_state;
			}
			/* standard opcodes */
			else switch (* p ++)
			{
				default:
				{
					DwarfUtil::panic();
					/*! \todo	print a proper warning here */
					qDebug() << "WARNING: unsupported dwarf line number program opcode:" << p[-1] << "skipping opcode";
					/* Skip over opcode arguments. */
					for (int i = opcode_lengths[p[-1] - 1]; i--;)
						DwarfUtil::uleb128x(p);
				}

					break;
				case DW_LNS_set_prologue_end:
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set prologue end to true";
					break;
				case DW_LNS_copy:
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "copy";
					if (!visitor(current_line_state, prev_line_state))
						return;
					swap_line_states();
					* current_line_state = * prev_line_state;
					break;
				case DW_LNS_advance_pc:
					current_line_state->address += DwarfUtil::uleb128(p, & len) * min_insn_length;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "advance pc to" << HEX(current_line_state->address);
					p += len;
					break;
				case DW_LNS_advance_line:
					current_line_state->line += DwarfUtil::sleb128(p, & len);
					p += len;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "advance line to" << current_line_state->line;
					break;
				case DW_LNS_const_add_pc:
					current_line_state->address += ((255 - op_base) / lrange) * min_insn_length;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "advance pc to" << HEX(current_line_state->address);
					break;
				case DW_LNS_set_file:
					current_line_state->file = DwarfUtil::uleb128(p, & len);
					p += len;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set file to" << current_line_state->file;
					break;
				case DW_LNS_set_column:
					current_line_state->column = DwarfUtil::uleb128(p, & len);
					p += len;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set column to" << current_line_state->column;
					break;
				case DW_LNS_negate_stmt:
					current_line_state->is_stmt = ! current_line_state->is_stmt;
					if (DEBUG_LINE_PROGRAMS_ENABLED) qDebug() << "set is_stmt to " << current_line_state->is_stmt;
					break;
			}
		}
	}

public:
	struct lineAddress
	{
		uint32_t line, address, address_span;
		bool is_stmt;
		struct lineAddress * next;
		lineAddress(void) { line = address = address_span = -1; next = 0; is_stmt = false; }
		bool operator < (const struct lineAddress & rhs) const { return address < rhs.address; }
	};
	struct sourceFileNames { const char * file, * directory, * compilation_directory; };
	DebugLine(const uint8_t * debug_line, uint32_t debug_line_len)
	{ header = this->debug_line = debug_line, this->debug_line_len = debug_line_len; validateHeader(); }

	void dump(void)
	{
		std::function<bool (const struct line_state * current_line_state, const struct line_state * prev_line_state)>
		visitor = [&](const struct line_state * current_line_state, const struct line_state * prev_line_state) -> bool
		{ return true; };
		decode_line_number_program(visitor);
	}
	/* returns -1 if no line number was found */
	uint32_t lineNumberForAddress(uint32_t target_address, uint32_t statement_list_offset, uint32_t & file_number, bool & is_address_on_exact_line_number_boundary)
	{
		file_number = 0;
		uint32_t lineNumber = -1;
		is_address_on_exact_line_number_boundary = false;
		header = debug_line + statement_list_offset;

		std::function<bool (const struct line_state * current_line_state, const struct line_state * prev_line_state)>
		visitor = [&](const struct line_state * current_line_state, const struct line_state * prev_line_state) -> bool
		{
			if (prev_line_state->address <= target_address && target_address < current_line_state->address)
			{
				if (prev_line_state->address == target_address && prev_line_state->is_stmt)
					is_address_on_exact_line_number_boundary = true;
				file_number = prev_line_state->file, lineNumber = prev_line_state->line;
				return false;
			}
			return true;
		};

		decode_line_number_program(visitor);
		return lineNumber;
	}

	void addressesForFile(uint32_t file_number, std::vector<struct lineAddress> & line_addresses)
	{
		std::function<bool (const struct line_state * current_line_state, const struct line_state * prev_line_state)>
		visitor = [&](const struct line_state * current_line_state, const struct line_state * prev_line_state) -> bool
		{
			if (current_line_state->file == file_number)
			{
				struct lineAddress line_data;
				line_data.address = prev_line_state->address;
				line_data.is_stmt = prev_line_state->is_stmt;
				line_data.line = prev_line_state->line;
				line_data.address_span = current_line_state->address;
				line_addresses.push_back(line_data);
			}
			return true;
		};
		decode_line_number_program(visitor);
	}


	/* Returns -1 if the file is not found in the file name table of the current line number program. */
	int fileNumber(const char * filename)
	{
		if (version() > 4)
			DwarfUtil::panic();
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
		return -1;
		
	}
	void stringsForFileNumber(uint32_t file_number, const char * & file_name, const char * & directory_name, const char * compilation_directory)
	{
		if (version() > 4)
			DwarfUtil::panic();
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
		if (version() > 4)
			DwarfUtil::panic();
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


struct AddressRange
{
protected:
	/* The end address is NOT included in the address range, i.e. this is the first address location after this address range */
	struct address_range { uint32_t start_address = -1, end_address = -1; };
	std::list<struct address_range> ranges;
public:
	AddressRange(uint32_t start_address, uint32_t end_address) { ranges.push_back((struct address_range){ .start_address = start_address, .end_address = end_address, });}
	AddressRange(void){}
	bool isAddressInRange(uint32_t address) const { for (const auto& range : ranges) if (range.start_address <= address && address < range.end_address) return true; return false; }
	virtual void dump(void) const { qDebug() << "Address range count:" << ranges.size(); for (const auto& r: ranges) qDebug() << HEX(r.start_address) << "-" << HEX(r.end_address); }
};

struct pre_dwarf5_address_range : public AddressRange
{
	pre_dwarf5_address_range(const uint8_t * debug_ranges_section, uint32_t debug_ranges_section_offset, uint32_t base_address)
	{
		auto range_list = (const uint32_t *) (debug_ranges_section + debug_ranges_section_offset);
		while (range_list[0] && range_list[1])
		{
			if (range_list[0] == -1)
				/* A base address selection entry */
				base_address = range_list[1];
			else
			{
				if (base_address == UNDEFINED_COMPILATION_UNIT_BASE_ADDRESS)
					DwarfUtil::panic();
				ranges.push_back((address_range) { .start_address = range_list[0] + base_address, .end_address = range_list[1] + base_address});
			}
			range_list += 2;
		}
	}
};


struct dwarf5_rangelist_header : public AddressRange
{
private:
	uint32_t offset_entry_count(void) const { return *(uint32_t *) (header + 8); }
	uint32_t * offset_array(void) const { return (uint32_t *) (header + header_length()); }
public:
	static int header_length(void) { return 12; }
	/* This constructor decodes a range list, given a range table, and a range list index.
	 * It corresponds to a DW_FORM_rnglistx form encoding of the DW_AT_ranges attribute. */
	dwarf5_rangelist_header(const uint8_t * header, int range_index, const uint32_t * address_table, uint32_t base_address = UNDEFINED_COMPILATION_UNIT_BASE_ADDRESS)
		: header(header)
	{
		decode_rangelist(range_index, address_table, base_address);
	}
	/* This constructor decodes a range list, given an offset in the '.debug_rnglists' section.
	 * It corresponds to a DW_FORM_sec_offset form encoding of the DW_AT_ranges attribute. */
	dwarf5_rangelist_header(const uint8_t * debug_rnglists, uint32_t offset, const uint32_t * address_table, uint32_t base_address = UNDEFINED_COMPILATION_UNIT_BASE_ADDRESS)
	        : header(header)
	{
		decode_rangelist(debug_rnglists + offset, address_table, base_address);
	}
private:
	void decode_rangelist(int range_index, const uint32_t * address_table, uint32_t base_address = UNDEFINED_COMPILATION_UNIT_BASE_ADDRESS)
	{
		if (range_index >= offset_entry_count())
		{
			qDebug() << "error: range list index out of bounds";
			return;
		}
		return decode_rangelist(header + header_length() + offset_array()[range_index], address_table, base_address);
	}
	void decode_rangelist(const uint8_t * rangelist_bytes, const uint32_t * address_table, uint32_t base_address = UNDEFINED_COMPILATION_UNIT_BASE_ADDRESS)
	{
		const uint8_t * p(rangelist_bytes);

		while (1)
		{
			switch (*p++)
			{
			case DW_RLE_end_of_list:
				qDebug() << "<end of range list>";
				return;
			case DW_RLE_base_addressx:
				assert(address_table);
				base_address = address_table[DwarfUtil::uleb128x(p)];
				break;
			case DW_RLE_startx_endx:
				assert(address_table);
			{
				uint32_t start = address_table[DwarfUtil::uleb128x(p)], end = address_table[DwarfUtil::uleb128x(p)];
				ranges.push_back((struct address_range) { .start_address = start, .end_address = end, });
			}
				break;
			case DW_RLE_startx_length:
				assert(address_table);
			{
				uint32_t addr = address_table[DwarfUtil::uleb128x(p)], len = DwarfUtil::uleb128x(p);
				ranges.push_back((struct address_range) { .start_address = addr, .end_address = addr + len, });
			}
				break;
			case DW_RLE_offset_pair:
				assert(base_address != UNDEFINED_COMPILATION_UNIT_BASE_ADDRESS);
			{
				uint32_t o1 = DwarfUtil::uleb128x(p), o2 = DwarfUtil::uleb128x(p);
				ranges.push_back((struct address_range) { .start_address = base_address + o1, .end_address = base_address + o2, });
			}
				break;
			case DW_RLE_base_address:
				base_address = *(uint32_t*)p;
				p += sizeof(uint32_t);
				break;
			case DW_RLE_start_end:
				assert(base_address != UNDEFINED_COMPILATION_UNIT_BASE_ADDRESS);
				ranges.push_back((struct address_range) { .start_address = *(uint32_t*)p, .end_address = *(uint32_t*)(p + sizeof(uint32_t)), });
				p += 2 * sizeof(uint32_t);
				break;
			case DW_RLE_start_length:
				assert(base_address != UNDEFINED_COMPILATION_UNIT_BASE_ADDRESS);
			{
				uint32_t start = *(uint32_t*)p, end = start + (p += sizeof(uint32_t), DwarfUtil::uleb128x(p));
				ranges.push_back((struct address_range) { .start_address = start, .end_address = end, });
			}
				break;
			default:
				qDebug() << QString("error: bad range list encoding, aborting range decoding: $%1").arg(p[-1], 2, 16, QChar(0));
				ranges.clear();
				return;
			}
		}
	}
	const uint8_t * header = 0;

};

/*! \todo	Check for functions duplicating functionality. This is quite possible, because I have
 * 		not done any development for almost 3 years as of february 2019, and I am now resuming
 * 		development on the troll, and I am adding new code */
class DwarfData
{
private:

	/* If a '.debug_types' dwarf section is present, assume it is a DWARF4 formatted
	 * list of type units. The DWARF standard defines this section only for DWARF4, and
	 * type units are merged into the '.debug_info' section in DWARF5, which is a very
	 * fine decision. Also, in DWARF5, there are other dwarf units defined, other than
	 * a compilation unit and a type unit, and these units all reside in the '.debug_info'
	 * section for DWARF5. In DWARF5, the type of a dwarf unit is determined by a 'type' field
	 * in the unit header, but there is no such 'type' fielf prior to DWARF5, so in DWARF4,
	 * the discrimination between compilation units and type units is a function of in which
	 * section ('.debug_info', or '.debug_types') a dwarf unit resides - all units in the
	 * '.debug_info' section are compilation units, and all units in the '.debug_types'
	 * section are type units.
	 *
	 * That said, handling only one section ('.debug_info') to contain all dwarf units and
	 * all DIEs would be much more convenient than to deal with separate '.debug_info'
	 * and '.debug_types'. Perform a HACK, and append the '.debug_types' section
	 * to the '.debug_info' section, and deal with only one section from then on.
	 * The only way that a DIE can refer to a DIE in a type unit (and then, not to just
	 * any DIE in the type unit, but to a single 'type' DIE that the type unit exports)
	 * is through a DW_FORM_ref_sig8 reference, so also maintain a map that maps
	 * signatures to DIE offsets - for each signature of a type unit - provide the DIE
	 * offset of the type that the type unit exports.
	 *
	 * In short, the 'debug_info_bytes' array is THE master internally used '.debug_info' section
	 * that contains all dwarf units and all DIEs, and in the case of DWARF4 where, the
	 * compilation units and type units reside in separate sections (namely, '.debug_info'
	 * and '.debug_types'), some HACKS are performed to unify the two sections in a single
	 * section, so that handling of DWARF debug information is simpler and more consistent
	 * than it would be if the two sections ('.debug_info' and '.debug_types') were handled
	 * separately. */
	/*! \todo Copy all dwarf debug sections supplied to the constructor into internal byte vectors */
	std::vector<uint8_t> debug_info_bytes;
	const uint8_t * debug_info;
	uint32_t	debug_info_len;
	uint32_t	debug_types_len;
	/* A map from a Dwarf type signature (DW_FORM_ref_sig8) to the DIE offset that a type unit exports. */
	std::map</* type signature */ uint64_t, /* exported type unit DIE offset */ uint32_t> typeSignatureMap;

	const uint8_t	* debug_abbrev;
	uint32_t	debug_abbrev_len;

	const uint8_t * debug_ranges;
	uint32_t	debug_ranges_len;
	
	const uint8_t * debug_str;
	uint32_t	debug_str_len;
	
	const uint8_t * debug_line;
	uint32_t	debug_line_len;
	
	const uint8_t * debug_loc;
	uint32_t	debug_loc_len;

	const uint8_t * debug_rnglists;
	uint32_t	debug_rnglists_len;

	const uint8_t * debug_loclists;
	uint32_t	debug_loclists_len;

	const uint8_t * debug_addr;
	uint32_t	debug_addr_len;

	const uint8_t * debug_str_offsets;
	uint32_t	debug_str_offsets_len;

	std::function<const char * (unsigned x)> getStrxResolver(uint32_t die_offset)
	{
		return [this, die_offset](unsigned x) -> const char *
		{
			auto cu = compilationUnitFingerprints.at(cuFingerprintIndexForDieOffset(die_offset));
			return (const char *)(debug_str + ((uint32_t *)(debug_str_offsets + cu.str_offsets_base))[x]);
		};
	}

	std::function<uint32_t (unsigned x)> getAddrxResolver(uint32_t die_offset)
	{
		return [this, die_offset](unsigned x) -> uint32_t
		{
			auto cu = compilationUnitFingerprints.at(cuFingerprintIndexForDieOffset(die_offset));
			return x[((uint32_t *)(debug_addr + cu.addr_base_section_offset))];
		};
	}

	AddressRange getRangeOfDie(const Die & die, const std::function<uint32_t (unsigned x)> & addrx_resolver)
	{
		struct AddressRange r;

		struct Abbreviation a(debug_abbrev + die.abbrev_offset);
		auto range = a.dataForAttribute(DW_AT_ranges, debug_info + die.offset);
		if (range.form)
		{
			const auto & cu = compilationUnitFingerprints.at(cuFingerprintIndexForDieOffset(die.offset));
			if (range.form == DW_FORM_sec_offset)
			{
				auto range_offset = DwarfUtil::formConstant(range);
				/* In dwarf5, the range lists have a new encoding, so take the dwarf version used in account. */
				if (cu.version == 5)
					r = dwarf5_rangelist_header(debug_rnglists, range_offset, (uint32_t *) (debug_addr + cu.addr_base_section_offset), cu.base_address);
				else if (cu.version < 5)
					r = pre_dwarf5_address_range(debug_ranges, range_offset, cu.base_address);
				else
					DwarfUtil::panic();
			}
			else if (range.form == DW_FORM_rnglistx)
			{
				if (cu.version != 5)
					DwarfUtil::panic();
				auto range_index = DwarfUtil::formConstant(range);
				r = dwarf5_rangelist_header(debug_rnglists + cu.rnglist_header_base, range_index, (uint32_t *) (debug_addr + cu.addr_base_section_offset), cu.base_address);
			}
			else if /* dwarf3 */ (range.form == DW_FORM_data4)
			{
				if (cu.version != 3)
					DwarfUtil::panic();
				auto range_offset = DwarfUtil::formConstant(range);
				r = pre_dwarf5_address_range(debug_ranges, range_offset, cu.base_address);
			}
			else
				DwarfUtil::panic();

		}
		else
		{
			auto low_pc = a.dataForAttribute(DW_AT_low_pc, debug_info + die.offset);
			if (low_pc.form)
			{
				uint32_t start_address;
				auto hi_pc = a.dataForAttribute(DW_AT_high_pc, debug_info + die.offset);
				if (!hi_pc.form)
				{
					/* Call site and label dies (DW_TAG_call_site and DW_TAG_label) may have a DW_AT_low_pc attribute,
					 * and should not have a DW_AT_high_pc attribute. */
					if (die.tag != DW_TAG_call_site && die.tag != DW_TAG_GNU_call_site && die.tag != DW_TAG_label)
						DwarfUtil::panic("Missing dwarf 'hi_pc' attribute for die; cannot build an address range");
				}
				else
				{
					start_address = DwarfUtil::fetchHighLowPC(low_pc, addrx_resolver);
					r = AddressRange(start_address, DwarfUtil::fetchHighLowPC(hi_pc, addrx_resolver, start_address));
				}
			}
		}
		return r;
	}

	void buildCompilationUnitFingerprints(void)
	{
		uint32_t cu_header_offset;
		for (cu_header_offset = 0; cu_header_offset != -1; cu_header_offset = next_compilation_unit(cu_header_offset))
		{
			auto ch = compilation_unit_header(this->debug_info + cu_header_offset);
			auto cu_die_offset = cu_header_offset + /* skip compilation unit header */ ch.header_length();
			CompilationUnitFingerprint f(debug_info, cu_header_offset, debug_abbrev + abbreviationOffsetForDieOffset(cu_header_offset + ch.header_length()), debug_str, debug_str_offsets, debug_addr);
			auto compilation_unit_die = debug_tree_of_die(cu_die_offset, 0, 1).at(0);
			compilationUnitFingerprints.push_back(f);
			compilationUnitFingerprints.back().range = getRangeOfDie(compilation_unit_die, getAddrxResolver(cu_die_offset));
		}
		/* Handle dwarf4 type units as a special case. Ugly... */
		uint32_t tu_header_offset = debug_info_len;
		while (tu_header_offset < debug_info_len + debug_types_len)
		{
			compilationUnitFingerprints.push_back(CompilationUnitFingerprint(debug_info, tu_header_offset));
			tu_header_offset += compilation_unit_header(this->debug_info + tu_header_offset).unit_length() + sizeof(uint32_t);

		}
	}
	
	struct
	{
		unsigned total_compilation_units;
		unsigned dies_read;
		unsigned compilation_unit_address_ranges_hits;
		unsigned compilation_unit_address_ranges_misses;
		unsigned compilation_unit_header_hits;
		unsigned compilation_unit_header_misses;
		unsigned abbreviation_hits;
		unsigned abbreviation_misses;
	}
	stats;
	struct DieFingerprint
	{
		/*! \todo	Maybe also add here a sibling pointer field, and maybe also stash the DIE tag value.
		 * 		This may eventually simpllify the dwarf DIE scanning code. */
		/* This is the offset of the DIE in the '.debug_info' section. */
		uint32_t	offset;
		/* This is the abbreviation offset for the DIE in the '.debug_abbrev' section. */
		uint32_t	abbrev_offset;
	};
	/* This can also be a std::map. */
	std::vector<struct DieFingerprint> die_fingerprints;
	int die_fingerprint_index(uint32_t die_offset)
	{
		int l = 0, h = die_fingerprints.size() - 1, m;
		while (l <= h)
		{
			m = (l + h) >> 1;
			if (die_fingerprints.at(m).offset == die_offset)
				return m;
			if (die_fingerprints.at(m).offset < die_offset)
				l = m + 1;
			else
				h = m - 1;
		}
		DwarfUtil::panic();
	}
	uint32_t abbreviationOffsetForDieOffset(uint32_t die_offset) { return die_fingerprints.at(die_fingerprint_index(die_offset)).abbrev_offset; }

	struct CompilationUnitFingerprint
	{
		AddressRange	range;
		/* The dwarf version of the compilation unit. */
		int		version;
		uint32_t	cu_header_offset;
		/* These are the offsets of the compilation unit's contribution to the '.debug_info' section.
		 * The start offset is the offset of the first byte past the compilation unit header - it is
		 * the offset of the compilation unit die in the '.debug_info' section. The
		 * end offset is the first byte past the last DIE data in the compilation unit. */
		uint32_t	debug_info_start_offset;
		uint32_t	debug_info_end_offset;
		/* The 'offset' fields below are applicable for DWARF5 and above. See the dwarf standard for details.
		 * Basically, these are used as index bases in other sections, and this allows reference to data from
		 * dies in the '.debug_info' section, to other dwarf sections ('.debug_str_offsets', '.debug_addr', '.debug_rnglist',
		 * and '.debug_loclist') by an 'indexing' indirection, instead of directly having section offsets
		 * (DW_FORM_sec_offset). This removes the need for relocating the section offset values by the linkage
		 * editor, and that is the main reason for having this form of indirection.
		 *
		 * A value of (-1) indicates that the respective base address is undefined. */
		uint32_t	str_offsets_base = -1;
		uint32_t	addr_base_section_offset = -1;
		/*! \todo	One of these fields is redundant, because either of them can be computed from the other.
		 * 		They are put here for convenience, but maybe eventually remove one of them. */
		uint32_t	rnglist_header_base = -1;
		uint32_t	rnglist_base = -1;

		uint32_t	loclist_base = -1;

		/* Compilation unit base address for range lists computations. */
		uint32_t	base_address = UNDEFINED_COMPILATION_UNIT_BASE_ADDRESS;

		uint32_t	tag = 0;
		const char	* compilation_directory_name = 0;
		uint32_t	statement_list_offset = -1;

		bool containsDieAtOffset(uint32_t die_offset) { return debug_info_start_offset <= die_offset && die_offset < debug_info_end_offset; }
		/* Special case constructor, to be used only for dwarf4 type units. */
		CompilationUnitFingerprint(const uint8_t * debug_info, uint32_t tu_header_offset)
		{
			compilation_unit_header h(debug_info + tu_header_offset);
			this->cu_header_offset = tu_header_offset;
			version = h.version();
			if (h.version() != 4)
				DwarfUtil::panic();
			debug_info_start_offset = tu_header_offset + /* Hard coded - special case for dwarf4 type units! */ 23;
			debug_info_end_offset = debug_info_start_offset + h.unit_length() - /* Hard coded - special case for dwarf4 type units! */ 23
			        + /* The unit_length field size itself is not included in the unit_length value, account for this. */sizeof(uint32_t);

		}
		CompilationUnitFingerprint(const uint8_t * debug_info, uint32_t cu_header_offset, const uint8_t * debug_abbrev_data_of_compilation_unit_die, const void * debug_str, const uint8_t * debug_str_offsets, const uint8_t * debug_addr)
		{
			compilation_unit_header h(debug_info + cu_header_offset);
			this->cu_header_offset = cu_header_offset;
			version = h.version();
			debug_info_start_offset = cu_header_offset + h.header_length();
			debug_info_end_offset = debug_info_start_offset + h.unit_length() - h.header_length()
				+ /* The unit_length field size itself is not included in the unit_length value, account for this. */sizeof(uint32_t);
			const uint8_t * debug_info_bytes = debug_info + debug_info_start_offset;

			struct Abbreviation a(debug_abbrev_data_of_compilation_unit_die);
			tag = a.tag();

			auto s = a.dataForAttribute(DW_AT_str_offsets_base, debug_info_bytes);
			if (s.form)
				str_offsets_base = DwarfUtil::formConstant(s);
			s = a.dataForAttribute(DW_AT_addr_base, debug_info_bytes);
			if (s.form)
				addr_base_section_offset = DwarfUtil::formConstant(s);
			s = a.dataForAttribute(DW_AT_rnglists_base, debug_info_bytes);
			if (s.form)
			{
				rnglist_base = DwarfUtil::formConstant(s);
				rnglist_header_base = rnglist_base - dwarf5_rangelist_header::header_length();
			}
			s = a.dataForAttribute(DW_AT_loclists_base, debug_info_bytes);
			if (s.form)
				loclist_base = DwarfUtil::formConstant(s);

			s = a.dataForAttribute(DW_AT_low_pc, debug_info_bytes);
			if (s.form)
				base_address = DwarfUtil::fetchHighLowPC(s,
				        [this, debug_addr](unsigned x) -> uint32_t {
					return ((uint32_t *)(debug_addr + addr_base_section_offset))[x];
				});

			s = a.dataForAttribute(DW_AT_stmt_list, debug_info_bytes);
			if (s.form)
				statement_list_offset = DwarfUtil::formConstant(s);

			s = a.dataForAttribute(DW_AT_comp_dir, debug_info_bytes);
			if (s.form)
				compilation_directory_name = DwarfUtil::formString(s, (const uint8_t *) debug_str,
				           [this, debug_str_offsets, debug_str](unsigned x) -> const char * {
					return (const char *)(debug_str + ((uint32_t *)(debug_str_offsets + str_offsets_base))[x]);
				});
		}
	};
	std::vector<CompilationUnitFingerprint> compilationUnitFingerprints;
	/* The last searched compilation unit fingerprint is cached with the intention to speed up subsequent searches. */
	int lastSearchedCUFingerprintIndex = -1;
	int cuFingerprintIndexForAddress(uint32_t address)
	{
		if (lastSearchedCUFingerprintIndex != -1 && compilationUnitFingerprints.at(lastSearchedCUFingerprintIndex).range.isAddressInRange(address))
		{
			if (STATS_ENABLED) stats.compilation_unit_header_hits ++;
			return lastSearchedCUFingerprintIndex;
		}
		if (STATS_ENABLED) stats.compilation_unit_header_misses ++;
		for (lastSearchedCUFingerprintIndex = 0; lastSearchedCUFingerprintIndex < compilationUnitFingerprints.size(); lastSearchedCUFingerprintIndex ++)
			if (compilationUnitFingerprints.at(lastSearchedCUFingerprintIndex).range.isAddressInRange(address))
				return lastSearchedCUFingerprintIndex;
		return lastSearchedCUFingerprintIndex = -1;
	}
	int cuFingerprintIndexForDieOffset(uint32_t die_offset)
	{
		int m = lastSearchedCUFingerprintIndex;
		if (m != -1 && compilationUnitFingerprints.at(m).containsDieAtOffset(die_offset))
		{
			if (STATS_ENABLED) stats.compilation_unit_header_hits ++;
			return lastSearchedCUFingerprintIndex;
		}
		if (STATS_ENABLED) stats.compilation_unit_header_misses ++;
		int l = 0, h = compilationUnitFingerprints.size() - 1;
		while (l <= h)
		{
			m = (l + h) >> 1;
			if (compilationUnitFingerprints.at(m).containsDieAtOffset(die_offset))
				return lastSearchedCUFingerprintIndex = m;
			if (die_offset < compilationUnitFingerprints.at(m).debug_info_start_offset)
				h = m - 1;
			else
				l = m + 1;
		}
		return lastSearchedCUFingerprintIndex = -1;
	}
	/*! \todo	This really shouldn't be public, it is only needed for some dwarf opcodes. Fix this. */
public:
	uint32_t cuHeaderOffsetForOffsetInDebugInfo(uint32_t debug_info_offset)
	{
		int i = cuFingerprintIndexForDieOffset(debug_info_offset);
		if (i != -1)
			return compilationUnitFingerprints.at(i).cu_header_offset;
		else
			return -1;
	}
private:
	/* first number is the abbreviation code, the second is the offset in .debug_abbrev */
	void getAbbreviationsOfCompilationUnit(uint32_t abbreviation_offset, std::map<uint32_t, uint32_t> & abbreviations)
	{
		if (STATS_ENABLED) stats.abbreviation_misses ++;
		const uint8_t * debug_abbrev = this->debug_abbrev + abbreviation_offset;
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
		const uint8_t * p = debug_info + die_offset;
		int len;
		uint32_t code = DwarfUtil::uleb128(p, & len);
		p += len;

		/*! \note	some compilers (e.g. IAR) generate abbreviations in .debug_abbrev which specify that a die has
		 * children, while it actually does not - such a die actually contains a single null die child,
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
		/* Also, save null dies. */
		die_fingerprints.push_back((struct DieFingerprint) { .offset = die_offset, .abbrev_offset = 0});
		die_offset = p - debug_info;
	}
public:
	DwarfData(const void * debug_info, uint32_t debug_info_len,
		  const void * debug_abbrev, uint32_t debug_abbrev_len,
		  const void * debug_ranges, uint32_t debug_ranges_len,
		  const void * debug_str, uint32_t debug_str_len,
		  const void * debug_line, uint32_t debug_line_len,
		  const void * debug_loc, uint32_t debug_loc_len,
		  const void * debug_types, uint32_t debug_types_len,
		  const void * debug_rnglists, uint32_t debug_rnglists_len,
		  const void * debug_loclists, uint32_t debug_loclists_len,
		  const void * debug_addr, uint32_t debug_addr_len,
		  const void * debug_str_offsets, uint32_t debug_str_offsets_len
		  )
	{
		/* Warning: some HACKS are employed here for the case of DWARF4, where two separate
		 * debug sections - '.debug_info' and '.debug_types' are present! These sections are
		 * internally unified, because DWARF debug information is easier to handle that way.
		 * Also, see the comments about 'debug_info_bytes'. */
		this->debug_info_len = debug_info_len;
		this->debug_types_len = debug_types_len;
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

		/* These sections are first introduced in dwarf5 */
		this->debug_rnglists = (const uint8_t *) debug_rnglists;
		this->debug_rnglists_len = debug_rnglists_len;
		this->debug_loclists = (const uint8_t *) debug_loclists;
		this->debug_loclists_len = debug_loclists_len;
		this->debug_addr = (const uint8_t *) debug_addr;
		this->debug_addr_len = debug_addr_len;
		this->debug_str_offsets = (const uint8_t *) debug_str_offsets;
		this->debug_str_offsets_len = debug_str_offsets_len;

		debug_info_bytes = std::vector<uint8_t>(debug_info_len + debug_abbrev_len);
		memcpy(debug_info_bytes.data(), debug_info, debug_info_len);
		/* In case of a DWARF4 '.debug_types' section - append the section contents to '.debug_info'. */
		memcpy(debug_info_bytes.data() + debug_info_len, debug_types, debug_types_len);

		this->debug_info = debug_info_bytes.data();

		memset(& stats, 0, sizeof stats);
		
		std::map<uint32_t, uint32_t> abbreviations;
		uint32_t dwarf_unit_offset;
		for (dwarf_unit_offset = 0; dwarf_unit_offset != -1; dwarf_unit_offset = next_compilation_unit(dwarf_unit_offset))
		{
			compilation_unit_header c(this->debug_info + dwarf_unit_offset);
			auto die_offset = dwarf_unit_offset + /* Skip unit header. */ c.header_length();
			stats.total_compilation_units ++;
			getAbbreviationsOfCompilationUnit(c.debug_abbrev_offset(), abbreviations);
			reapDieFingerprints(die_offset, abbreviations);
		}
		/* If present, also reap the DIEs of a DWARF4 '.debug_types' section. */
		dwarf_unit_offset = debug_info_len;
		while (dwarf_unit_offset < debug_info_len + debug_types_len)
		{
			compilation_unit_header tu(this->debug_info + dwarf_unit_offset);
			typeSignatureMap[tu.type_unit_type_signature()] = tu.data + tu.type_unit_type_offset() - this->debug_info;
			auto die_offset = dwarf_unit_offset + /* Skip unit header. */ tu.dwarf4_type_unit_header_length();
			getAbbreviationsOfCompilationUnit(tu.debug_abbrev_offset(), abbreviations);
			reapDieFingerprints(die_offset, abbreviations);
			dwarf_unit_offset = tu.next().data - this->debug_info;
		}

		buildCompilationUnitFingerprints();
	}
	void dumpStats(void)
	{
		qDebug() << "total dies in .debug_info:" << die_fingerprints.size();
		qDebug() << "total compilation units in .debug_info:" << stats.total_compilation_units;
		qDebug() << "total dies read:" << stats.dies_read;
		qDebug() << "compilation unit address range search hits:" << stats.compilation_unit_address_ranges_hits;
		qDebug() << "compilation unit address range search misses:" << stats.compilation_unit_address_ranges_misses;
		qDebug() << "compilation unit die search hits:" << stats.compilation_unit_header_hits;
		qDebug() << "compilation unit die search misses:" << stats.compilation_unit_header_misses;
		qDebug() << "abbreviation fetch hits:" << stats.abbreviation_hits;
		qDebug() << "abbreviation fetch misses:" << stats.abbreviation_misses;
	}

private:

	/*! \todo	the name of this function is misleading, it really reads a sequence of dies on a same die tree level; that
	 * 		is because of the dwarf die flattenned tree representation */
	/*! \todo	This is an essential core function, it can be made more efficient by utilizing the dwarf die fingerprint array. */
	std::vector<struct Die> debug_tree_of_die(uint32_t & die_offset, int depth = 0, int max_depth = -1)
	{
		if (STATS_ENABLED) stats.dies_read ++;
		std::vector<struct Die> dies;
		if (DEBUG_DIE_READ_ENABLED) qDebug() << "at offset " << QString("$%1").arg(die_offset, 0, 16);
		uint32_t code = DwarfUtil::uleb128(debug_info + die_offset);
		/*! \note	some compilers (e.g. IAR) generate abbreviations in .debug_abbrev which specify that a die has
		 * children, while it actually does not - such a die actually considers a single null die child,
		 * which is explicitly permitted by the dwarf standard. Handle this at the condition check at the start of the loop */
		while (code)
		{
			auto i = die_fingerprint_index(die_offset);
			uint32_t abbrev_offset = die_fingerprints.at(i).abbrev_offset;
			struct Abbreviation a(debug_abbrev + abbrev_offset);
			struct Die die(a.tag(), die_offset, abbrev_offset);
			
			die_offset = die_fingerprints.at(i + 1).offset;
			if (a.has_children())
			{
				if (depth + 1 == max_depth)
				{
					if (/* special case for reading a single die */ depth == 0)
						goto out;
					auto x = a.dataForAttribute(DW_AT_sibling, debug_info + die.offset);
					if (x.form)
					{
						die_offset = DwarfUtil::formReference(x.form, x.debug_info_bytes, cuHeaderOffsetForOffsetInDebugInfo(die.offset));
						goto out;
					}
				}
				die.children = debug_tree_of_die(die_offset, depth + 1, max_depth);
			}
out:
			dies.push_back(die);
			
			if (depth == 0)
				return dies;
			code = DwarfUtil::uleb128(debug_info + die_offset);
			if (DEBUG_DIE_READ_ENABLED) qDebug() << "at offset " << QString("$%1").arg(die_offset, 0, 16);
		}
		/* Skip null die. */
		die_offset ++;

		return dies;
	}
public:
	struct Die dieForDieOffset(uint32_t die_offset) { return debug_tree_of_die(die_offset, 0, 1).at(0); }
	uint32_t next_compilation_unit(uint32_t compilation_unit_offset)
	{
		uint32_t x = compilation_unit_header(debug_info + compilation_unit_offset).next().data - debug_info;
		if (x >= debug_info_len)
			x = -1;
		return x;
	}
	int compilation_unit_count(void) const { return compilationUnitFingerprints.size(); }
	std::vector<struct Die> executionContextForAddress(uint32_t address)
	{
		std::vector<struct Die> context;
		int cu_index = cuFingerprintIndexForAddress(address);
		if (cu_index == -1)
			return context;
		qDebug() << "cu index:" << cu_index;
		uint32_t die_offset = compilationUnitFingerprints.at(cu_index).debug_info_start_offset;
		auto compilation_unit_die = debug_tree_of_die(die_offset, 0, 1);
		int i(0);
		std::vector<struct Die> * die_list(& compilation_unit_die);
		auto addrx_resolver = getAddrxResolver(compilation_unit_die.at(0).offset);
		while (i < die_list->size())
			if (getRangeOfDie(die_list->at(i), addrx_resolver).isAddressInRange(address))
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
		auto x = executionContextForAddress(address - 1);
		if (!x.size())
			return false;
		auto d = x.back().children.begin();
		auto addrx_resolver = getAddrxResolver(x.at(0).offset);
		while (d != x.back().children.end())
		{
			/*! \todo DW_TAG_call_site is officially a part of DWARF5, and is somewhat different than
			 * the pre-dwarf5 gcc extensions using DW_TAG_GNU_call_site and related tags, e.g.,
			 * in dwarf5 there is now a DW_AT_call_return_pc instead of the DW_AT_low_pc that gcc
			 * uses instead prior to dwarf5. Code the new dwarf5 representation, but also keep the
			 * pre-dwarf5 code, */
			if ((* d).tag == DW_TAG_GNU_call_site)
			{
				Abbreviation a(debug_abbrev + d->abbrev_offset);
				auto l = a.dataForAttribute(DW_AT_low_pc, debug_info + d->offset);
				if (l.form && DwarfUtil::fetchHighLowPC(l, addrx_resolver) == address)
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
		for (i = context.size() - 1; i >= 0 && frame_base.empty(); frame_base = locationSforthCode(context.at(i --), program_counter, DW_AT_frame_base));
		qDebug() << QString::fromStdString(frame_base);
		return frame_base;
	}
	struct SourceCodeCoordinates sourceCodeCoordinatesForDieOffset(uint32_t die_offset)
	{
		SourceCodeCoordinates s;
		auto die = dieForDieOffset(die_offset);
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
		auto cu_offset = cuHeaderOffsetForOffsetInDebugInfo(die.offset);
		cu_offset += /* skip compilation unit header */ compilation_unit_header(debug_info + cu_offset).header_length();

		auto compilation_unit_die = dieForDieOffset(cu_offset);
		Abbreviation b(debug_abbrev + compilation_unit_die.abbrev_offset);
		auto statement_list = b.dataForAttribute(DW_AT_stmt_list, debug_info + compilation_unit_die.offset);
		auto compilation_directory = b.dataForAttribute(DW_AT_comp_dir, debug_info + compilation_unit_die.offset);
		if (!statement_list.form)
			return s;
		DebugLine l(debug_line, debug_line_len);
		static const char * compilation_directory_string = "<<< unknown compilation directory >>>";
		if (compilation_directory.form)
			compilation_directory_string = DwarfUtil::formString(compilation_directory, debug_str, getStrxResolver(die_offset));
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
		int cu_index = cuFingerprintIndexForAddress(address);
		if (cu_index == -1)
			return s;
		const CompilationUnitFingerprint & cu(compilationUnitFingerprints.at(cu_index));

		uint32_t file_number;
		if (cu.tag != DW_TAG_compile_unit)
			DwarfUtil::panic();
		if (cu.statement_list_offset == -1)
			return s;
		class DebugLine l(debug_line, debug_line_len);
		bool dummy;
		s.line = l.lineNumberForAddress(address, cu.statement_list_offset, file_number, is_address_on_exact_line_number_boundary ? * is_address_on_exact_line_number_boundary : dummy);
		if (cu.compilation_directory_name)
			s.compilation_directory_name = cu.compilation_directory_name;
		l.stringsForFileNumber(file_number, s.file_name, s.directory_name, s.compilation_directory_name);
		return s;
	}

	struct TypePrintFlags
	{
		bool	verbose_printing		: 1;
		bool	discard_typedefs		: 1;
		TypePrintFlags(void)	{ verbose_printing = discard_typedefs = false; }
	};

private:
	/*! \todo	!!! This must be merged to 'formReference()' !!! */
	uint32_t readTypeOffset(uint32_t attribute_form, const uint8_t * debug_info_bytes, uint32_t compilation_unit_header_offset)
	{
		switch (attribute_form)
		{
		case DW_FORM_addr:
		case DW_FORM_data4:
		case DW_FORM_sec_offset:
			return * (uint32_t *) debug_info_bytes;
		/*! \todo handle all reference forms here, including signatures */
		case DW_FORM_ref2:
			return * (uint16_t *) debug_info_bytes + compilation_unit_header_offset;
		case DW_FORM_ref4:
			return * (uint32_t *) debug_info_bytes + compilation_unit_header_offset;
		case DW_FORM_ref_udata:
			return DwarfUtil::uleb128(debug_info_bytes) + compilation_unit_header_offset;
		case DW_FORM_ref_addr:
			return * (uint32_t *) debug_info_bytes;
		case DW_FORM_indirect:
		{
			int form, bytes_to_skip;
			return form = DwarfUtil::uleb128(debug_info_bytes, & bytes_to_skip), readTypeOffset(form, debug_info_bytes + bytes_to_skip, compilation_unit_header_offset);
		}
		case DW_FORM_ref_sig8:
			return typeSignatureMap.at(* (uint64_t *) debug_info_bytes);
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
		auto i = cuHeaderOffsetForOffsetInDebugInfo(die.offset);
		auto referred_die_offset = DwarfUtil::formReference(x.form, x.debug_info_bytes, i);
		referred_die = dieForDieOffset(referred_die_offset);
		return true;
	}
std::map</* die offset */ uint32_t, /* type cache index */ uint32_t> recursion_detector;

	int verbose_type_printing_indentation_level = 0;
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
					type_string += "{\n";
					verbose_type_printing_indentation_level ++;
					for (const auto& member : type.at(node_number).children)
					{
						if (type.at(member).die.tag != DW_TAG_member && type.at(member).die.tag != DW_TAG_enumerator)
							/* Some compilers (e.g. the ARM compiler) are known
							 * to generate dies for structure types, that have
							 * child dies which are not of type 'DW_TAG_member'.
							 * Skip such entries */
							continue;
						type_string += std::string(verbose_type_printing_indentation_level, '\t') + typeString(type, member, flags, false) + ((die.tag == DW_TAG_enumeration_type) ? "," : ";") + "\n";
					}
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
				QString name = nameOfDie(die, true);
				if (x.form == 0 || size.form == 0)
					DwarfUtil::panic();
				if (name.isEmpty()) switch (DwarfUtil::formConstant(x))
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
				else
					type_string += (name + ' ').toStdString();
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
							{
								if (DwarfUtil::isClassConstant(subrange.form))
									type_string += "[!" + std::to_string(DwarfUtil::formConstant(subrange) + 1) + "]";
								else
									type_string += "[!variable length array]";
							}
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
			case DW_TAG_unspecified_type:
				/* The ARM compiler shipped with Keil generates such tags for 'void' types */
				if (is_prefix_printed)
					type_string += nameOfDie(die, true), type_string += " ";
				break;
			default:
				qDebug() << "unhandled tag" <<  type.at(node_number).die.tag << "in" << __func__;
				qDebug() << "die offset" << type.at(node_number).die.offset;
				DwarfUtil::panic();
		}
		return type_string;
	}

public:
	std::string arrayUpperBoundSforthCode(uint32_t array_subrange_die_offset, uint32_t address_for_location)
	{
		Die subrange_die = dieForDieOffset(array_subrange_die_offset);
		struct Abbreviation a(debug_abbrev + subrange_die.abbrev_offset);
		auto upper_bound = a.dataForAttribute(DW_AT_upper_bound, debug_info + subrange_die.offset);
		if (!upper_bound.form)
			DwarfUtil::panic();
		if (DwarfUtil::isClassReference(upper_bound.form))
		{
			/* Currently, only references to (artificial) variables, containing the
			 * array upper bound, are supported. The gcc compiler generates such references. */
			uint32_t cu_offset = cuHeaderOffsetForOffsetInDebugInfo(subrange_die.offset);
			uint32_t r = DwarfUtil::formReference(upper_bound.form, upper_bound.debug_info_bytes, cu_offset);
			Die t = dieForDieOffset(r);
			if (t.tag != DW_TAG_variable)
				DwarfUtil::panic();
			return locationSforthCode(t, address_for_location);
		}
		else
			DwarfUtil::panic();
	}
public:
int readType(uint32_t die_offset, std::vector<struct DwarfTypeNode> & type_cache, bool reset_recursion_detector = true);
DwarfBaseType readBaseOrGenericType(uint32_t /* If the die offset is zero, return the 'generic' type */ die_offset);
bool isPointerType(const std::vector<struct DwarfTypeNode> & type, int node_number = 0);
bool isArrayType(const std::vector<struct DwarfTypeNode> & type, int node_number = 0);
bool isSubroutineType(const std::vector<struct DwarfTypeNode> & type, int node_number = 0);

/* Checks if the referred type node is a dwarf base type node, and if so - returns its dwarf encoding.
 * If this is not a base type node, return -1 */
int baseTypeEncoding(const std::vector<struct DwarfTypeNode> & type, int node_number = 0);

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
			/*! \todo	Properly handle variable length arrays here! */
			//for (i = 0; i < type.at(node_number).array_dimensions.size(); n *= type.at(node_number).array_dimensions.at(i ++).constant_value);
			qDebug() << "xxx: processing array";
			for (i = 0; i < type.at(node_number).array_dimensions.size(); i++)
			{
				qDebug() << "array dimension size:" << type.at(node_number).array_dimensions.at(i).constant_value;
				n *= type.at(node_number).array_dimensions.at(i).constant_value;
			}
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
		/* Special case - in case of a variable length array, the 'bytesize' field
		 * will be deliberately set to zero. That is because the actual dimensions
		 * of a variable size array usually need runtime target context information
		 * to be computed properly. It is responsibility of the consumer to check
		 * this, and to compute the actual dimension sizes of a variable length
		 * array by using the 'array_dimensions' field below. */
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
		std::vector<struct DwarfTypeNode::array_dimension> array_dimensions;
		std::vector<struct DataNode> children;
	};
	void dataForType(std::vector<struct DwarfTypeNode> & type, struct DataNode & node, int type_node_number = 0, struct TypePrintFlags flags = TypePrintFlags())
	{
		if (type_node_number == -1)
		{
			QMessageBox::critical(0, "DWARF parser internal error",
				    "Internal error when parsing DWARF type information:\n"
				    "invalid type node index\n\n"
				    "Please! Report this error");
			return;
		}

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
					case DW_FORM_exprloc:
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
		{
				node.enumeration_die = die;
				node.is_enumeration = 1;
				int enum_type_index = type.at(type_node_number).next;
				/* Gather more details about the enumeration encoding and bytesize.
				 * First, take in account a (base) type attribute (DW_AT_type), if present,
				 * and then let any explicit encoding and/or bytesize (if present) to
				 * override the values gathered from the (base) type for the enumeration (if any). */
				if (enum_type_index != -1)
				{
					node.base_type_encoding = baseTypeEncoding(type, enum_type_index);
					node.base_type_encoding = sizeOf(type, enum_type_index);
				}
				auto x = a.dataForAttribute(DW_AT_encoding, debug_info + die.offset);
				if (x.form)
					node.base_type_encoding = DwarfUtil::formConstant(x);
				x = a.dataForAttribute(DW_AT_byte_size, debug_info + die.offset);
				if (x.form)
					node.bytesize = DwarfUtil::formConstant(x);
		}
				break;
			case DW_TAG_array_type:
				if (type.at(type_node_number).array_dimensions.size())
					node.array_dimensions = type.at(type_node_number).array_dimensions;
				node.children.push_back(DataNode()), dataForType(type, node.children.back(), type.at(type_node_number).next, flags);
				break;
			case DW_TAG_template_type_parameter:
			case DW_TAG_template_value_parameter:
			case DW_TAG_subprogram:
			case DW_TAG_subroutine_type:
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
				return DwarfUtil::formString(x, debug_str, getStrxResolver(enumeration_die_offset));
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
		return DwarfUtil::formString(x, debug_str, getStrxResolver(die.offset));
	}
	void dumpLines(void)
	{
		class DebugLine l(debug_line, debug_line_len);
		l.dump();
		while (l.next())
			l.dump();
	}
	/* the returned vector is sorted by increasing start address */
	void suggestedBreakpointLocationsForFile(const char * filename, std::vector<struct DebugLine::lineAddress> & line_addresses)
	{
		class DebugLine l(debug_line, debug_line_len);
		do
		{
			auto x(l.fileNumber(filename));
			if (x != -1)
				l.addressesForFile(x, line_addresses);
		}
		while (l.next());
		int i;
		for (i = 0; i < line_addresses.size();)
			/*
			if (line_addresses.at(i).address == line_addresses.at(i).address_span)
				line_addresses.erase(line_addresses.begin() + i);
			else
			//*/
			if (!line_addresses.at(i).is_stmt)
				line_addresses.erase(line_addresses.begin() + i);
			else
				i ++;
		std::sort(line_addresses.begin(), line_addresses.end());
	}
	/* the returned vector is sorted by increasing start address */
	void disassemblyAddressRangesForFile(const char * filename, std::vector<struct DebugLine::lineAddress> & line_addresses)
	{
		class DebugLine l(debug_line, debug_line_len);
		do
		{
			auto x(l.fileNumber(filename));
			if (x != -1)
				l.addressesForFile(x, line_addresses);
		}
		while (l.next());
		int i;
		for (i = 0; i < line_addresses.size();)
			//*
			if (line_addresses.at(i).address == line_addresses.at(i).address_span)
				line_addresses.erase(line_addresses.begin() + i);
			else
			/*/
			if (!line_addresses.at(i).is_stmt)
				line_addresses.erase(line_addresses.begin() + i);
			else
			//*/
				i ++;
		std::sort(line_addresses.begin(), line_addresses.end());
	}

	std::vector<uint32_t> unfilteredAddressesForFileAndLineNumber(const char * filename, int line_number)
	{
		std::vector<uint32_t> addresses;
		std::vector<struct DebugLine::lineAddress> line_addresses;
		suggestedBreakpointLocationsForFile(filename, line_addresses);
		int i;
		for (i = 0; i < line_addresses.size(); i ++)
			if (line_addresses.at(i).line == line_number/* && line_addresses.at(i).address_span - line_addresses.at(i).address*/)
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
			{
				contexts.operator [](x.back().offset) = 1, filtered_addresses.push_back(addresses.at(i));
				qDebug() << "Adding filtered address" << QString("$%1").arg(addresses.at(i), 8, 16, QChar('0')) << "die:" << QString("$%1").arg(x.back().offset, 8, 16, QChar('0'));
			}
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
				/* The ARM compiler in the Keil installation is known to generate compilation
				 * unit dies without source code statement list information */
				continue;
			l.skipToOffset(DwarfUtil::formConstant(x));
			x = a.dataForAttribute(DW_AT_name, debug_info + cu_die_offset);
			auto strx_resolver = getStrxResolver(cu_die_offset);
			if (x.form)
				filename = DwarfUtil::formString(x, debug_str, strx_resolver);
			x = a.dataForAttribute(DW_AT_comp_dir, debug_info + cu_die_offset);
			if (x.form)
				compilation_directory = DwarfUtil::formString(x, debug_str, strx_resolver);
			/*! \todo	is this line below necessary??? */
			//sources.push_back((struct DebugLine::sourceFileNames) { .file = filename, .directory = compilation_directory, .compilation_directory = compilation_directory, });
			/* Some compilers (e.g. the ARM compiler) do not bother emitting any entries in the
			 * line number information directory table, so the directory strings may be null ponters.
			 * Use an ampty string in such a case, in order to simplify processing */
			l.getFileAndDirectoryNamesPointers(sources, compilation_directory ? compilation_directory : "");
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
			if (x.form && DwarfUtil::isLocationConstant(x, address))
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
	QByteArray constantValueSforthCode(const struct Die & die)
	{
		Abbreviation a(debug_abbrev + die.abbrev_offset);
		auto x(a.dataForAttribute(DW_AT_const_value, debug_info + die.offset));
		if (!x.form)
			return "";
		qDebug() << "processing die offset" << die.offset;
		return DwarfUtil::data_block_for_constant_value(x);
	}
	std::string locationSforthCode(const struct Die & die, uint32_t address_for_location = -1, uint32_t location_attribute = DW_AT_location)
	{
		Abbreviation a(debug_abbrev + die.abbrev_offset);
		auto x(a.dataForAttribute(location_attribute, debug_info + die.offset));
		if (!x.form)
			return "";
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
				return DwarfExpression::sforthCode(x.debug_info_bytes, len, cuHeaderOffsetForOffsetInDebugInfo(die.offset));
			}
			case DW_FORM_data4:
			case DW_FORM_sec_offset:
			qDebug() << "location list offset:" << * (uint32_t *) x.debug_info_bytes;
				auto l = LocationList::locationExpressionForAddress(debug_loc, * (uint32_t *) x.debug_info_bytes,
					compilationUnitFingerprints.at(cuFingerprintIndexForDieOffset(die.offset)).base_address, address_for_location);
				return l ? DwarfExpression::sforthCode(l + 2, * (uint16_t *) l, cuHeaderOffsetForOffsetInDebugInfo(die.offset)) : "";
				
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
			static const int tested_expression_attributes[] = { DW_AT_location, DW_AT_GNU_call_site_value, };
			for (const auto& tested_attribute : tested_expression_attributes)
			{
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
					DwarfExpression::sforthCode(x.debug_info_bytes, len, cuHeaderOffsetForOffsetInDebugInfo(die_fingerprints[i].offset));
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
							DwarfExpression::sforthCode((uint8_t *) p + 2, * (uint16_t *) p, cuHeaderOffsetForOffsetInDebugInfo(die_fingerprints[i].offset)), p = (uint32_t *)((uint8_t *) p + * (uint16_t *) p + 2), test_count ++;
					}
					break;
				}
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
		const uint8_t * data, * debug_frame;
		uint32_t debug_frame_len;

		bool isFDE(void) { return * (uint32_t *) (data + 4) != 0xffffffff; }
		bool isCIE(void) { return * (uint32_t *) (data + 4) == 0xffffffff; }
		/* The length field is common for CIEs and FDEs */
		uint32_t length(void) { return * (uint32_t *) data; }

		/*
		 * FDE fields
		 */
		uint32_t CIE_pointer(void) { return * (uint32_t *) (data + 4); }
		uint32_t initial_location(void) { return * (uint32_t *) (data + 8); }
		uint32_t address_range(void) { return * (uint32_t *) (data + 12); }
		/* pointer to the initial instructions, and bytecount in instruction block */
		std::pair<const uint8_t *, int> fde_instructions(void) { return std::pair<const uint8_t *, int>(data + 16, length() - 12); }
		
		/*
		 * CIE fields
		 */
		/* IMPORTANT: Note that for dwarf 2, the version number is 1, for dwarf 3, it is 3 - there
		 * is no documented version number 2 that I am aware of */
		uint8_t version(void) { if (!isCIE()) DwarfUtil::panic(); return data[8]; }
		const char * augmentation = 0;
		uint32_t code_alignment_factor = -1;
		int32_t data_alignment_factor = -1;
		/* In dwarf 2, this field is an unsigned byte */
		uint32_t return_address_register = -1;
		/* pointer to the initial instructions, and bytecount in instruction block */
		std::pair<const uint8_t *, int> cie_initial_instructions;
		std::pair<const uint8_t *, int> instructions(void) { return isCIE() ? cie_initial_instructions : fde_instructions(); }

		CIEFDE(const uint8_t * debug_frame, uint32_t debug_frame_len, uint32_t debug_frame_offset)
		{
			this->debug_frame = debug_frame;
			this->debug_frame_len = debug_frame_len;
			data = debug_frame + debug_frame_offset;
			augmentation = (const char *) data + /* Skip the fields before the augmentation string */ 4 + 4 + 1;

			if (isCIE())
				if (version() > 4 || version() == /* No version 2 documented, that I am aware of */ 2 || version() == 0)
				DwarfUtil::panic("unsupported .debug_frame version");
			if (isCIE() && strlen(augmentation)
					&&	/* Special case for the ARM compiler. To cut a long story short, the augmentation
						 * string "armcc+" means that the dwarf unwind information has the standard-described
						 * behavior. See this if you are interested in more details:
						 * https://sourceware.org/ml/gdb-patches/2006-12/msg00249.html
						 */ strcmp(augmentation, "armcc+"))
				DwarfUtil::panic("unsupported .debug_frame CIE augmentation");
			if (isCIE())
			{
				int offset = /* Skip initial CIE fields */ 4 + 4 + 1 + strlen(augmentation) + /* Skip the null byte terminator of the augmentation string */ 1;
				int len;
				if (version() == 4)
					/* Account for the 'address_size' and 'segment_size' fields
					 * introduced in dwarf 4 */
					offset += 2;
				code_alignment_factor = DwarfUtil::uleb128(data + offset, & len);
				offset += len;
				data_alignment_factor = DwarfUtil::sleb128(data + offset, & len);
				offset += len;
				if (version() == 1)
					return_address_register = data[offset ++];
				else
					return_address_register = DwarfUtil::uleb128(data + offset, & len), offset += len;
				cie_initial_instructions = std::pair<const uint8_t *, int>(data + offset, length() + sizeof length() - offset);
			}
		}
		/* pointer to the initial instructions, and bytecount in instruction block */
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
				if (UNWIND_DEBUG_ENABLED) qDebug() << "code alignment factor " << code_alignment_factor;
				if (UNWIND_DEBUG_ENABLED) qDebug() << "data alignment factor " << data_alignment_factor;
				if (UNWIND_DEBUG_ENABLED) qDebug() << "return address register " << return_address_register;
				auto insn = cie_initial_instructions;
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
				<< cie.code_alignment_factor << " to code-alignment-factor "
				<< cie.data_alignment_factor << " to data-alignment-factor "
				<< (cie.ciefde_sforth_code() + " initial-cie-instructions-defined " + fde.ciefde_sforth_code()) << " unwinding-rules-defined ";
		return std::pair<std::string, uint32_t>(sfcode.str(), fde.initial_location());
	}
};

#endif /* __LIBTROLL_HXX__ */
