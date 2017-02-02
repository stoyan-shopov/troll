#ifndef UTIL_H
#define UTIL_H


class Util
{
public:
	static void panic(...) { *(int*)0=0; }
	template<typename T> static T min(T x, T y) { return x < y ? x : y; }
	template<typename T> static T max(T x, T y) { return x > y ? x : y; }
};

#endif // UTIL_H
