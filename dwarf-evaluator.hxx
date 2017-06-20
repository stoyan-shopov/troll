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
#ifndef DWARFEXPRESSION_HXX
#define DWARFEXPRESSION_HXX

#include <QObject>

#include "sforth.hxx"
#include "util.hxx"
#include "libtroll.hxx"
#include "registercache.hxx"

class DwarfEvaluator : public QObject
{
	Q_OBJECT
private:
	Sforth	* sforth;
	int saved_active_register_frame_number;
public:
	/*! \warning	these constants must match the dwarf expression type constants in file 'dwarf-evaluator.fs' */
	enum DwarfExpressionType
	{
		INVALID		= 0,
		CONSTANT	= 1,
		MEMORY_ADDRESS	= 2,
		REGISTER_NUMBER	= 3,
	};
	struct DwarfExpressionValue
	{
		uint32_t			value;
		enum DwarfExpressionType	type;
	};
	DwarfEvaluator(class Sforth * sforth,
	               class DwarfData * /* needed for evaluating dwarf expressions containing DW_OP_entry_value opcodes */ libtroll_class,
	               class RegisterCache * /* needed for evaluating dwarf expressions containing DW_OP_entry_value opcodes */ register_cache_class
	               );
	struct DwarfExpressionValue evaluateLocation(uint32_t cfa_value, const QString & frameBaseSforthCode, const QString & locationSforthCode, bool reset_expression_evaluator = true);
	void entryValueReady(struct DwarfExpressionValue entry_value) { emit entryValueComputed(entry_value); }
signals:
	void entryValueComputed(struct DwarfEvaluator::DwarfExpressionValue entry_value);
};

#endif // DWARFEXPRESSION_HXX
