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

/*! \todo	These are bad... */
static DwarfData * libtroll;
static RegisterCache * register_cache;
static DwarfEvaluator * dwarf_evaluator;
static Sforth * sforth;

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
				break;
			}
		}
		else
			DwarfUtil::panic();
		composite_location_pieces.push_back(l);
		/* By default, make the next piece a memory address location */
		sforth->evaluate("expression-is-a-memory-address to expression-value-type");
	}
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
