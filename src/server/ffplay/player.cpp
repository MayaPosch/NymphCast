

#include "player.h"

#include "stream_handler.h"
#include "clock.h"
#include "video_renderer.h"
#include "frame_queue.h"
#include "sdl_renderer.h"

// Enable profiling.
//#define PROFILING 1
#ifdef PROFILING
#include <chrono>
#include <fstream>
std::ofstream debugfile;
uint32_t profcount = 0;
#endif


// Static initialisations.
std::atomic_bool Player::run;
VideoState* Player::cur_stream = 0;
double Player::remaining_time = 0.0;


// --- CONSTRUCTOR ---
Player::Player() {
	//
}


bool Player::start(Config config) {
	//
	
	return true;
}


// --- SET VIDEO STATE ---
void Player::setVideoState(VideoState* vs) {
	cur_stream = vs;
}


static void toggle_pause(VideoState *is)
{
    StreamHandler::stream_toggle_pause(is);
    is->step = 0;
}

static void toggle_mute(VideoState *is)
{
    is->muted = !is->muted;
}


static void toggle_full_screen(VideoState *is) {
    is_full_screen = !is_full_screen;
    SdlRenderer::set_fullscreen(is_full_screen);
}

static void toggle_audio_display(VideoState *is)
{
    int next = is->show_mode;
    do {
        next = (next + 1) % SHOW_MODE_NB;
    } while (next != is->show_mode && (next == SHOW_MODE_VIDEO && !is->video_st || next != SHOW_MODE_VIDEO && !is->audio_st));
    if (is->show_mode != next) {
        is->force_refresh = 1;
        is->show_mode = (ShowMode) next;
    }
}

static void update_volume(VideoState *is, int sign, double step)
{
    double volume_level = is->audio_volume ? (20 * log(is->audio_volume / (double)SDL_MIX_MAXVOLUME) / log(10)) : -1000.0;
    int new_volume = lrint(SDL_MIX_MAXVOLUME * pow(10.0, (volume_level + sign * step) / 20.0));
    is->audio_volume = av_clip(is->audio_volume == new_volume ? (is->audio_volume + sign) : new_volume, 0, SDL_MIX_MAXVOLUME);
}

static void refresh_loop_wait_event(VideoState *is, SDL_Event *event) {
    double remaining_time = 0.0;
    SDL_PumpEvents();
    while (!SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) {
        if (!cursor_hidden && av_gettime_relative() - cursor_last_shown > CURSOR_HIDE_DELAY) {
            SDL_ShowCursor(0);
            cursor_hidden = 1;
        }
        if (remaining_time > 0.0)
            av_usleep((int64_t)(remaining_time * 1000000.0));
        remaining_time = REFRESH_RATE;
        if (is->show_mode != SHOW_MODE_NONE && (!is->paused || is->force_refresh))
            VideoRenderer::video_refresh(is, &remaining_time);
        SDL_PumpEvents();
    }
}


void Player::refresh_loop(VideoState* is) {
	av_log(NULL, AV_LOG_INFO, "Entering video refresh loop.\n");
	bool run = true;
	while (run) {
        if (!cursor_hidden && av_gettime_relative() - cursor_last_shown > CURSOR_HIDE_DELAY) {
            SDL_ShowCursor(0);
            cursor_hidden = 1;
        }
		
        if (remaining_time > 0.0)
            av_usleep((int64_t)(remaining_time * 1000000.0));
        remaining_time = REFRESH_RATE;
        if (is->show_mode != SHOW_MODE_NONE && (!is->paused || is->force_refresh))
            VideoRenderer::video_refresh(is, &remaining_time);
    }
	
	av_log(NULL, AV_LOG_INFO, "Leaving video refresh loop.\n");
}


