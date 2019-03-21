/*
Copyright (c) 2016-2017, 2019 Stoyan Shopov

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
#include <QMessageBox>
#include <QFile>
#include "dwarf-evaluator.hxx"

#include "dwarf-type-stack.hxx"

/*! \todo	These are bad... */
static DwarfData * libtroll;
static RegisterCache * register_cache;
static DwarfEvaluator * dwarf_evaluator;
static Sforth * sforth;
static std::vector<dwarf_type_stack_entry> dwarf_type_stack;

extern "C"
{
	/* only DW_OP_regX dwarf opcodes are accepted in DW_OP_entry_value expression blocks */
	/*! \todo	Test this - provide test samples. Test recursive, multiple nested entry value expression evaluation that
	 *		utilizes the 'expression-stack' stack */
	void do_entry_value_expression_block(void)
	{
		struct Die call_site;
		std::vector<struct Die> context;
		int register_number;
		if (!libtroll->callSiteAtAddress(register_cache->readCachedRegister(/*! \todo fix this, do not hardcode it!!! */ 15, 1) &~ 1, call_site, & context))
		{
			print_str(__func__);
			print_str("(): call site not found, aborting DW_OP_entry_value evaluation");
			do_cr();
			do_abort();
		}
		if (sf_pop() != 1 || (register_number = DwarfExpression::registerNumberOfRegXOpcode(sf_pop())) == -1)
		{
			print_str(__func__);
			print_str("(): unsupported dwarf expression for DW_OP_entry_value dwarf opcode, aborting expression evaluation");
			do_cr();
			Util::panic();
			do_abort();
		}
		auto l = libtroll->callSiteValueDwarfExpressionForRegisterNumber(call_site, register_number);
		if (!l.first)
		{
			print_str(__func__);
			print_str("(): unavailable entry value expression, aborting expression evaluation");
			do_cr();
			do_abort();
		}
		sf_eval(">expression-stack");
		register_cache->setActiveFrame(register_cache->activeFrame() + 1);
		auto result = dwarf_evaluator->evaluateLocation(register_cache->readCachedRegister(13), QString::fromStdString(libtroll->sforthCodeFrameBaseForContext(context, register_cache->readCachedRegister(15))), QString::fromStdString(DwarfExpression::sforthCode(l.second, l.first)), false);
#if 0
		if (result.type != DwarfEvaluator::CONSTANT)
			Util::panic();
#endif
		sf_eval("expression-stack>");
		register_cache->setActiveFrame(register_cache->activeFrame() - 1);
		sf_push(result.value);
		dwarf_evaluator->entryValueReady(result);
	}

static std::list<DwarfEvaluator::DwarfCompositeLocation> composite_location_pieces;

	void do_DW_OP_piece(void)
	{
		DwarfEvaluator::DwarfCompositeLocation l;
		l.byte_size = sf_pop();
		sforth->evaluate("expression-value-type");
		auto x = sforth->getResults(2);
		if (x.size() == 1)
			/* Piece unavailable */
			l.details.type = DwarfEvaluator::DwarfExpressionValue::INVALID;
		else if (x.size() == 2)
		{
			l.details.type = (enum DwarfEvaluator::DwarfExpressionValue::DwarfExpressionType) x.at(1), l.details.value = x.at(0);
			switch (l.details.type)
			{
			default: DwarfUtil::panic();
			case DwarfEvaluator::DwarfExpressionValue::MEMORY_ADDRESS: case DwarfEvaluator::DwarfExpressionValue::REGISTER_NUMBER:
			case DwarfEvaluator::DwarfExpressionValue::CONSTANT:
				break;
			}
		}
		else
			DwarfUtil::panic();
		composite_location_pieces.push_back(l);
		/* By default, make the next piece a memory address location */
		sforth->evaluate("expression-is-a-memory-address to expression-value-type");
		/* Empty the type stack for the next piece expression */
		dwarf_type_stack.clear();
	}

	void do_DW_OP_implicit_value(void)
	{
		cell bytesize = sf_pop(), implicit_value = 0;
		int i;
		/* Make sure the bytesize of the implicit constant is a supported one */
		if (bytesize > sizeof(cell) || (bytesize != 4 && bytesize != 8))
			DwarfUtil::panic();
		for (i = 0; i < bytesize; implicit_value |= sf_pop() << (i ++ << 3))
			;
		sf_push(implicit_value);
		sf_eval("DW_OP_stack_value");
	}

	/*
	 * Words handling type information attached to dwarf stack elements
	 */

	void do_DW_OP_regval_type(void)
	{
		cell base_type_die_offset = sf_pop(), register_number = sf_pop();
		std::vector<DwarfTypeNode> base_type;
		libtroll->readType(base_type_die_offset, base_type);
		/* Make sure the type die is indeed a base type die, and the bytesize of this base type can fit in the
		 * sforth stack */
		int base_type_encoding = libtroll->baseTypeEncoding(base_type), bytesize = libtroll->sizeOf(base_type);
		if (base_type.size() != 1 || base_type_encoding == -1 || bytesize > sizeof(cell))
			DwarfUtil::panic();
		/* Make sure only known base type lengths are handled */
		if (bytesize > sizeof(uint32_t) && bytesize != 8)
			DwarfUtil::panic();
		/*! \todo	Handling of floating point types that are 8 bytes in size, most commonly c 'double'
		 *		types - I need to find where this is documented, but it looks like in this case, gcc
		 * 		at least, places the 'double' value in two registers, and emits a 'DW_OP_regval_type'
		 *		opcode specifying the first register used - 'R{n}', and it contains (on low endian machines),
		 * 		the low order 4 bytes of the 'double' value, and the next four bytes of the 'double'
		 * 		value are placed in the next register, numbered 'R{n+1}'. So, for example, if the
		 * 		compiler decides to put a 'double' value in registers 'r0' and 'r1', it emits a
		 *		'DW_OP_regval_type' specifying register 'r0' */
		uint32_t r;
		cell value;
		r = register_cache->readCachedRegister(register_number);
		if (bytesize <= sizeof(uint32_t))
			value = r;
		else if (bytesize == 8)
			value = r | (((cell) register_cache->readCachedRegister(register_number + 1)) << 32);
		sf_push(value);
		/*! \todo	Save the dwarf type entry for later use. This really is not at all correct... */
		dwarf_type_stack.push_back(dwarf_type_stack_entry(base_type_encoding, bytesize, sf_get_depth()));
	}

	void do_DW_OP_const_type(void)
	{
		cell base_type_die_offset = sf_pop(), constant_block_size = sf_pop();
		std::vector<DwarfTypeNode> base_type;
		libtroll->readType(base_type_die_offset, base_type);
		/* Make sure the type die is indeed a base type die, and the bytesize of this base type can fit in the
		 * sforth stack */
		int base_type_encoding = libtroll->baseTypeEncoding(base_type), bytesize = libtroll->sizeOf(base_type);
		if (base_type.size() != 1 || base_type_encoding == -1 || bytesize > sizeof(cell) || constant_block_size != bytesize)
			DwarfUtil::panic();
		/* Make sure only known base type lengths are handled */
		if (constant_block_size > sizeof(uint32_t) && constant_block_size != 8)
			DwarfUtil::panic();
		union { cell value; uint8_t constant_block[sizeof(cell)]; } v = { .value = 0, };
		for (int i = 0; i < constant_block_size; v.constant_block[i ++] = sf_pop())
			;
		sf_push(v.value);
		/*! \todo	Save the dwarf type entry for later use. This really is not at all correct... */
		dwarf_type_stack.push_back(dwarf_type_stack_entry(base_type_encoding, constant_block_size, sf_get_depth()));
	}

	void do_DW_OP_deref_type(void)
	{
		cell base_type_die_offset = sf_pop(), type_bytesize = sf_pop();
		std::vector<DwarfTypeNode> base_type;
		libtroll->readType(base_type_die_offset, base_type);
		/* Make sure the type die is indeed a base type die, and the bytesize of this base type can fit in the
		 * sforth stack */
		int base_type_encoding = libtroll->baseTypeEncoding(base_type), bytesize = libtroll->sizeOf(base_type);
		if (base_type.size() != 1 || base_type_encoding == -1 || bytesize > sizeof(cell) || type_bytesize != bytesize)
			DwarfUtil::panic();
		/* Make sure only known base type lengths are handled */
		if (type_bytesize > sizeof(uint32_t) && type_bytesize != 8)
			DwarfUtil::panic();
		union { cell value; uint8_t constant_block[sizeof(cell)]; } v = { .value = 0, };
		uint32_t address = (uint32_t) sf_pop();
		sf_push((cell) & v.constant_block), sf_push(address), sf_push(type_bytesize);
		sf_eval("target-mem-read");
		/* Check for target read error */
		if (!sf_pop())
			DwarfUtil::panic();

		sf_push(v.value);
		/*! \todo	Save the dwarf type entry for later use. This really is not at all correct... */
		dwarf_type_stack.push_back(dwarf_type_stack_entry(base_type_encoding, type_bytesize, sf_get_depth()));
	}

	void do_DW_OP_convert(void)
	{
		/* In essence, this is like the C-language type cast operator: '(type)' */
		cell base_type_die_offset = sf_pop();
		std::vector<DwarfTypeNode> base_type;
		libtroll->readType(base_type_die_offset, base_type);
		/* Make sure the type die is indeed a base type die, and the bytesize of this base type can fit in the
		 * sforth stack */
		int base_type_encoding = libtroll->baseTypeEncoding(base_type), bytesize = libtroll->sizeOf(base_type);
		if (base_type.size() != 1 || base_type_encoding == -1 || bytesize > sizeof(cell))
			DwarfUtil::panic();
		/* Make sure only known base type lengths are handled */
		if (bytesize > sizeof(uint32_t) && bytesize != 8)
			DwarfUtil::panic();
		union { cell value; float f; double d; } v = { .value = sf_pop(), };
		if (!dwarf_type_stack.size())
			DwarfUtil::panic();
		auto cast_from_type = dwarf_type_stack.back();
		dwarf_type_stack.pop_back();
		switch (cast_from_type.dwarf_type_encoding)
		{
		default: DwarfUtil::panic();
		case DW_ATE_float:
			switch (cast_from_type.dwarf_type_bytesize)
			{
			default: DwarfUtil::panic();
			case 4:
				/* Typecast from 'float' */
				switch (bytesize)
				{
					default: DwarfUtil::panic();
					case 8:
					/* Typecast to 'double': (float) --> (double) */
					v.d = v.f;
				}
				break;
			}
		}

		sf_push(v.value);
		/*! \todo	Save the dwarf type entry for later use. This really is not at all correct... */
		dwarf_type_stack.push_back(dwarf_type_stack_entry(base_type_encoding, bytesize, sf_get_depth()));
	}

	void do_type_stack_nonempty(void)
	{
		sf_push(dwarf_type_stack.empty() ? 0 : -1);
	}

	void typed_dwarf_binary_operation(int dwarf_opcode)
	{
		/*! \todo	Check the type steck integrity. This is currently far from exact, but still better than nothing... */
		if (dwarf_type_stack.size() < 2)
			DwarfUtil::panic();
		auto op2 = dwarf_type_stack.back();
		dwarf_type_stack.pop_back();
		auto op1 = dwarf_type_stack.back();
		dwarf_type_stack.pop_back();
		if (op1.depth_of_data_node_in_data_stack + 1 != op2.depth_of_data_node_in_data_stack
				|| op1.dwarf_type_bytesize != op2.dwarf_type_bytesize
				|| op1.dwarf_type_encoding != op2.dwarf_type_encoding
				|| op2.depth_of_data_node_in_data_stack != sf_get_depth())
			DwarfUtil::panic();
		cell y = sf_pop(), x = sf_pop();
		switch (op1.dwarf_type_encoding)
		{
			default: DwarfUtil::panic();
			case DW_ATE_float: switch(op1.dwarf_type_bytesize)
			{
				default: DwarfUtil::panic();
				case 8:
			{
				double a = * (double *) & x, b = * (double *) & y, result;
				switch (dwarf_opcode)
				{
				default: DwarfUtil::panic();
				case DW_OP_plus: result = a + b; break;
				case DW_OP_minus: result = a - b; break;
				}
				sf_push(* (cell *) & result);
			}
				break;
				case 4:
			{
				float a = * (float *) & x, b = * (float *) & y, result;
				switch (dwarf_opcode)
				{
				default: DwarfUtil::panic();
				case DW_OP_plus: result = a + b; break;
				case DW_OP_minus: result = a - b; break;
				}
				sf_push(* (uint32_t *) & result);
			}
				break;
			}
		}
		/* Update the type of the result. This is actually equal to 'op1' */
		dwarf_type_stack.push_back(dwarf_type_stack_entry(op1.dwarf_type_encoding, op1.dwarf_type_bytesize, sf_get_depth()));
	}

	void do_DW_OP_plus_typed(void) { typed_dwarf_binary_operation(DW_OP_plus); }
	void do_DW_OP_minus_typed(void) { typed_dwarf_binary_operation(DW_OP_minus); }
}


