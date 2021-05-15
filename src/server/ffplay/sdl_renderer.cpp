

#include "sdl_renderer.h"

#include "frame_queue.h"
#include "types.h"


// Globals
SDL_AudioDeviceID audio_dev;
SDL_RendererInfo renderer_info = {0};


// Static variables
SDL_Window* SdlRenderer::window = 0;
SDL_Renderer* SdlRenderer::renderer = 0;
SDL_Texture* SdlRenderer::texture = 0;
//SDL_RendererInfo SdlRenderer::renderer_info = {0};
//SDL_AudioDeviceID SdlRenderer::audio_dev;
std::atomic<bool> SdlRenderer::run_events;


bool SdlRenderer::init() {
	int flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
	if (display_disable) { flags &= ~SDL_INIT_VIDEO; }
	if (audio_disable) { flags &= ~SDL_INIT_AUDIO; }
	else {
		// Try to work around an occasional ALSA buffer underflow issue when the
		// period size is NPOT due to ALSA resampling by forcing the buffer size.
		if (!SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE")) {
			SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE","1", 1);
		}
	}
	
	if (SDL_Init(flags)) {
		av_log(NULL, AV_LOG_FATAL, "Could not initialize SDL - %s\n", SDL_GetError());
		av_log(NULL, AV_LOG_FATAL, "(Did you set the DISPLAY variable?)\n");
		return false;
	}

	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

	if (!display_disable) {
		// Set up SDL_Image.
		IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF);
	
		// Create the window and renderer.
		int flags = SDL_WINDOW_HIDDEN;
		if (alwaysontop) {
#if SDL_VERSION_ATLEAST(2,0,5)
			flags |= SDL_WINDOW_ALWAYS_ON_TOP;
#else
			av_log(NULL, AV_LOG_WARNING, "Your SDL version doesn't support SDL_WINDOW_ALWAYS_ON_TOP. Feature will be inactive.\n");
#endif
		}
		
		if (borderless) { flags |= SDL_WINDOW_BORDERLESS; }
		else { flags |= SDL_WINDOW_RESIZABLE; }
		
		// Obtain dimensions of primary display.
		SDL_DisplayMode dm;
		screen_width = dm.w;
		screen_height = dm.h;
		if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
			av_log(NULL, AV_LOG_FATAL, "Couldn't get current display mode: %s\n", SDL_GetError());
			return false;
		}
		
		av_log(NULL, AV_LOG_WARNING, "Creating window with dimensions %dx%d.\n", dm.w, dm.h);
		
		window = SDL_CreateWindow("NymphCast", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
															dm.w, dm.h, flags);
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		if (window) {
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (!renderer) {
				av_log(NULL, AV_LOG_WARNING, "Failed to initialize a hardware accelerated renderer: %s\n", SDL_GetError());
				renderer = SDL_CreateRenderer(window, -1, 0);
			}
			
			if (renderer) {
				if (!SDL_GetRendererInfo(renderer, &renderer_info))
					av_log(NULL, AV_LOG_VERBOSE, "Initialized %s renderer.\n", renderer_info.name);
			}
		}
		
		if (!window || !renderer || !renderer_info.num_texture_formats) {
			av_log(NULL, AV_LOG_FATAL, "Failed to create window or renderer: %s", SDL_GetError());
			return false;
		}
		
		SDL_ShowWindow(window);
	}
	
	return true;
}


// --- QUIT ---
// Clean up SDL resources and terminate SDL.
void SdlRenderer::quit() {
	// Clean up SDL.
	if (!display_disable) {
		av_log(NULL, AV_LOG_FATAL, "Destroying texture...\n");
		texture = 0;
	
		av_log(NULL, AV_LOG_FATAL, "Destroying renderer...\n");
		SDL_DestroyRenderer(renderer);
		renderer = 0;
		
		av_log(NULL, AV_LOG_FATAL, "Destroying window...\n");
		SDL_DestroyWindow(window);
		window = 0;
	}
	
	av_log(NULL, AV_LOG_FATAL, "Quitting...\n");
	
	IMG_Quit();
	SDL_Quit();
}


