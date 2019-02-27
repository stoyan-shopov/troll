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
#ifndef REGISTERCACHE_H
#define REGISTERCACHE_H

#include <stdint.h>
#include <vector>
#include <QDebug>
#include "util.hxx"

class RegisterCache
{
private:
	std::vector<std::vector<uint32_t> > register_frames;
	int active_frame;
public:
	RegisterCache(void) { }
	void clear(void) { register_frames.clear(); active_frame = -1; }
	void pushFrame(const std::vector<uint32_t> & register_frame) { register_frames.push_back(register_frame); }
	int frameCount(void) { return register_frames.size(); }
	int registerCount(void) { return register_frames[active_frame].size(); }
	void setActiveFrame(int frame_number) { if (frame_number >= register_frames.size()) Util::panic(); active_frame = frame_number; }
	int activeFrame(void) { return active_frame; }
	uint32_t readCachedRegister(unsigned register_number, int frame_offset = 0)
	{
		if (active_frame + frame_offset >= register_frames.size())
			Util::panic();
		if (register_number >= register_frames[active_frame + frame_offset].size()) Util::panic();
		if (register_number == 15)
		{
			qDebug() << "reading pc";
		}
		return register_frames[active_frame + frame_offset][register_number];
	}
};

#endif // REGISTERCACHE_H