DwarfEvaluator::DwarfEvaluator(class Sforth * sforth,
	       class DwarfData * /* needed for evaluating dwarf expressions containing DW_OP_entry_value opcodes */ libtroll,
	       RegisterCache * register_cache_class)
{
	dwarf_evaluator = this;
	::libtroll = libtroll;
	register_cache = register_cache_class;
	this->sforth = ::sforth = sforth;
	QFile f(":/sforth/dwarf-evaluator.fs");
	f.open(QFile::ReadOnly);
	sforth->evaluate(f.readAll());
}

DwarfEvaluator::DwarfExpressionValue DwarfEvaluator::evaluateLocation(uint32_t cfa_value, const QString &frameBaseSforthCode, const QString &locationSforthCode, bool reset_expression_evaluator)
{
	if (reset_expression_evaluator)
	{
		dwarf_type_stack.clear();
		saved_active_register_frame_number = register_cache->activeFrame();
		sforth->evaluate("expression-stack-reset");
		composite_location_pieces.clear();
	}
	DwarfExpressionValue result;
	sforth->evaluate(QString("init-dwarf-evaluator $%1 to cfa-value").arg(cfa_value, 0, 16));
	if (!frameBaseSforthCode.isEmpty())
		sforth->evaluate(frameBaseSforthCode + " to frame-base-value ' frame-base-defined >vector frame-base-rule");
	sforth->evaluate(locationSforthCode + " expression-value-type");
	
	auto x = sforth->getResults(2);
	if (x.size() != 2)
	{
		if (composite_location_pieces.size())
			result.type = DwarfExpressionValue::DwarfExpressionType::COMPOSITE_VALUE, result.pieces = composite_location_pieces;
		else
			result.type = DwarfExpressionValue::DwarfExpressionType::INVALID;
	}
	else
		result.type = (enum DwarfExpressionValue::DwarfExpressionType) x.at(1), result.value = x.at(0);
	if (reset_expression_evaluator)
		register_cache->setActiveFrame(saved_active_register_frame_number);
	return result;
}