// -- RESIZE WINDOW ---
void SdlRenderer::resizeWindow(int width, int height) {
	if (!window_title) { window_title = input_filename; }
	SDL_SetWindowTitle(window, window_title);
	
	SDL_SetWindowSize(window, width, height);
	SDL_SetWindowPosition(window, screen_left, screen_top);
	
	if (is_full_screen) {
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	
	SDL_ShowWindow(window);
}


// --- SET FULLSCREEN ---
void SdlRenderer::set_fullscreen(bool fullscreen) {
	SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}


inline void SdlRenderer::fill_rectangle(int x, int y, int w, int h) {
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	if (w && h) { SDL_RenderFillRect(renderer, &rect); }
}


int SdlRenderer::realloc_texture(SDL_Texture **texture, Uint32 new_format, int new_width, 
									int new_height, SDL_BlendMode blendmode, int init_texture) {
	Uint32 format;
	int access, w, h;
	if (!*texture || SDL_QueryTexture(*texture, &format, &access, &w, &h) < 0 || new_width != w || new_height != h || new_format != format) {
		void *pixels;
		int pitch;
		if (*texture){
			av_log(NULL, AV_LOG_INFO, "Destroying texture.\n");
			SDL_DestroyTexture(*texture);
		}
		
		if (!(*texture = SDL_CreateTexture(renderer, new_format, SDL_TEXTUREACCESS_STREAMING, new_width, new_height))) {
			av_log(NULL, AV_LOG_FATAL, "Cannot create SDL texture: %s\n", SDL_GetError());
			
			if (renderer == 0) {
				av_log(NULL, AV_LOG_TRACE, "SDL renderer is 0.\n");
			}
			else {
				av_log(NULL, AV_LOG_TRACE, "SDL renderer is 0.\n");
			}
			
			return -1;
		}
		
		if (SDL_SetTextureBlendMode(*texture, blendmode) < 0) {
			av_log(NULL, AV_LOG_FATAL, "Cannot set SDL texture blend mode: %s\n", SDL_GetError());
			return -1;
		}
		
		if (init_texture) {
			if (SDL_LockTexture(*texture, NULL, &pixels, &pitch) < 0) {
				av_log(NULL, AV_LOG_FATAL, "Cannot lock SDL texture: %s\n", SDL_GetError());
				return -1;
			}
			
			memset(pixels, 0, pitch * new_height);
			SDL_UnlockTexture(*texture);
		}
		
		av_log(NULL, AV_LOG_VERBOSE, "Created %dx%d texture with %s.\n", new_width, new_height, SDL_GetPixelFormatName(new_format));
	}
	return 0;
}


static void calculate_display_rect(SDL_Rect *rect,
								   int scr_xleft, int scr_ytop, int scr_width, int scr_height,
								   int pic_width, int pic_height, AVRational pic_sar)
{
	AVRational aspect_ratio = pic_sar;
	int64_t width, height, x, y;

	if (av_cmp_q(aspect_ratio, av_make_q(0, 1)) <= 0)
		aspect_ratio = av_make_q(1, 1);

	aspect_ratio = av_mul_q(aspect_ratio, av_make_q(pic_width, pic_height));

	/* XXX: we suppose the screen has a 1.0 pixel ratio */
	height = scr_height;
	width = av_rescale(height, aspect_ratio.num, aspect_ratio.den) & ~1;
	if (width > scr_width) {
		width = scr_width;
		height = av_rescale(width, aspect_ratio.den, aspect_ratio.num) & ~1;
	}
	x = (scr_width - width) / 2;
	y = (scr_height - height) / 2;
	rect->x = scr_xleft + x;
	rect->y = scr_ytop  + y;
	rect->w = FFMAX((int)width,  1);
	rect->h = FFMAX((int)height, 1);
}

static void get_sdl_pix_fmt_and_blendmode(int format, Uint32 *sdl_pix_fmt, SDL_BlendMode *sdl_blendmode)
{
	int i;
	*sdl_blendmode = SDL_BLENDMODE_NONE;
	*sdl_pix_fmt = SDL_PIXELFORMAT_UNKNOWN;
	if (format == AV_PIX_FMT_RGB32   ||
		format == AV_PIX_FMT_RGB32_1 ||
		format == AV_PIX_FMT_BGR32   ||
		format == AV_PIX_FMT_BGR32_1)
		*sdl_blendmode = SDL_BLENDMODE_BLEND;
	for (i = 0; i < FF_ARRAY_ELEMS(sdl_texture_format_map) - 1; i++) {
		if (format == sdl_texture_format_map[i].format) {
			*sdl_pix_fmt = sdl_texture_format_map[i].texture_fmt;
			return;
		}
	}
}

int SdlRenderer::upload_texture(SDL_Texture **tex, AVFrame *frame, struct SwsContext **img_convert_ctx) {
	int ret = 0;
	Uint32 sdl_pix_fmt;
	SDL_BlendMode sdl_blendmode;
	get_sdl_pix_fmt_and_blendmode(frame->format, &sdl_pix_fmt, &sdl_blendmode);
	if (realloc_texture(tex, sdl_pix_fmt == SDL_PIXELFORMAT_UNKNOWN ? SDL_PIXELFORMAT_ARGB8888 : sdl_pix_fmt, frame->width, frame->height, sdl_blendmode, 0) < 0)
		return -1;
	
	switch (sdl_pix_fmt) {
		case SDL_PIXELFORMAT_UNKNOWN:
			/* This should only happen if we are not using avfilter... */
			*img_convert_ctx = sws_getCachedContext(*img_convert_ctx,
				frame->width, frame->height, (AVPixelFormat) frame->format, frame->width, frame->height,
				AV_PIX_FMT_BGRA, sws_flags, NULL, NULL, NULL);
			if (*img_convert_ctx != NULL) {
				uint8_t *pixels[4];
				int pitch[4];
				if (!SDL_LockTexture(*tex, NULL, (void **)pixels, pitch)) {
					sws_scale(*img_convert_ctx, (const uint8_t * const *)frame->data, frame->linesize,
							  0, frame->height, pixels, pitch);
					SDL_UnlockTexture(*tex);
				}
			} else {
				av_log(NULL, AV_LOG_FATAL, "Cannot initialize the conversion context\n");
				ret = -1;
			}
			break;
		case SDL_PIXELFORMAT_IYUV:
			if (frame->linesize[0] > 0 && frame->linesize[1] > 0 && frame->linesize[2] > 0) {
				ret = SDL_UpdateYUVTexture(*tex, NULL, frame->data[0], frame->linesize[0],
													   frame->data[1], frame->linesize[1],
													   frame->data[2], frame->linesize[2]);
			} else if (frame->linesize[0] < 0 && frame->linesize[1] < 0 && frame->linesize[2] < 0) {
				ret = SDL_UpdateYUVTexture(*tex, NULL, frame->data[0] + frame->linesize[0] * (frame->height					- 1), -frame->linesize[0],
													   frame->data[1] + frame->linesize[1] * (AV_CEIL_RSHIFT(frame->height, 1) - 1), -frame->linesize[1],
													   frame->data[2] + frame->linesize[2] * (AV_CEIL_RSHIFT(frame->height, 1) - 1), -frame->linesize[2]);
			} else {
				av_log(NULL, AV_LOG_ERROR, "Mixed negative and positive linesizes are not supported.\n");
				return -1;
			}
			break;
		default:
			if (frame->linesize[0] < 0) {
				ret = SDL_UpdateTexture(*tex, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1), -frame->linesize[0]);
			} else {
				ret = SDL_UpdateTexture(*tex, NULL, frame->data[0], frame->linesize[0]);
			}
			break;
	}
	
	return ret;
}


