#ifndef REGISTERCACHE_H
#define REGISTERCACHE_H

#include <stdint.h>
#include <vector>
#include "util.hxx"

class RegisterCache
{
private:
	std::vector<std::vector<uint32_t> > register_frames;
public:
	RegisterCache(void) { }
	void clear(void) { register_frames.clear(); }
	void pushFrame(const std::vector<uint32_t> & register_frame) { register_frames.push_back(register_frame); }
	const std::vector<uint32_t> cachedRegisterFrame(uint32_t frame_number)
	{ if (frame_number < register_frames.size()) return register_frames.at(frame_number); else Util::panic();}
	int frameCount(void) { return register_frames.size(); }
};

#endif // REGISTERCACHE_H
