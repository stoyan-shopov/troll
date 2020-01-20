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
#include "libtroll.hxx"

#define TYPE_DEBUG_ENABLED	0
/* the first node in the vector is the head of the type graph
 * returns the position in the vector at which the new node is placed */
int DwarfData::readType(uint32_t die_offset, std::vector<struct DwarfTypeNode> & type_cache, bool reset_recursion_detector)
{
	int index;
	if (reset_recursion_detector)
		recursion_detector.clear();
	uint32_t saved_die_offset(die_offset);
	if (TYPE_DEBUG_ENABLED) qDebug() << "reading type die at offset" << QString("$%1").arg(die_offset, 0, 16);
	if (recursion_detector.operator [](saved_die_offset))
	{
		if (TYPE_DEBUG_ENABLED) qDebug() << "!!! type chain recursion detected";
		return recursion_detector.operator [](saved_die_offset);
	}
	auto type = debug_tree_of_die(die_offset);
	if (type.size() != 1)
		DwarfUtil::panic();
	struct DwarfTypeNode node(type.at(0));
	struct Abbreviation a(debug_abbrev + node.die.abbrev_offset);
	DwarfUtil::attribute_data t;
	if ((t = a.dataForAttribute(DW_AT_abstract_origin, debug_info + node.die.offset)).form
		|| (t = a.dataForAttribute(DW_AT_import, debug_info + node.die.offset)).form
		|| (t = a.dataForAttribute(DW_AT_specification, debug_info + node.die.offset)).form
			)
	{
		return readType(DwarfUtil::formReference(t.form, t.debug_info_bytes,
		/*! \todo	this is braindamaged; whoever passes the abbreviation cache to this function
		 *		should already have information about the containing compilation unit - maybe
		 *		just pass that as an additional parameter */
			compilationUnitOffsetForOffsetInDebugInfo(saved_die_offset)
			), type_cache, false);
	}
	
	type_cache.push_back(node);
	recursion_detector.operator [](saved_die_offset) = index = type_cache.size() - 1;

	t = a.dataForAttribute(DW_AT_type, debug_info + type_cache.at(index).die.offset);
	if (t.form)
	{
                int i;
		/*! \todo	this is braindamaged; whoever passes the abbreviation cache to this function
		 *		should already have information about the containing compilation unit - maybe
		 *		just pass that as an additional parameter */
		uint32_t cu_offset(compilationUnitOffsetForOffsetInDebugInfo(saved_die_offset));
		/* generally, the referred type can be in another compilation unit; gcc has not been observed
		 * to do this until now (02022017), but the IAR compiler does generate such references;
		 * if this is the case, the abbreviations of the referred compilation unit must be fetched,
		 * so do fetch them in all cases */
		auto x = readTypeOffset(t.form, t.debug_info_bytes, cu_offset);
                i = readType(x, type_cache, false);
                type_cache.at(index).next = i;
                if (TYPE_DEBUG_ENABLED) qDebug() << "read type die at index " << i;
        }

	if (type_cache.at(index).die.children.size())
	{
		int i, x;
		for (i = 0; i < type_cache.at(index).die.children.size(); i ++)
		{
			if (TYPE_DEBUG_ENABLED) qDebug() << "reading type child" << i << ", offset is" << type_cache.at(index).die.children.at(i).offset;
			x = readType(type_cache.at(index).die.children.at(i).offset, type_cache, false);
			type_cache.at(index).children.push_back(x);
			if (type_cache.at(x).die.tag == DW_TAG_subrange_type)
			{
				/*! \todo	process here DW_AT_count attributes; clang generates DW_AT_count attributes
				 * instead of DW_AT_upper_bound, while gcc generates DW_AT_upper_bound attributes instead of
				 * DW_AT_count */
				Abbreviation a(debug_abbrev + type_cache.at(x).die.abbrev_offset);
				auto subrange = a.dataForAttribute(DW_AT_upper_bound, debug_info + type_cache.at(x).die.offset);
				if (subrange.form == 0)
					/*! \todo	at least some versions of gcc are known to omit the upper bound attribute if it is 0;
					 *		maybe have the option to store a zero here */
					type_cache.at(index).array_dimensions.push_back(DwarfTypeNode::array_dimension(-1));
				else
				{
					if (DwarfUtil::isClassConstant(subrange.form))
						type_cache.at(index).array_dimensions.push_back(DwarfTypeNode::array_dimension(DwarfUtil::formConstant(subrange) + 1));
					else if (DwarfUtil::isClassReference(subrange.form))
						type_cache.at(index).array_dimensions.push_back(DwarfTypeNode::array_dimension(0, type_cache.at(x).die.offset));
					else
						DwarfUtil::panic();
				}
			}
		}
	}
	
	if (TYPE_DEBUG_ENABLED) qDebug() << "done" << QString("$%1").arg(saved_die_offset, 0, 16);
	return index;
}

