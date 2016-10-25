#ifndef UTIL_H
#define UTIL_H


class Util
{
public:
	static void panic(...) { *(int*)0=0; }
};

#endif // UTIL_H
