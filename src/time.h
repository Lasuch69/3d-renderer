#ifndef TIME_H
#define TIME_H

#include <SDL2/SDL.h>
#include <cstdint>

class Time {
	uint64_t _now = SDL_GetPerformanceCounter();
	uint64_t _last = 0;
	double _deltaTime = 0;

public:
	inline void startNewFrame() {
		_last = _now;
		_now = SDL_GetPerformanceCounter();

		_deltaTime = (double)((_now - _last) / (double)SDL_GetPerformanceFrequency());
	}

	inline double getDeltaTime() {
		return _deltaTime;
	}
};

#endif // !TIME_H
