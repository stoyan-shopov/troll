#include <QFile>
#include "dwarf-evaluator.hxx"


DwarfEvaluator::DwarfEvaluator(Sforth *sforth)
{
	this->sforth = sforth;
	QFile f(":/sforth/dwarf-evaluator.fs");
	f.open(QFile::ReadOnly);
	sforth->evaluate(f.readAll());
}

void DwarfEvaluator::evaluateLocation(uint32_t cfa_value, const QString &frameBaseSforthCode, const QString &locationSforthCode)
{
	sforth->evaluate(QString("init-dwarf-evaluator $%1 to cfa-value").arg(cfa_value, 0, 16));
	if (!frameBaseSforthCode.isEmpty())
		sforth->evaluate(frameBaseSforthCode + " to frame-base-value ' frame-base-defined >vector frame-base-rule");
	sforth->evaluate(locationSforthCode);
}
