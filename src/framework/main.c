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

#include "framework/app.h"
#include "framework/nanotime.h"
#include "framework/defs.h"
#include "game/game.h"
#include "SDL.h"
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>

static bool quit_now;

static int refresh_rate_get() {
	SDL_Window* const window = app_window_get();
	const int display_index = SDL_GetWindowDisplayIndex(window);
	if (display_index < 0) {
		fprintf(stderr, "Error: %s\n", SDL_GetError());
		fflush(stderr);
		return -1;
	}
	SDL_DisplayMode display_mode;
	if (SDL_GetDesktopDisplayMode(display_index, &display_mode) < 0) {
		fprintf(stderr, "Error: %s\n", SDL_GetError());
		fflush(stderr);
		return -1;
	}
	return display_mode.refresh_rate;
}

static int SDLCALL event_filter(void* userdata, SDL_Event* event) {
	if (event->type == SDL_QUIT) {
		quit_now = true;
	}
	return 1;
}

int main(int argc, char** argv) {
	int exit_code = EXIT_SUCCESS;

	if (!app_init(argc, argv)) {
		return EXIT_FAILURE;
	}
	
	SDL_GLContext context = app_context_create();
	if (context == NULL) {
		app_deinit();
		return EXIT_FAILURE;
	}

	frames_status_enum frames_status;
	frames_struct* const frames = frames_create();
	if (frames == NULL) {
		app_context_destroy(context);
		app_deinit();
		return EXIT_FAILURE;
	}

	frames_status = frames_start(frames);
	if (frames_status == FRAMES_STATUS_ERROR) {
		if (!frames_destroy(frames)) {
			fprintf(stderr, "Failed destroying the frames object\n");
			fflush(stderr);
		}
		app_context_destroy(context);
		app_deinit();
		return EXIT_FAILURE;
	}

	uint64_t tick_rate;
	if (!game_init(frames, &tick_rate)) {
		fflush(stderr);
		frames_destroy(frames);
		app_context_destroy(context);
		app_deinit();
		return EXIT_FAILURE;
	}
	
	frames_end(frames);

	nanotime_step_data stepper;
	nanotime_step_init(&stepper, NANOTIME_NSEC_PER_SEC / tick_rate, nanotime_now_max(), nanotime_now, nanotime_sleep);
	uint64_t last_render = nanotime_now();
	
	quit_now = false;
	while (SDL_PumpEvents(), SDL_FilterEvents(event_filter, NULL), !quit_now) {
		frames_status = frames_start(frames);
		if (frames_status == FRAMES_STATUS_ERROR) {
			exit_code = EXIT_FAILURE;
			break;
		}

		if (!game_update(frames)) {
			fflush(stdout);
			exit_code = EXIT_FAILURE;
			break;
		}
		fflush(stdout);
		
		frames_end(frames);

		int refresh_rate = refresh_rate_get();
		if (refresh_rate < 0) {
			exit_code = EXIT_FAILURE;
			break;
		}
		if (refresh_rate == 0) {
			refresh_rate = 60;
		}
		
		const uint64_t frame_duration = NANOTIME_NSEC_PER_SEC / refresh_rate;
		const uint64_t now = nanotime_now();
		if (now >= last_render + frame_duration) {
			frames_status = frames_draw_latest(frames);
			if (frames_status == FRAMES_STATUS_ERROR) {
				exit_code = EXIT_FAILURE;
				break;
			}
			last_render = now + frame_duration;
		}

		static uint64_t skips = 0u;
		if (!nanotime_step(&stepper)) {
			skips++;
			printf("skips == %" PRIu64 "\n", skips);
			fflush(stdout);
		}
	}

	frames_status = frames_start(frames);
	if (frames_status != FRAMES_STATUS_ERROR) {
		if (!game_deinit(frames)) {
			fprintf(stderr, "Error deinitializing the game\n");
			fflush(stderr);
		}
		frames_end(frames);
	}
	if (!frames_destroy(frames)) {
		fprintf(stderr, "Error destroying the frames object\n");
		fflush(stderr);
	}
	app_context_destroy(context);
	app_deinit();
	return exit_code;
}