QByteArray DwarfEvaluator::fetchValueFromTarget(const DwarfEvaluator::DwarfExpressionValue& location, Target * target, int bytesize)
{
QByteArray data;
	switch (location.type)
	{
		case DwarfEvaluator::DwarfExpressionValue::MEMORY_ADDRESS:
			data = target->readBytes(location.value, bytesize, true);
			break;
		case DwarfEvaluator::DwarfExpressionValue::REGISTER_NUMBER:
		{
			uint32_t register_contents = register_cache->readCachedRegister(location.value);
			data = QByteArray((const char *) & register_contents, sizeof register_contents);
		}
			break;
		case DwarfEvaluator::DwarfExpressionValue::COMPOSITE_VALUE:
			for (const auto& piece : location.pieces)
				data += fetchValueFromTarget(piece.details, target, piece.byte_size);
			return data;
		case DwarfEvaluator::DwarfExpressionValue::CONSTANT:
			data = QByteArray((const char *) & location.value, sizeof location.value);
			/*! \todo	Zero extend the data array that is returned, in case the expected
			 * 		byte size of the data item is greater than the length of the
			 * 		computed constant. Maybe this is inappropriate? */
			data.append(bytesize - data.size(), '\0');
			/* Truncate extend the data array that is returned, in case the expected
			 * byte size of the data item is less than the length of the
			 * computed constant. */
			data.truncate(bytesize);
			break;
		case DwarfEvaluator::DwarfExpressionValue::INVALID:
			/* Return an array containing deliberately invalid values, which is, however, of the proper size.
			 * IMPORTANT: it is the caller's responsibility to properly handle this special case! */
			return QByteArray(2 * bytesize, '?');
		default:
			DwarfUtil::panic();
	}
	return data.toHex();
}
