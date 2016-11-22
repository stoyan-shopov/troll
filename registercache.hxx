#ifndef REGISTERCACHE_H
#define REGISTERCACHE_H

#include <stdint.h>
#include <vector>

class RegisterCache
{
private:
	std::vector<std::vector<uint32_t> > register_frames;
	uint32_t
	/*! \todo	extract this from the .debug_frame dwarf cie program, and make it a generic rule - not simply a register number */ cfa_register_number;
public:
	RegisterCache(uint32_t cfa_register_number) { this->cfa_register_number = cfa_register_number; }
	void clear(void) { register_frames.clear(); }
	void pushFrame(const std::vector<uint32_t> & register_frame) { register_frames.push_back(register_frame); }
	const std::vector<uint32_t> registerFrame(uint32_t frame_number) { return register_frames.at(frame_number); }
};

#endif // REGISTERCACHE_H
