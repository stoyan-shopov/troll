#include "libtroll.hxx"

/* the first node in the vector is the head of the type graph
 * returns the position in the vector at which the new node is placed */
int DwarfData::readType(uint32_t die_offset, std::map<uint32_t, uint32_t> & abbreviations, std::vector<struct DwarfTypeNode> & type_cache)
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