static void seek_chapter(VideoState *is, int incr)
{
    int64_t pos = ClockC::get_master_clock(is) * AV_TIME_BASE;
    int i;

    if (!is->ic->nb_chapters)
        return;

    /* find the current chapter */
    for (i = 0; i < is->ic->nb_chapters; i++) {
        AVChapter *ch = is->ic->chapters[i];
        if (av_compare_ts(pos, AV_TIME_BASE_Q, ch->start, ch->time_base) < 0) {
            i--;
            break;
        }
    }

    i += incr;
    i = FFMAX(i, 0);
    if (i >= is->ic->nb_chapters)
        return;

    av_log(NULL, AV_LOG_VERBOSE, "Seeking to chapter %d.\n", i);
    StreamHandler::stream_seek(is, av_rescale_q(is->ic->chapters[i]->start, is->ic->chapters[i]->time_base,
                                 AV_TIME_BASE_Q), 0, 0);
}


/* handle an event sent by the GUI */
void Player::event_loop(VideoState *cur_stream) {
    SDL_Event event;
    double incr, pos, frac;
	
	av_log(NULL, AV_LOG_INFO, "Entering event loop.\n");

	bool run = true;
    while (run) {
        double x;
        refresh_loop_wait_event(cur_stream, &event);
        switch (event.type) {
        case SDL_KEYDOWN:
            if (exit_on_keydown || event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q) {
                //do_exit(cur_stream);
				
				av_log(NULL, AV_LOG_INFO, "Exiting event loop...\n");
				
				run = false;
				continue;
                break;
            }
			
            // If we don't yet have a window, skip all key events, because read_thread might still be initializing...
			// FIXME: disabling this check as it disables interaction during audio-only (no window) 
			// playback, where features like pause are still needed.
            /* if (!cur_stream->width) {
				av_log(NULL, AV_LOG_INFO, "We don't have a window yet. Skip key events.\n");
                continue;
			} */
			
            switch (event.key.keysym.sym) {
				case SDLK_f:
					toggle_full_screen(cur_stream);
					cur_stream->force_refresh = 1;
					break;
				case SDLK_MINUS:
					SdlRenderer::setShowWindow(true); // Show window.
					break;
				case SDLK_UNDERSCORE:
					SdlRenderer::setShowWindow(false); // Set hide window.
					break;
				case SDLK_p:
				case SDLK_SPACE:
					toggle_pause(cur_stream);
					break;
				case SDLK_m:
					toggle_mute(cur_stream);
					break;
				case SDLK_KP_MULTIPLY:
				case SDLK_0:
					update_volume(cur_stream, 1, SDL_VOLUME_STEP);
					break;
				case SDLK_KP_DIVIDE:
				case SDLK_9:
					update_volume(cur_stream, -1, SDL_VOLUME_STEP);
					break;
				case SDLK_s: // S: Step to next frame
					StreamHandler::step_to_next_frame(cur_stream);
					break;
				case SDLK_a:
					StreamHandler::stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
					break;
				case SDLK_v:
					StreamHandler::stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
					break;
				case SDLK_c:
					StreamHandler::stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
					StreamHandler::stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
					StreamHandler::stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
					break;
				case SDLK_t:
					StreamHandler::stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
					break;
				case SDLK_w:
	#if CONFIG_AVFILTER
					if (cur_stream->show_mode == SHOW_MODE_VIDEO && cur_stream->vfilter_idx < nb_vfilters - 1) {
						if (++cur_stream->vfilter_idx >= nb_vfilters)
							cur_stream->vfilter_idx = 0;
					} else {
						cur_stream->vfilter_idx = 0;
						toggle_audio_display(cur_stream);
					}
	#else
					toggle_audio_display(cur_stream);
	#endif
					break;
				case SDLK_PAGEUP:
					if (cur_stream->ic->nb_chapters <= 1) {
						incr = 600.0;
						goto do_seek;
					}
					seek_chapter(cur_stream, 1);
					break;
				case SDLK_PAGEDOWN:
					if (cur_stream->ic->nb_chapters <= 1) {
						incr = -600.0;
						goto do_seek;
					}
					seek_chapter(cur_stream, -1);
					break;
				case SDLK_LEFT:
					incr = seek_interval ? -seek_interval : -10.0;
					goto do_seek;
				case SDLK_RIGHT:
					incr = seek_interval ? seek_interval : 10.0;
					goto do_seek;
				case SDLK_UP:
					incr = 60.0;
					goto do_seek;
				case SDLK_DOWN:
					incr = -60.0;
				do_seek:
						if (seek_by_bytes) {
							av_log(NULL, AV_LOG_INFO, "Seek by bytes...\n");
							pos = -1;
							if (pos < 0 && cur_stream->video_stream >= 0)
								pos = FrameQueueC::frame_queue_last_pos(&cur_stream->pictq);
							if (pos < 0 && cur_stream->audio_stream >= 0)
								pos = FrameQueueC::frame_queue_last_pos(&cur_stream->sampq);
							if (pos < 0)
								pos = avio_tell(cur_stream->ic->pb);
							if (cur_stream->ic->bit_rate)
								incr *= cur_stream->ic->bit_rate / 8.0;
							else
								incr *= 180000.0;
							pos += incr;
							StreamHandler::stream_seek(cur_stream, pos, incr, 1);
						} 
						else {
							av_log(NULL, AV_LOG_INFO, "Seek by time...\n");
							pos = ClockC::get_master_clock(cur_stream);
							if (isnan(pos))
								pos = (double)cur_stream->seek_pos / AV_TIME_BASE;
							pos += incr;
							if (cur_stream->ic->start_time != AV_NOPTS_VALUE && pos < cur_stream->ic->start_time / (double)AV_TIME_BASE)
								pos = cur_stream->ic->start_time / (double)AV_TIME_BASE;
							StreamHandler::stream_seek(cur_stream, (int64_t)(pos * AV_TIME_BASE), (int64_t)(incr * AV_TIME_BASE), 0);
						}
					break;
				default:
					break;
				}
				break;
        case SDL_MOUSEBUTTONDOWN:
            if (exit_on_mousedown) {
                //do_exit(cur_stream);
				av_log(NULL, AV_LOG_DEBUG, "Exiting event loop due to mouse...\n");
				run = false;
				continue;
                break;
            }
            if (event.button.button == SDL_BUTTON_LEFT) {
                static int64_t last_mouse_left_click = 0;
                if (av_gettime_relative() - last_mouse_left_click <= 500000) {
                    toggle_full_screen(cur_stream);
                    cur_stream->force_refresh = 1;
                    last_mouse_left_click = 0;
                } else {
                    last_mouse_left_click = av_gettime_relative();
                }
            }
        case SDL_MOUSEMOTION:
            if (cursor_hidden) {
                SDL_ShowCursor(1);
                cursor_hidden = 0;
            }
			
            cursor_last_shown = av_gettime_relative();
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button != SDL_BUTTON_RIGHT) { break; }
                x = event.button.x;
            } 
			else {
                if (!(event.motion.state & SDL_BUTTON_RMASK)) { break; }
                x = event.motion.x;
            }
			
			if (seek_by_bytes || cur_stream->ic->duration <= 0) {
				uint64_t size =  avio_size(cur_stream->ic->pb);
				StreamHandler::stream_seek(cur_stream, size*x/cur_stream->width, 0, 1);
			} 
			else {
				int64_t ts;
				int ns, hh, mm, ss;
				int tns, thh, tmm, tss;
				tns  = cur_stream->ic->duration / 1000000LL;
				thh  = tns / 3600;
				tmm  = (tns % 3600) / 60;
				tss  = (tns % 60);
				frac = x / cur_stream->width;
				ns   = frac * tns;
				hh   = ns / 3600;
				mm   = (ns % 3600) / 60;
				ss   = (ns % 60);
				av_log(NULL, AV_LOG_INFO,
					   "Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n", frac*100,
						hh, mm, ss, thh, tmm, tss);
				ts = frac * cur_stream->ic->duration;
				if (cur_stream->ic->start_time != AV_NOPTS_VALUE)
					ts += cur_stream->ic->start_time;
				StreamHandler::stream_seek(cur_stream, ts, 0, 0);
			}
			
            break;
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    screen_width  = cur_stream->width  = event.window.data1;
                    screen_height = cur_stream->height = event.window.data2;
                    if (cur_stream->vis_texture) {
                        SDL_DestroyTexture(cur_stream->vis_texture);
                        cur_stream->vis_texture = NULL;
                    }
                case SDL_WINDOWEVENT_EXPOSED:
                    cur_stream->force_refresh = 1;
            }
            break;
		
        case SDL_QUIT:
        case FF_QUIT_EVENT:
            //do_exit(cur_stream);
				av_log(NULL, AV_LOG_INFO, "Exiting event loop due to quit event...\n");
			run = false;
			continue;
            break;
        default:
            break;
        }
		
		if (event.type == nymph_seek_event) {
			// Seek to the desired location.
			int64_t ts;
			int ns, hh, mm, ss;
			int tns, thh, tmm, tss;
			tns  = cur_stream->ic->duration / 1000000LL;
			thh  = tns / 3600;
			tmm  = (tns % 3600) / 60;
			tss  = (tns % 60);
			frac = event.user.code / 100.0;
			ns   = frac * tns;
			hh   = ns / 3600;
			mm   = (ns % 3600) / 60;
			ss   = (ns % 60);
			av_log(NULL, AV_LOG_INFO,
				   "Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n", frac*100,
					hh, mm, ss, thh, tmm, tss);
			ts = frac * cur_stream->ic->duration;
			if (cur_stream->ic->start_time != AV_NOPTS_VALUE)
				ts += cur_stream->ic->start_time;
			StreamHandler::stream_seek(cur_stream, ts, 0, 0);
		}			
    }
}