static void set_sdl_yuv_conversion_mode(AVFrame *frame)
{
#if SDL_VERSION_ATLEAST(2,0,8)
	SDL_YUV_CONVERSION_MODE mode = SDL_YUV_CONVERSION_AUTOMATIC;
	if (frame && (frame->format == AV_PIX_FMT_YUV420P || frame->format == AV_PIX_FMT_YUYV422 || frame->format == AV_PIX_FMT_UYVY422)) {
		if (frame->color_range == AVCOL_RANGE_JPEG)
			mode = SDL_YUV_CONVERSION_JPEG;
		else if (frame->colorspace == AVCOL_SPC_BT709)
			mode = SDL_YUV_CONVERSION_BT709;
		else if (frame->colorspace == AVCOL_SPC_BT470BG || frame->colorspace == AVCOL_SPC_SMPTE170M || frame->colorspace == AVCOL_SPC_SMPTE240M)
			mode = SDL_YUV_CONVERSION_BT601;
	}
	SDL_SetYUVConversionMode(mode);
#endif
}


/* display the current picture, if any */
void SdlRenderer::video_display(VideoState *is) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    if (is->audio_st && is->show_mode != SHOW_MODE_VIDEO)
        SdlRenderer::video_audio_display(is);
    else if (is->video_st)
        SdlRenderer::video_image_display(is);
    SDL_RenderPresent(renderer);
}