DwarfBaseType DwarfData::readBaseOrGenericType(uint32_t die_offset)
{
	if (!die_offset)
		/* Special case - a die_offset of zero means that the 'generic' type, described in the Dwarf 5 standard, should be returned.
		 * This special case is described in the Dwarf 5 standard, in the section describing 'Type conversions' */
		return DwarfBaseType(sizeof(uint32_t), DW_ATE_unsigned);
	std::vector<DwarfTypeNode> base_type;
	readType(die_offset, base_type);
	/* Make sure the type die is indeed a base type die, and the bytesize of this base type can fit in the
	 * sforth stack */
	int base_type_encoding = baseTypeEncoding(base_type), bytesize = sizeOf(base_type);
	if (base_type.size() != 1 || base_type_encoding == -1 || bytesize > sizeof(uint64_t))
		DwarfUtil::panic();
	return DwarfBaseType(bytesize, base_type_encoding);
}

bool DwarfData::isPointerType(const std::vector<DwarfTypeNode> &type, int node_number)
{
	if (node_number == -1)
		return false;
	switch (type.at(node_number).die.tag)
	{
		case DW_TAG_rvalue_reference_type:
		case DW_TAG_reference_type:
		case DW_TAG_pointer_type:
		case DW_TAG_ptr_to_member_type:
			return true;
	}
	return false;
}

bool DwarfData::isArrayType(const std::vector<DwarfTypeNode> &type, int node_number)
{
	if (node_number == -1)
		return false;
	switch (type.at(node_number).die.tag)
	{
		case DW_TAG_array_type:
			return true;
	}
	return false;
}

bool DwarfData::isSubroutineType(const std::vector<DwarfTypeNode> &type, int node_number)
{
	if (node_number == -1)
		return false;
	switch (type.at(node_number).die.tag)
	{
		case DW_TAG_subroutine_type:
			return true;
	}
	return false;
}

int DwarfData::baseTypeEncoding(const std::vector<DwarfTypeNode> &type, int node_number)
{
	if (node_number == -1)
		return -1;
	const auto& die = type.at(node_number).die;
	if (die.tag != DW_TAG_base_type)
		return -1;
	Abbreviation a(debug_abbrev + die.abbrev_offset);
	auto e = a.dataForAttribute(DW_AT_encoding, debug_info + die.offset);
	if (e.form)
		return DwarfUtil::formConstant(e);
	return -1;
}

std::string DwarfData::typeString(std::vector<struct DwarfTypeNode> & type, int node_number, TypePrintFlags flags, bool reset_type_recursion_detection_flags)
{
	if (reset_type_recursion_detection_flags)
		for (auto x = type.begin(); x != type.end(); x->type_print_recursion_flag = false, x ++);
	std::string type_string;
	type_string = typeChainString(type, true, node_number, flags);
	//type_string += nameOfDie(type.at(node_number).die);
	type_string += typeChainString(type, false, node_number, flags);
	return type_string;
}
