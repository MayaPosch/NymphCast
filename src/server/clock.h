

#ifndef CLOCK_H
#define CLOCK_H


#include "types.h"


class ClockC {
	//
	
public:
	static double get_clock(Clock *c);
	static void set_clock_at(Clock *c, double pts, int serial, double time);
	static void set_clock(Clock *c, double pts, int serial);
	static void set_clock_speed(Clock *c, double speed);
	static void init_clock(Clock *c, int *queue_serial);
	static void sync_clock_to_slave(Clock *c, Clock *slave);
	static int get_master_sync_type(VideoState *is);
	static double get_master_clock(VideoState *is);
	static void check_external_clock_speed(VideoState *is);
	static double compute_target_delay(double delay, VideoState *is);
};

#endif