// --- IMAGE DISPLAY ---
// Create a texture from the file and display it on the screen.
void SdlRenderer::image_display(std::string image) {
	if (texture) { SDL_DestroyTexture(texture); texture = 0; }
	texture = IMG_LoadTexture(renderer, image.data());
	
	/* int w, h;
	SDL_QueryTexture(texture, 0, 0, &w, &h);
	av_log(NULL, AV_LOG_INFO, "Resizing window for texture with w/h: %d, %d.\n", w, h);
	resizeWindow(w, h); */
	
	SDL_RenderCopy(renderer, texture, 0, 0);
	SDL_RenderPresent(renderer);
	
	//SDL_Event event;
	//SDL_PollEvent(&event);
}


void SdlRenderer::run_event_loop() {
	run_events = true;
	SDL_Event event;
	while (run_events) {
		SDL_PollEvent(&event);
		
		// Check for quit events.
		switch (event.type) {
			case SDL_QUIT:
				gCon.signal();
				run_events = false;
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_c && (event.key.keysym.mod & KMOD_CTRL) != 0 ) {
					gCon.signal();
					run_events = false;
					break;
            }
		}
		
		//SDL_Delay(200);
	}
}


void SdlRenderer::stop_event_loop() {
	run_events = false;
}


void SdlRenderer::video_image_display(VideoState *is) {
	Frame *vp;
	Frame *sp = NULL;
	SDL_Rect rect;

	vp = FrameQueueC::frame_queue_peek_last(&is->pictq);
	if (is->subtitle_st) {
		if (FrameQueueC::frame_queue_nb_remaining(&is->subpq) > 0) {
			sp = FrameQueueC::frame_queue_peek(&is->subpq);

			if (vp->pts >= sp->pts + ((float) sp->sub.start_display_time / 1000)) {
				if (!sp->uploaded) {
					uint8_t* pixels[4];
					int pitch[4];
					int i;
					if (!sp->width || !sp->height) {
						sp->width = vp->width;
						sp->height = vp->height;
					}
					if (realloc_texture(&is->sub_texture, SDL_PIXELFORMAT_ARGB8888, sp->width, sp->height, SDL_BLENDMODE_BLEND, 1) < 0)
						return;

					for (i = 0; i < sp->sub.num_rects; i++) {
						AVSubtitleRect *sub_rect = sp->sub.rects[i];

						sub_rect->x = av_clip(sub_rect->x, 0, sp->width );
						sub_rect->y = av_clip(sub_rect->y, 0, sp->height);
						sub_rect->w = av_clip(sub_rect->w, 0, sp->width  - sub_rect->x);
						sub_rect->h = av_clip(sub_rect->h, 0, sp->height - sub_rect->y);

						is->sub_convert_ctx = sws_getCachedContext(is->sub_convert_ctx,
							sub_rect->w, sub_rect->h, AV_PIX_FMT_PAL8,
							sub_rect->w, sub_rect->h, AV_PIX_FMT_BGRA,
							0, NULL, NULL, NULL);
						if (!is->sub_convert_ctx) {
							av_log(NULL, AV_LOG_FATAL, "Cannot initialize the conversion context\n");
							return;
						}
						if (!SDL_LockTexture(is->sub_texture, (SDL_Rect *)sub_rect, (void **)pixels, pitch)) {
							sws_scale(is->sub_convert_ctx, (const uint8_t * const *)sub_rect->data, sub_rect->linesize,
									  0, sub_rect->h, pixels, pitch);
							SDL_UnlockTexture(is->sub_texture);
						}
					}
					sp->uploaded = 1;
				}
			} else
				sp = NULL;
		}
	}

	calculate_display_rect(&rect, is->xleft, is->ytop, is->width, is->height, vp->width, vp->height, vp->sar);

	if (!vp->uploaded) {
		// FIXME: if upload_texture fails, we cannot just continue.
		if (upload_texture(&is->vid_texture, vp->frame, &is->img_convert_ctx) < 0)
			return;
		vp->uploaded = 1;
		vp->flip_v = vp->frame->linesize[0] < 0;
	}

	set_sdl_yuv_conversion_mode(vp->frame);
	SDL_RenderCopyEx(renderer, is->vid_texture, NULL, &rect, 0, NULL, vp->flip_v ? SDL_FLIP_VERTICAL : (SDL_RendererFlip) 0);
	set_sdl_yuv_conversion_mode(NULL);
	if (sp) {
#if USE_ONEPASS_SUBTITLE_RENDER
		SDL_RenderCopy(renderer, is->sub_texture, NULL, &rect);
#else
		int i;
		double xratio = (double)rect.w / (double)sp->width;
		double yratio = (double)rect.h / (double)sp->height;
		for (i = 0; i < sp->sub.num_rects; i++) {
			SDL_Rect *sub_rect = (SDL_Rect*)sp->sub.rects[i];
			SDL_Rect target = {.x = rect.x + sub_rect->x * xratio,
							   .y = rect.y + sub_rect->y * yratio,
							   .w = sub_rect->w * xratio,
							   .h = sub_rect->h * yratio};
			SDL_RenderCopy(renderer, is->sub_texture, sub_rect, &target);
		}
#endif
	}
}

