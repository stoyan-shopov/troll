#include <QFile>
#include "dwarf-evaluator.hxx"


DwarfEvaluator::DwarfEvaluator(Sforth *sforth)
{
	this->sforh = sforth;
	QFile f(":/sforth/dwarf-evaluator.fs");
	f.open(QFile::ReadOnly);
	sforth->evaluate(f.readAll());
}