// --- RUN UPDATES ---
void Player::run_updates() {
	if (!cursor_hidden && av_gettime_relative() - cursor_last_shown > CURSOR_HIDE_DELAY) {
		SDL_ShowCursor(0);
		cursor_hidden = 1;
	}
	
#ifdef PROFILING
	if (!debugfile.is_open()) {
		av_log(NULL, AV_LOG_WARNING, "Start profiling...\n");
		debugfile.open("profiling.txt");
	}
	
	debugfile << profcount++ << "\tRemaining: " << remaining_time << "\t";
	std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();
#endif
	
	if (remaining_time > 0.0) {
		av_usleep((int64_t)(remaining_time * 1000000.0));
	}
	
	remaining_time = REFRESH_RATE;
	if (cur_stream->show_mode != SHOW_MODE_NONE && (!cur_stream->paused || cur_stream->force_refresh)) {
		VideoRenderer::video_refresh(cur_stream, &remaining_time);
	}

#ifdef PROFILING
	std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
	debugfile << "Duration: " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "µs.\n";
#endif

}


// --- PROCESS EVENT ---
bool Player::process_event(SDL_Event &event) {
#ifdef PROFILING
	debugfile << "Player::process_event called.\n";
#endif
	double incr, pos, frac;
	double x;
	switch (event.type) {
	case SDL_KEYDOWN:
		//av_log(NULL, AV_LOG_DEBUG, "Processing keydown event...");
		if (exit_on_keydown || event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q) {
			//do_exit(cur_stream);
			//av_log(NULL, AV_LOG_DEBUG, "Processing ESC/q event...");
			
			av_log(NULL, AV_LOG_INFO, "Exiting event loop...\n");
			
			// Signal the player thread that the playback has ended.
			StreamHandler::quit();
			
			break;
		}
		
		// If we don't yet have a window, skip all key events, because read_thread might still be initializing...
		// FIXME: disabling this check as it disables interaction during audio-only (no window) 
		// playback, where features like pause are still needed.
		/* if (!cur_stream->width) {
			av_log(NULL, AV_LOG_INFO, "We don't have a window yet. Skip key events.\n");
			continue;
		} */
		
		switch (event.key.keysym.sym) {
			case SDLK_f:
				toggle_full_screen(cur_stream);
				cur_stream->force_refresh = 1;
				break;
			case SDLK_p:
			case SDLK_SPACE:
				toggle_pause(cur_stream);
				break;
			case SDLK_m:
				toggle_mute(cur_stream);
				break;
			case SDLK_KP_MULTIPLY:
			case SDLK_0:
				update_volume(cur_stream, 1, SDL_VOLUME_STEP);
				break;
			case SDLK_KP_DIVIDE:
			case SDLK_9:
				update_volume(cur_stream, -1, SDL_VOLUME_STEP);
				break;
			case SDLK_s: // S: Step to next frame
				StreamHandler::step_to_next_frame(cur_stream);
				break;
			case SDLK_a:
				StreamHandler::stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
				break;
			case SDLK_v:
				StreamHandler::stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
				break;
			case SDLK_c:
				StreamHandler::stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
				StreamHandler::stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
				StreamHandler::stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
				break;
			case SDLK_t:
				StreamHandler::stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
				break;
			case SDLK_w:
#if CONFIG_AVFILTER
				if (cur_stream->show_mode == SHOW_MODE_VIDEO && cur_stream->vfilter_idx < nb_vfilters - 1) {
					if (++cur_stream->vfilter_idx >= nb_vfilters)
						cur_stream->vfilter_idx = 0;
				} else {
					cur_stream->vfilter_idx = 0;
					toggle_audio_display(cur_stream);
				}
#else
				toggle_audio_display(cur_stream);
#endif
				break;
			case SDLK_PAGEUP:
				if (cur_stream->ic->nb_chapters <= 1) {
					incr = 600.0;
					goto do_seek;
				}
				seek_chapter(cur_stream, 1);
				break;
			case SDLK_PAGEDOWN:
				if (cur_stream->ic->nb_chapters <= 1) {
					incr = -600.0;
					goto do_seek;
				}
				seek_chapter(cur_stream, -1);
				break;
			case SDLK_LEFT:
				incr = seek_interval ? -seek_interval : -10.0;
				goto do_seek;
			case SDLK_RIGHT:
				incr = seek_interval ? seek_interval : 10.0;
				goto do_seek;
			case SDLK_UP:
				incr = 60.0;
				goto do_seek;
			case SDLK_DOWN:
				incr = -60.0;
			do_seek:
					if (seek_by_bytes) {
						av_log(NULL, AV_LOG_INFO, "Seek by bytes...\n");
						pos = -1;
						if (pos < 0 && cur_stream->video_stream >= 0)
							pos = FrameQueueC::frame_queue_last_pos(&cur_stream->pictq);
						if (pos < 0 && cur_stream->audio_stream >= 0)
							pos = FrameQueueC::frame_queue_last_pos(&cur_stream->sampq);
						if (pos < 0)
							pos = avio_tell(cur_stream->ic->pb);
						if (cur_stream->ic->bit_rate)
							incr *= cur_stream->ic->bit_rate / 8.0;
						else
							incr *= 180000.0;
						pos += incr;
						StreamHandler::stream_seek(cur_stream, pos, incr, 1);
					} 
					else {
						av_log(NULL, AV_LOG_INFO, "Seek by time...\n");
						pos = ClockC::get_master_clock(cur_stream);
						if (isnan(pos))
							pos = (double)cur_stream->seek_pos / AV_TIME_BASE;
						pos += incr;
						if (cur_stream->ic->start_time != AV_NOPTS_VALUE && pos < cur_stream->ic->start_time / (double)AV_TIME_BASE)
							pos = cur_stream->ic->start_time / (double)AV_TIME_BASE;
						StreamHandler::stream_seek(cur_stream, (int64_t)(pos * AV_TIME_BASE), (int64_t)(incr * AV_TIME_BASE), 0);
					}
				break;
			default:
				break;
			}
			break;
	case SDL_MOUSEBUTTONDOWN:
		if (exit_on_mousedown) {
			//do_exit(cur_stream);
			av_log(NULL, AV_LOG_DEBUG, "Exiting event loop due to mouse...\n");
			
			// Signal the player thread that the playback has ended.
			StreamHandler::quit();
			break;
		}
		if (event.button.button == SDL_BUTTON_LEFT) {
			static int64_t last_mouse_left_click = 0;
			if (av_gettime_relative() - last_mouse_left_click <= 500000) {
				toggle_full_screen(cur_stream);
				cur_stream->force_refresh = 1;
				last_mouse_left_click = 0;
			} else {
				last_mouse_left_click = av_gettime_relative();
			}
		}
	case SDL_MOUSEMOTION:
		if (cursor_hidden) {
			SDL_ShowCursor(1);
			cursor_hidden = 0;
		}
		
		cursor_last_shown = av_gettime_relative();
		if (event.type == SDL_MOUSEBUTTONDOWN) {
			if (event.button.button != SDL_BUTTON_RIGHT) { break; }
			x = event.button.x;
		} 
		else {
			if (!(event.motion.state & SDL_BUTTON_RMASK)) { break; }
			x = event.motion.x;
		}
		
		if (seek_by_bytes || cur_stream->ic->duration <= 0) {
			uint64_t size =  avio_size(cur_stream->ic->pb);
			StreamHandler::stream_seek(cur_stream, size*x/cur_stream->width, 0, 1);
		} 
		else {
			int64_t ts;
			int ns, hh, mm, ss;
			int tns, thh, tmm, tss;
			tns  = cur_stream->ic->duration / 1000000LL;
			thh  = tns / 3600;
			tmm  = (tns % 3600) / 60;
			tss  = (tns % 60);
			frac = x / cur_stream->width;
			ns   = frac * tns;
			hh   = ns / 3600;
			mm   = (ns % 3600) / 60;
			ss   = (ns % 60);
			av_log(NULL, AV_LOG_INFO,
				   "Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n", frac*100,
					hh, mm, ss, thh, tmm, tss);
			ts = frac * cur_stream->ic->duration;
			if (cur_stream->ic->start_time != AV_NOPTS_VALUE)
				ts += cur_stream->ic->start_time;
			StreamHandler::stream_seek(cur_stream, ts, 0, 0);
		}
		
		break;
	case SDL_WINDOWEVENT:
		switch (event.window.event) {
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				screen_width  = cur_stream->width  = event.window.data1;
				screen_height = cur_stream->height = event.window.data2;
				if (cur_stream->vis_texture) {
					SDL_DestroyTexture(cur_stream->vis_texture);
					cur_stream->vis_texture = NULL;
				}
			case SDL_WINDOWEVENT_EXPOSED:
				cur_stream->force_refresh = 1;
		}
		break;
	
	case SDL_QUIT:
	case FF_QUIT_EVENT:
		//do_exit(cur_stream);
			av_log(NULL, AV_LOG_INFO, "Exiting event loop due to quit event...\n");
			
			// Signal the player thread that the playback has ended.
			StreamHandler::quit();
		break;
	default:
		break;
	}
	
	if (event.type == nymph_seek_event) {
		// Seek to the desired location.
		int64_t ts;
		int ns, hh, mm, ss;
		int tns, thh, tmm, tss;
		tns  = cur_stream->ic->duration / 1000000LL;
		thh  = tns / 3600;
		tmm  = (tns % 3600) / 60;
		tss  = (tns % 60);
		frac = event.user.code / 100.0;
		ns   = frac * tns;
		hh   = ns / 3600;
		mm   = (ns % 3600) / 60;
		ss   = (ns % 60);
		av_log(NULL, AV_LOG_INFO,
			   "Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n", frac*100,
				hh, mm, ss, thh, tmm, tss);
		ts = frac * cur_stream->ic->duration;
		if (cur_stream->ic->start_time != AV_NOPTS_VALUE)
			ts += cur_stream->ic->start_time;
		StreamHandler::stream_seek(cur_stream, ts, 0, 0);
	}
	
	return true;
}


// --- QUIT ---
void Player::quit() {
	run = false;
	
#ifdef PROFILING
	if (debugfile.is_open()) {
		debugfile.flush();
		debugfile.close();
	}
#endif
}

