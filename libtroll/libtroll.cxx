#include "libtroll.hxx"

#define TYPE_DEBUG_ENABLED	0
/* the first node in the vector is the head of the type graph
 * returns the position in the vector at which the new node is placed */
int DwarfData::readType(uint32_t die_offset, std::map<uint32_t, uint32_t> & abbreviations, std::vector<struct DwarfTypeNode> & type_cache, bool reset_recursion_detector)
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
	auto type = debug_tree_of_die(die_offset, abbreviations);
	if (type.size() != 1)
		DwarfUtil::panic();
	struct DwarfTypeNode node(type.at(0));
	struct Abbreviation a(debug_abbrev + node.die.abbrev_offset);
	std::pair<uint32_t, const uint8_t *> t;
	if ((t = a.dataForAttribute(DW_AT_abstract_origin, debug_info + node.die.offset)).first
		|| (t = a.dataForAttribute(DW_AT_import, debug_info + node.die.offset)).first
		|| (t = a.dataForAttribute(DW_AT_specification, debug_info + node.die.offset)).first
			)
	{
		return readType(DwarfUtil::formReference(t.first, t.second,
		/*! \todo	this is braindamaged; whoever passes the abbreviation cache to this function
		 *		should already have information about the containing compilation unit - maybe
		 *		just pass that as an additional parameter */
			compilationUnitOffsetForOffsetInDebugInfo(saved_die_offset)
			), abbreviations, type_cache, false);
	}
	
	type_cache.push_back(node);
	recursion_detector.operator [](saved_die_offset) = index = type_cache.size() - 1;

	t = a.dataForAttribute(DW_AT_type, debug_info + type_cache.at(index).die.offset);
	if (t.first)
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
		auto x = readTypeOffset(t.first, t.second, cu_offset);
		std::map<uint32_t, uint32_t> abbreviations;
		get_abbreviations_of_compilation_unit(compilationUnitOffsetForOffsetInDebugInfo(x), abbreviations);
                i = readType(x, abbreviations, type_cache, false);
                type_cache.at(index).next = i;
                if (TYPE_DEBUG_ENABLED) qDebug() << "read type die at index " << i;
        }

	if (type_cache.at(index).die.children.size())
	{
		int i, x;
		if (TYPE_DEBUG_ENABLED) qDebug() << "aggregate type, child count" << type_cache.at(index).die.children.size();
		x = readType(type_cache.at(index).die.children.at(0).offset, abbreviations, type_cache, false);
		type_cache.at(index).children.push_back(x);
		if (type_cache.at(x).die.tag == DW_TAG_subrange_type)
		{
			Abbreviation a(debug_abbrev + type_cache.at(x).die.abbrev_offset);
			auto subrange = a.dataForAttribute(DW_AT_upper_bound, debug_info + type_cache.at(x).die.offset);
			if (subrange.first == 0)
				/*! \todo	at least some versions of gcc are known to omit the upper bound attribute if it is 0;
				 *		maybe have the option to store a zero here */
				type_cache.at(index).array_dimensions.push_back(-1);
			else
				type_cache.at(index).array_dimensions.push_back(DwarfUtil::formConstant(subrange.first, subrange.second) + 1);
		}
		for (i = 1; i < type_cache.at(index).die.children.size(); i ++)
		{
			if (TYPE_DEBUG_ENABLED) qDebug() << "reading type child" << i << ", offset is" << type_cache.at(index).die.children.at(i).offset;
			x = readType(type_cache.at(index).die.children.at(i).offset, abbreviations, type_cache, false);
			type_cache.at(index).children.push_back(x);
		}
	}
	
	if (TYPE_DEBUG_ENABLED) qDebug() << "done" << QString("$%1").arg(saved_die_offset, 0, 16);
	return index;
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
