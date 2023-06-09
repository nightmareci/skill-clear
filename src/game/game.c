/*
 * MIT License
 * 
 * Copyright (c) 2023 Brandon McGriff <nightmareci@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "game/game.h"
#include "framework/render.h"
#include "framework/nanotime.h"
#include "framework/defs.h"
#include "SDL.h"
#include <inttypes.h>
#include <math.h>

#define TICK_RATE 300u
static uintptr_t ticks;
static bool reset_average;
static uint64_t average_ticks;
static uint64_t average_duration;
static uint64_t last_tick_time;

static const char* const text = "\
Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor\n\
incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis\n\
nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\n\
Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu\n\
fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in\n\
culpa qui officia deserunt mollit anim id est laborum.\
";

bool game_init(frames_struct* const frames, uint64_t* const tick_rate) {
	*tick_rate = TICK_RATE;
	ticks = 0u;
	last_tick_time = nanotime_now();
	reset_average = true;
	
	return render_init(frames);
}

bool game_update(frames_struct* const frames) {
	const uint64_t current_tick_time = nanotime_now();
	const double tick_rate = (double)NANOTIME_NSEC_PER_SEC / (current_tick_time - last_tick_time);
    const Uint8* const keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_SPACE]) {
        reset_average = true;
    }
	if (!reset_average) {
        average_ticks++;
        average_duration += current_tick_time - last_tick_time;
    }
	last_tick_time = current_tick_time;
	ticks++;

	if (!render_clear(frames, (uint8_t)((sinf(M_PIf * fmodf(ticks / (float)TICK_RATE, 1.0f)) * 0.25f + 0.25f) * 255.0f))) {
		return false;
	}
	
	if (!reset_average) {
		return render_print(frames, "font.fnt", 8.0f, 8.0f, "Ticks: %" PRIu64 "\n\nCurrent tick rate: %.9f\n\nAverage tick rate: %.9f\n\nTest text:\n%s", ticks, tick_rate, average_ticks / (average_duration / (double)NANOTIME_NSEC_PER_SEC), text);
	}
	else {
		reset_average = false;
        average_ticks = 0u;
        average_duration = 0u;
		return render_print(frames, "font.fnt", 8.0f, 8.0f, "Ticks: %" PRIu64 "\n\nCurrent tick rate: %.9f\n\nAverage tick rate: N/A\n\nTest text:\n%s", ticks, tick_rate, text);
	}
}

bool game_deinit(frames_struct* const frames) {
	return render_deinit(frames);
}