static inline int compute_mod(int a, int b)
{
	return a < 0 ? a%b + b : a%b;
}

void SdlRenderer::video_audio_display(VideoState *s) {
	int i, i_start, x, y1, y, ys, delay, n, nb_display_channels;
	int ch, channels, h, h2;
	int64_t time_diff;
	int rdft_bits, nb_freq;

	for (rdft_bits = 1; (1 << rdft_bits) < 2 * s->height; rdft_bits++)
		;
	nb_freq = 1 << (rdft_bits - 1);

	/* compute display index : center on currently output samples */
	channels = s->audio_tgt.channels;
	nb_display_channels = channels;
	if (!s->paused) {
		int data_used= s->show_mode == SHOW_MODE_WAVES ? s->width : (2*nb_freq);
		n = 2 * channels;
		delay = s->audio_write_buf_size;
		delay /= n;

		/* to be more precise, we take into account the time spent since
		   the last buffer computation */
		if (audio_callback_time) {
			time_diff = av_gettime_relative() - audio_callback_time;
			delay -= (time_diff * s->audio_tgt.freq) / 1000000;
		}

		delay += 2 * data_used;
		if (delay < data_used)
			delay = data_used;

		i_start= x = compute_mod(s->sample_array_index - delay * channels, SAMPLE_ARRAY_SIZE);
		if (s->show_mode == SHOW_MODE_WAVES) {
			h = INT_MIN;
			for (i = 0; i < 1000; i += channels) {
				int idx = (SAMPLE_ARRAY_SIZE + x - i) % SAMPLE_ARRAY_SIZE;
				int a = s->sample_array[idx];
				int b = s->sample_array[(idx + 4 * channels) % SAMPLE_ARRAY_SIZE];
				int c = s->sample_array[(idx + 5 * channels) % SAMPLE_ARRAY_SIZE];
				int d = s->sample_array[(idx + 9 * channels) % SAMPLE_ARRAY_SIZE];
				int score = a - d;
				if (h < score && (b ^ c) < 0) {
					h = score;
					i_start = idx;
				}
			}
		}

		s->last_i_start = i_start;
	} else {
		i_start = s->last_i_start;
	}

	if (s->show_mode == SHOW_MODE_WAVES) {
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

		/* total height for one channel */
		h = s->height / nb_display_channels;
		/* graph height / 2 */
		h2 = (h * 9) / 20;
		for (ch = 0; ch < nb_display_channels; ch++) {
			i = i_start + ch;
			y1 = s->ytop + ch * h + (h / 2); /* position of center line */
			for (x = 0; x < s->width; x++) {
				y = (s->sample_array[i] * h2) >> 15;
				if (y < 0) {
					y = -y;
					ys = y1 - y;
				} else {
					ys = y1;
				}
				fill_rectangle(s->xleft + x, ys, 1, y);
				i += channels;
				if (i >= SAMPLE_ARRAY_SIZE)
					i -= SAMPLE_ARRAY_SIZE;
			}
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

		for (ch = 1; ch < nb_display_channels; ch++) {
			y = s->ytop + ch * h;
			fill_rectangle(s->xleft, y, s->width, 1);
		}
	} else {
		if (realloc_texture(&s->vis_texture, SDL_PIXELFORMAT_ARGB8888, s->width, s->height, SDL_BLENDMODE_NONE, 1) < 0)
			return;

		nb_display_channels= FFMIN(nb_display_channels, 2);
		if (rdft_bits != s->rdft_bits) {
			av_rdft_end(s->rdft);
			av_free(s->rdft_data);
			s->rdft = av_rdft_init(rdft_bits, DFT_R2C);
			s->rdft_bits = rdft_bits;
			s->rdft_data = (FFTSample*) av_malloc_array(nb_freq, 4 *sizeof(*s->rdft_data));
		}
		if (!s->rdft || !s->rdft_data){
			av_log(NULL, AV_LOG_ERROR, "Failed to allocate buffers for RDFT, switching to waves display\n");
			s->show_mode = SHOW_MODE_WAVES;
		} else {
			FFTSample *data[2];
			SDL_Rect rect = {.x = s->xpos, .y = 0, .w = 1, .h = s->height};
			uint32_t *pixels;
			int pitch;
			for (ch = 0; ch < nb_display_channels; ch++) {
				data[ch] = s->rdft_data + 2 * nb_freq * ch;
				i = i_start + ch;
				for (x = 0; x < 2 * nb_freq; x++) {
					double w = (x-nb_freq) * (1.0 / nb_freq);
					data[ch][x] = s->sample_array[i] * (1.0 - w * w);
					i += channels;
					if (i >= SAMPLE_ARRAY_SIZE)
						i -= SAMPLE_ARRAY_SIZE;
				}
				av_rdft_calc(s->rdft, data[ch]);
			}
			/* Least efficient way to do this, we should of course
			 * directly access it but it is more than fast enough. */
			if (!SDL_LockTexture(s->vis_texture, &rect, (void **)&pixels, &pitch)) {
				pitch >>= 2;
				pixels += pitch * s->height;
				for (y = 0; y < s->height; y++) {
					double w = 1 / sqrt(nb_freq);
					int a = sqrt(w * sqrt(data[0][2 * y + 0] * data[0][2 * y + 0] + data[0][2 * y + 1] * data[0][2 * y + 1]));
					int b = (nb_display_channels == 2 ) ? sqrt(w * hypot(data[1][2 * y + 0], data[1][2 * y + 1]))
														: a;
					a = FFMIN(a, 255);
					b = FFMIN(b, 255);
					pixels -= pitch;
					*pixels = (a << 16) + (b << 8) + ((a+b) >> 1);
				}
				SDL_UnlockTexture(s->vis_texture);
			}
			SDL_RenderCopy(renderer, s->vis_texture, NULL, NULL);
		}
		if (!s->paused)
			s->xpos++;
		if (s->xpos >= s->width)
			s->xpos= s->xleft;
	}
}


void SdlRenderer::set_default_window_size(int width, int height, AVRational sar) {
	SDL_Rect rect;
	int max_width  = screen_width  ? screen_width  : INT_MAX;
	int max_height = screen_height ? screen_height : INT_MAX;
	if (max_width == INT_MAX && max_height == INT_MAX)
		max_height = height;
	calculate_display_rect(&rect, 0, 0, max_width, max_height, width, height, sar);
	default_width  = rect.w;
	default_height = rect.h;
}

