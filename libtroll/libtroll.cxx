#include "libtroll.hxx"

#define TYPE_DEBUG_ENABLED	1
/* the first node in the vector is the head of the type graph
 * returns the position in the vector at which the new node is placed */
int DwarfData::readType(uint32_t die_offset, std::map<uint32_t, uint32_t> & abbreviations, std::vector<struct DwarfTypeNode> & type_cache, bool reset_recursion_detector)
{
	int index;
	if (reset_recursion_detector)
		recursion_detector.clear();
	uint32_t saved_die_offset(die_offset);
	if (TYPE_DEBUG_ENABLED) qDebug() << "reading type die at offset" << QString("$%1").arg(die_offset, 0, 16);
	/*
	if (recursion_detector.find(die_offset) != recursion_detector.end())
		DwarfUtil::panic("type chain recursion detected");
		*/
	if (recursion_detector.operator [](saved_die_offset))
	{
		if (TYPE_DEBUG_ENABLED) qDebug() << "!!! type chain recursion detected";
		return recursion_detector.operator [](saved_die_offset);
	}
	if (die_offset == 76)
	{
		die_offset = 76;
	}
	auto type = debug_tree_of_die(die_offset, abbreviations);
	if (type.size() != 1)
		DwarfUtil::panic();
	struct DwarfTypeNode node(type.at(0));
	type_cache.push_back(node);
	recursion_detector.operator [](saved_die_offset) = index = type_cache.size() - 1;
	//struct DwarfTypeNode * pnode = & type_cache.at(index);
	struct Abbreviation a(debug_abbrev + type.at(0).abbrev_offset);
	auto t = a.dataForAttribute(DW_AT_type, debug_info + type_cache.at(index).die.offset);
	if (t.first)
	{
                int i;
		/*! \todo	this is braindamaged; whoever passes the abbreviation cache to this function
		 *		should already have information about the containing compilation unit - maybe
		 *		just pass that as an additional parameter */
		uint32_t cu_offset(compilationUnitOffsetForOffsetInDebugInfo(saved_die_offset));
                i = readType(readTypeOffset(t.first, t.second, cu_offset), abbreviations, type_cache, false);
                type_cache.at(index).next = i;
                if (TYPE_DEBUG_ENABLED) qDebug() << "read type die at index " << i;
        }
	if (type_cache.at(index).die.children.size())
	{
                int i, x, y;
		if (TYPE_DEBUG_ENABLED) qDebug() << "aggregate type, child count" << type_cache.at(index).die.children.size();
                x = readType(type_cache.at(index).die.children.at(0).offset, abbreviations, type_cache, false);
                type_cache.at(index).childlist = x;
                for (i = 1; i < type_cache.at(index).die.children.size(); i ++)
		{
			if (TYPE_DEBUG_ENABLED) qDebug() << "reading type child" << i << ", offset is" << type_cache.at(index).die.children.at(i).offset;
                        //* x = readType(type_cache.at(index).die.children.at(i).offset, abbreviations, type_cache), x = & type_cache.at(* x).sibling;
                        y = readType(type_cache.at(index).die.children.at(i).offset, abbreviations, type_cache, false);
                        type_cache.at(x).sibling = y;
                        x = y;
                }
	}
	if (TYPE_DEBUG_ENABLED) qDebug() << "done" << QString("$%1").arg(saved_die_offset, 0, 16);
	return index;
}
