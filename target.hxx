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
#ifndef TARGET_H
#define TARGET_H

#include <stdint.h>
#include <QObject>
#include <QDebug>
#include <QXmlStreamReader>
#include <list>

#include "util.hxx"

enum TARGET_HALT_REASON
{
	UNKNOWN		= 0,
	BREAKPOINT_HIT,
	GENERIC_HALT_CONDITION,
};

class Target : public QObject
{
	Q_OBJECT
signals:
	void targetHalted(enum TARGET_HALT_REASON reason);
	void targetRunning(void);
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
	virtual bool reset(void) = 0;
	virtual QByteArray readBytes(uint32_t address, int byte_count, bool is_failure_allowed = false) = 0;
	virtual uint32_t readRawUncachedRegister(uint32_t register_number) = 0;
	virtual uint32_t singleStep(void) = 0;
	virtual bool breakpointSet(uint32_t address, int length) = 0;
	virtual bool breakpointClear(uint32_t address, int length) = 0;
	virtual void requestSingleStep(void) = 0;
	virtual bool resume(void) = 0;
	virtual bool requestHalt(void) = 0;
	virtual uint32_t haltReason(void) = 0;
	virtual QByteArray interrogate(const QByteArray & query, bool * isOk = 0) = 0;
	QByteArray interrogate(const QString & query, bool * isOk = 0) { return interrogate(query.toLocal8Bit(), isOk); }
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
		std::sort(flash_areas.begin(), flash_areas.end(), compare_memory_areas);
	}
	/* if the memory range passed does not fit entirely in flash memory, an empty vector is returned */
	std::vector<std::pair<uint32_t /* start address in flash */, uint32_t /* length of flash area */> > flashAreasForRange(uint32_t address, uint32_t length)
	{
		int i;
		std::vector<std::pair<uint32_t, uint32_t> > ranges;
		for (i = 0; length && i < flash_areas.size(); i++)
			if (flash_areas[i].start <= address && address < flash_areas[i].start + flash_areas[i].length)
			{
				auto x = Util::min(length, flash_areas[i].start + flash_areas[i].length - address);
				ranges.push_back(std::pair<uint32_t, uint32_t>(address, x));
				address += x;
				length -= x;
				x = ((ranges[i].first - flash_areas[i].start) % flash_areas[i].blocksize);
				ranges[i].first -= x;
				ranges[i].second += x;
				x = flash_areas[i].blocksize;
				ranges[i].second += (x - (ranges[i].second % x)) % x;
			}
		if (length)
			ranges.clear();
		return ranges;
	}
protected:
	std::vector<struct ram_area> ram_areas;
	std::vector<struct flash_area> flash_areas;
private:
	static bool compare_memory_areas(const struct flash_area & first, const struct flash_area & second) { return first.start < second.start; }
};

#endif // TARGET_H
