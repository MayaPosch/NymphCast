/*
	module-nymphcast-discover.cpp - Discover PipeWire module for NymphCast instances.
	
	Features:
			- Discovers local NymphCast receivers on the network and creates a NymphCast sink
				for each discovered receiver.
	
	Notes:
			- 
			
	2024/03/01 - Maya Posch
*/


#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "config.h"

#include <spa/utils/result.h>
#include <spa/utils/string.h>
#include <spa/utils/json.h>

#include <pipewire/impl.h>
#include <pipewire/i18n.h>

#include <nymphcast_client.h>


// TODO: documentation summary.


#define NAME "nymphcast-discover"

PW_LOG_TOPIC_STATIC(mod_topic, "mod." NAME);
#define PW_LOG_TOPIC_DEFAULT mod_topic

#define MODULE_USAGE "( stream.rules=<rules>, use create-stream actions )"

#define DEFAULT_CREATE_RULES	\
		"[ { matches = [ { nymphcast.ip = \"~.*\" } ] actions = { create-stream = { } } } ] "

static const struct spa_dict_item module_props[] = {
	{ PW_KEY_MODULE_AUTHOR, "Maya Posch" },
	{ PW_KEY_MODULE_DESCRIPTION, "Discover NymphCast receivers." },
	{ PW_KEY_MODULE_USAGE, MODULE_USAGE },
	{ PW_KEY_MODULE_VERSION, PACKAGE_VERSION },
};


struct impl {
	struct pw_context *context;

	struct pw_impl_module *module;
	//struct spa_hook module_listener;

	struct pw_properties *properties;

	NymphCastClient* client;
};


// --- START NYANSD ---
static int start_nyansd(struct impl* impl) {
	impl->client = new NymphCastClient;
	
	// TODO: Start a loop to periodically poll for new remotes. => Use PW Loop?
	
	// Search for remotes.
	std::vector<NymphCastRemote> list = impl->client.findServers();
	
	// TODO: Compare list with known remotes. Add new module for each new remote, remove vanished.
	
	
	// Create a module per found remote.
	// Provide the new NC sink module with the NCS IP & port.
	//char modname = "libpipewire-module-nymphcast-sink";
	for (int i = 0; i < list.size(); i++) {
		std::string args = list[i].ipv4 + ":" + std::string(list[i].port);
		pw_log_info("loading module: libpipewire-module-nymphcast-sink");
		struct pw_impl_module *mod;
		mod = pw_context_load_module(impl->context, 
									"libpipewire-module-nymphcast-sink", 
									args.c_str(), NULL);
		if (mod == NULL) {
			res = -errno;
			pw_log_error("Can't load module: %m");
			return 1;
		}
	}
	
	// TODO: add event listener to module, implement submodule & stuff. Optional?
	//pw_impl_module_add_listener(mod, &t->module_listener, &submodule_events, t);
			
	return 0;
}


// --- PIPEWIRE MODULE INIT ---
SPA_EXPORT
int pipewire__module_init(struct pw_impl_module *module, const char *args) {
	struct pw_context* context = pw_impl_module_get_context(module);
	struct pw_properties* props;
	struct impl* impl;
	int res;

	PW_LOG_TOPIC_INIT(mod_topic);

	impl = calloc(1, sizeof(struct impl));
	if (impl == NULL) { goto error_errno; }

	pw_log_debug("module %p: new %s", impl, args);

	if (args == NULL) { args = ""; }

	props = pw_properties_new_string(args);
	if (props == NULL) { goto error_errno; }

	spa_list_init(&impl->tunnel_list);

	impl->module = module;
	impl->context = context;
	impl->properties = props;

	pw_impl_module_add_listener(module, &impl->module_listener, &module_events, impl);

	pw_impl_module_update_properties(module, &SPA_DICT_INIT_ARRAY(module_props));

	// TODO: handle error (non-zero) return value.
	// TODO: Start a timer to call this function every N seconds.
	start_nyansd(impl);

	return 0;

error_errno:
	res = -errno;
	if (impl) { impl_free(impl); }
	return res;
}



