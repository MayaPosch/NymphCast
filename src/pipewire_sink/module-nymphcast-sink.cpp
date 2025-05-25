/*
	module-nymphcast-sink.cpp - PipeWire sink module for NymphCast.
*/


#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <math.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <spa/utils/cleanup.h>
#include <spa/utils/result.h>
#include <spa/utils/string.h>
#include <spa/utils/json.h>
#include <spa/utils/ringbuffer.h>
#include <spa/debug/types.h>
#include <spa/pod/builder.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/audio/raw.h>
#include <spa/param/latency-utils.h>

#include <pipewire/impl.h>
#include <pipewire/i18n.h>

#include <nymphcast_client.h>


#define NAME "nymphcast-sink"


// TODO: expand MODULE_USAGE.
#define MODULE_USAGE	"( nc.ip=<ip address of host> ) "	\
			"( nc.port=<remote port> ) "

static const struct spa_dict_item module_props[] = {
	{ PW_KEY_MODULE_AUTHOR, "Maya Posch" },
	{ PW_KEY_MODULE_DESCRIPTION, "A NymphCast audio sink." },
	{ PW_KEY_MODULE_USAGE, MODULE_USAGE },
	{ PW_KEY_MODULE_VERSION, PACKAGE_VERSION },
};


struct impl {
	struct pw_context* context;

	struct pw_properties* props;

	struct pw_impl_module* module;

	struct spa_hook module_listener;

	struct pw_core* core;
	struct spa_hook core_proxy_listener;
	struct spa_hook core_listener;

	struct pw_properties* stream_props;
	struct pw_stream* stream;
	struct spa_hook stream_listener;
	struct spa_audio_info_raw info;
	uint32_t frame_size;

	unsigned int do_disconnect:1;
};


SPA_EXPORT int pipewire__module_init(struct pw_impl_module* module, const char* args) {
	struct pw_context* context = pw_impl_module_get_context(module);
	struct pw_properties* props = NULL;
	struct impl* impl;
	//const char* str, *name, *hostname, *ip, *port;
	int res = 0;

	PW_LOG_TOPIC_INIT(mod_topic);

	impl = calloc(1, sizeof(struct impl));
	if (impl == NULL) {
		return -errno;
	}

	pw_log_debug("module %p: new %s", impl, args);
	
	// Obtain the IP address & port string from the 'args' string.
	// Format is <ip>:<port>, where the address is in IPv4 string format. Port is an integer.
	std::string argstr = std::string(args);
	std::size_t pos = argstr.find_last_of(':');
	std::string ip = argstr.substr(0, pos);
	uint32_t port = (uint32_t) std::stoi(argstr.substr(pos + 1));
	
	// TODO: Set up the audio stuff?
	
}
