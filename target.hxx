#ifndef TARGET_H
#define TARGET_H

#include <stdint.h>
#include <QObject>
#include <QDebug>
#include <QXmlStreamReader>
#include <list>

class Target : public QObject
{
	Q_OBJECT
public:
	struct ram_area
	{
		uint32_t	start;
		uint32_t	length;
	};
	struct flash_area
	{
		uint32_t	start;
		uint32_t	length;
		unsigned	blocksize;
	};
	virtual uint32_t readWord(uint32_t address) = 0;
	virtual QByteArray readBytes(uint32_t address, int byte_count) = 0;
	virtual uint32_t readRegister(uint32_t register_number) = 0;
	virtual uint32_t singleStep(void) = 0;
	void parseMemoryAreas(const QString & xml_memory_description)
	{
		uint32_t start, length;
		bool is_ram = true;
		int blocksize, i;
		QXmlStreamReader xml(xml_memory_description);
		ram_areas.clear();
		flash_areas.clear();

		while (!xml.atEnd())
		{
			xml.readNext();
			auto x = xml.attributes();
			qDebug() << "xml element:" << xml.name() << xml.text() << (xml.isEndElement() ? "end":"");
			if (xml.isEndElement() && xml.name() == "memory")
			{
				if (is_ram)
					ram_areas.push_back((struct ram_area) { .start = start, .length = length, });
				else
					flash_areas.push_back((struct flash_area) { .start = start, .length = length, .blocksize = blocksize, });
			}
			for (i = 0; i < x.size(); i ++)
			{
				qDebug() << x[i].name() << x[i].value();
				if (x[i].name().contains("type"))
					is_ram = x[i].value().contains("ram") ? true : false;
				if (x[i].name().contains("start"))
					start = x[i].value().toUInt(0, 0);
				if (x[i].name().contains("length"))
					length = x[i].value().toUInt(0, 0);
				if (x[i].name().contains("name") && x[i].value().contains("blocksize"))
				{
					xml.readNext();
					if (!xml.atEnd())
						blocksize = xml.text().toUInt(0, 0);
					break;
				}
			}
		}
		qDebug() << "detected ram areas:";
		for (i = 0; i < ram_areas.size(); i++)
			qDebug() << ram_areas[i].start << ram_areas[i].length;
		qDebug() << "detected flash areas:";
		for (i = 0; i < flash_areas.size(); i++)
			qDebug() << flash_areas[i].start << flash_areas[i].length;
	}
protected:
	std::vector<struct ram_area> ram_areas;
	std::vector<struct flash_area> flash_areas;
};

#endif // TARGET_H
