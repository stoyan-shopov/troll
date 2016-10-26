#include "sforth.hxx"

extern "C"
{
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "sf-cfg.h"
#include "sf-arch.h"

static QPlainTextEdit	* sforth_console;
static QString sfbuf;

int sfgetc(void) { return EOF; }
int sffgetc(cell file_id) { return EOF; }
int sfsync(void) { sforth_console->appendPlainText(sfbuf); sfbuf.clear(); return 0; }
int sfputc(int c) { sfbuf += c; if (c == '\n') sfsync(); return c; }
cell sfopen(const char * pathname, int flags) { return -1; }
int sfclose(cell file_id) { return EOF; }
int sffseek(cell stream, long offset) { return -1; }
}

Sforth::Sforth(QPlainTextEdit * console)
{
	sforth_console = console;
	sf_reset();
}

void Sforth::evaluate(const QString &sforth_commands)
{
	sforth_console->appendPlainText(QString(">>> ") + sforth_commands);
	sf_eval(sforth_commands.toLocal8Bit().data());
}

std::vector<cell> Sforth::getResults(int result_count)
{
cell x[result_count];
int i, n;
std::vector<cell> result;
	n = sf_get_results(x, result_count);
	for (i = 0; i < n; result.push_back(x[i++]));
	return result;
}