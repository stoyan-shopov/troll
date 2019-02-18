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
#ifndef DWARFEXPRESSION_HXX
#define DWARFEXPRESSION_HXX

#include <QObject>

#include "sforth.hxx"
#include "util.hxx"
#include "libtroll.hxx"
#include "registercache.hxx"
#include "target.hxx"

class DwarfEvaluator : public QObject
{
	Q_OBJECT
private:
	Sforth	* sforth;
	int saved_active_register_frame_number;
public:
	struct DwarfCompositeLocation;

	/*! \warning	these constants must match the dwarf expression type constants in file 'dwarf-evaluator.fs' */
	/*! \todo	On startup initialization, load the constants in 'dwarf-evaluator.fs' with these values, to
	 *		maintain consistency and to avoid duplication */
	struct DwarfExpressionValue
	{
		enum DwarfExpressionType
		{
			INVALID		= 0,
			/*! \todo	Constant values are not yet handled properly, fix this */
			CONSTANT	= 1,
			MEMORY_ADDRESS	= 2,
			REGISTER_NUMBER	= 3,
			/* The dwarf location description that has been evaluated is a composite location description, i.e.
			 * a location description that contains dwarf opcodes 'DW_OP_piece' and/or 'DW_OP_bit_piece'.
			 * Information about the different pieces are stored in the 'compositeLocation' field */
			COMPOSITE_VALUE	= 4,
		}
		type;
		uint32_t				value;
		/*! \todo	This isn't really nice placed here, as the DwarfExpressionValue and DwarfCompositePieceLocation
		 * 		get mutually recursive, and this is not really needed. Make this more sane */
		std::list<DwarfCompositeLocation>	pieces;
	};

	struct DwarfCompositeLocation
	{
		struct DwarfExpressionValue details;
		int	byte_size;
	};

	DwarfEvaluator(class Sforth * sforth,
		       class DwarfData * /* needed for evaluating dwarf expressions containing DW_OP_entry_value opcodes */ libtroll,
		       class RegisterCache * /* needed for evaluating dwarf expressions containing DW_OP_entry_value opcodes */ register_cache_class
		       );
	struct DwarfExpressionValue evaluateLocation(uint32_t cfa_value, const QString & frameBaseSforthCode, const QString & locationSforthCode, bool reset_expression_evaluator = true);
	void entryValueReady(struct DwarfExpressionValue entry_value) { emit entryValueComputed(entry_value); }
	/* The format of the returned data is this - for each successfully retrieved byte of data, two ascii
	 * bytes are present, which contain the hexadecimal representation of that byte. For each
	 * byte that is unavailable (e.g., it has been optimized away, or can not be retrieved from the target),
	 * the bytes "??" are * stored */
	static QByteArray fetchValueFromTarget(const struct DwarfExpressionValue& location, Target * target, int bytesize = -1);
signals:
	void entryValueComputed(struct DwarfEvaluator::DwarfExpressionValue entry_value);
};

#endif // DWARFEXPRESSION_HXX
