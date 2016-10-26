#include <dwarf.h>
#include <stdint.h>
#include <map>
#include <vector>
#include <sstream>
#include <QDebug>

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
		int len;
		a.first = DwarfUtil::uleb128(p, & len);
		p += len;
		a.second = DwarfUtil::uleb128(p, & len);
		p += len;
		if (a.first || a.second)
			/* not the last attribute - skip to the next one */
			s.attributes = p;
		return a;
	}

	Abbreviation(const uint8_t * abbreviation_data)
	{
		int len;
		s.code = DwarfUtil::uleb128(abbreviation_data, & len);
		abbreviation_data += len;
		s.tag = DwarfUtil::uleb128(abbreviation_data, & len);
		abbreviation_data += len;
		s.has_children = (DwarfUtil::uleb128(abbreviation_data, & len) == DW_CHILDREN_yes);
		s.init_attributes = s.attributes = abbreviation_data + len;
	}
	/* the first number is the attribute form, the pointer is to the data in .debug_info for the attribute searched
	 * returns <0, 0> if the attribute with the searched name is not found */
	std::pair<uint32_t, const uint8_t *> dataForAttribute(uint32_t attribute_name, const uint8_t * debug_info_data_for_die)
	{
		s.attributes = s.init_attributes;
		std::pair<uint32_t, uint32_t> a;
		int len;
		/* skip the die abbreviation code */
		debug_info_data_for_die += (DwarfUtil::uleb128(debug_info_data_for_die, & len), len);
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
public:
	DwarfData(const void * debug_aranges, uint32_t debug_aranges_len, const void * debug_info, uint32_t debug_info_len,
		  const void * debug_abbrev, uint32_t debug_abbrev_len, const void * debug_ranges, uint32_t debug_ranges_len,
		  const void * debug_str, uint32_t debug_str_len)
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
	std::vector<struct Die> debug_tree_of_die(uint32_t & die_offset, std::map<uint32_t, uint32_t> & abbreviations)
	{
		std::vector<struct Die> dies;
		const uint8_t * p = debug_info + die_offset;
		int len;
		uint32_t code = DwarfUtil::uleb128(p, & len);
		if (0) qDebug() << "at offset " << QString("$%1").arg(p - debug_info, 0, 16);
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
				die.children = debug_tree_of_die(die_offset, abbreviations);
				p = debug_info + die_offset;
			}
			dies.push_back(die);
			
			if (a.tag() == DW_TAG_compile_unit)
				return dies;
				
			code = DwarfUtil::uleb128(p, & len);
			if (0) qDebug() << "at offset " << QString("$%1").arg(p - debug_info, 0, 16);
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
			if (0) qDebug() << (x = DwarfUtil::fetchHighLowPC(low_pc.first, low_pc.second));
			if (0) qDebug() << DwarfUtil::fetchHighLowPC(hi_pc.first, hi_pc.second, x);
			x = DwarfUtil::fetchHighLowPC(low_pc.first, low_pc.second);
			return x <= address && address < DwarfUtil::fetchHighLowPC(hi_pc.first, hi_pc.second, x);
		}
		return false;
	}
	std::vector<struct Die> executionContextForAddress(uint32_t address)
	{
		std::vector<struct Die> context;
		auto cu_die_offset = get_compilation_unit_debug_info_offset_for_address(address);
		auto abbreviations = abbreviations_of_compilation_unit(cu_die_offset);
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
	std::string nameOfDie(const struct Die & die)
	{
		struct Abbreviation a(debug_abbrev + die.abbrev_offset);
		auto x = a.dataForAttribute(DW_AT_name, debug_info + die.offset);
		if (!x.first)
			return "<<< no name >>>";
		return DwarfUtil::formString(x.first, x.second, debug_str);
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
			qDebug() << (isCIE() ? "CIE" : "FDE");
			if (isCIE())
			{
				qDebug() << "version " << version();
				qDebug() << "code alignment factor " << code_alignment_factor();
				qDebug() << "data alignment factor " << data_alignment_factor();
				qDebug() << "return address register " << return_address_register();
				auto insn = initial_instructions();
				//qDebug() << "initial instructions " << QByteArray((const char *) insn.first, insn.second).toHex() << "count" << insn.second;
				qDebug() << "initial instructions " << QString().fromStdString(ciefde_sforth_code());
			}
			else
			{
				qDebug() << "initial address " << QString("$%1").arg(initial_location(), 0, 16);
				qDebug() << "range " << address_range();
				qDebug() << "instructions " << QString().fromStdString(ciefde_sforth_code());
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

