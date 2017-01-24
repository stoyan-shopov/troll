#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <QVector>
#include <QByteArray>

struct memory_range
{
	uint32_t	address;
	QByteArray	data;
};

class Memory
{
private:
	QVector<struct memory_range> ranges;
public:
	Memory();
	void addRange(uint32_t address, const QByteArray & data)
	{
		int i;
		for (i = 0; i < ranges.size(); i ++)
			if (ranges[i].address <= address && address < ranges[i].address + ranges[i].data.size()
				|| address <= ranges[i].address && ranges[i].address < address + data.size())
			{
				/* regions overlap - coalesce */
				QByteArray x;
				if (address >= ranges[i].address)
				{
					x = ranges[i].data.left(address - ranges[i].address);
					x += data;
					address = ranges[i].address;
				}
				else
					x = data + ranges[i].data.right(ranges[i].data.size() - ((address + data.size()) - ranges[i].address));
				ranges[i] = (struct memory_range) { .address = address, .data = data, };
				break;
			}
	}
	void dump(void)
	{
		int i;
		for (i = 0; i < ranges.size(); i ++)
			qDebug() << "memory range at" << ranges[i].address << "size" << ranges[i].data.size();
	}
};

#endif // MEMORY_H
