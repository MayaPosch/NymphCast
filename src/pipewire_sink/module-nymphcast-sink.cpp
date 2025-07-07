/*
	module-nymphcast-sink.cpp - PipeWire sink module for NymphCast.
*/


#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <math.h>

//#include "config.h"

#include <spa/utils/result.h>
#include <spa/utils/string.h>
#include <spa/utils/json.h>
#include <spa/utils/ringbuffer.h>
#include <spa/debug/types.h>
#include <spa/pod/builder.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/audio/raw.h>
#include <spa/param/audio/raw-json.h>

#include <pipewire/impl.h>
#include <pipewire/i18n.h>

#include <nymphcast_client.h>
#include <queue>
#include <mutex>
#include <cstdint>
#include <iostream>

std::string ip;
uint32_t port;
uint32_t ncs_handle;
std::queue<uint16_t> sample_queue;
std::mutex sample_mutex;
uint16_t wav_header[22]; // 44 byte buffer.


#define NAME "nymphcast-sink"

#define PACKAGE_VERSION "0.1"

PW_LOG_TOPIC_STATIC(mod_topic, "mod." NAME);
#define PW_LOG_TOPIC_DEFAULT mod_topic

#define DEFAULT_FORMAT "S16LE"
#define DEFAULT_RATE 48000
#define DEFAULT_CHANNELS 2
#define DEFAULT_POSITION "[ FL FR ]"


// TODO: expand MODULE_USAGE.
#define MODULE_USAGE	"( nc.ip=<ip address of host> ) "	\
			"( nc.port=<remote port> ) "	\
			"( node.latency=<latency as fraction> ) "				\
			"( node.name=<name of the nodes> ) "					\
			"( node.description=<description of the nodes> ) "			\
			"( audio.format=<format, default:"DEFAULT_FORMAT"> ) "			\
			"( audio.rate=<sample rate, default: "SPA_STRINGIFY(DEFAULT_RATE)"> ) "			\
			"( audio.channels=<number of channels, default:"SPA_STRINGIFY(DEFAULT_CHANNELS)"> ) "	\
			"( audio.position=<channel map, default:"DEFAULT_POSITION"> ) "		\
			"( stream.props=<properties> ) "

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

	NymphCastClient* client;
};


// --- MEDIA READ CALLBACK ---
// Called when the NC client library receives a read request from the NCS.
void MediaReadCallback(uint32_t session, NymphMessage* msg, void* data) {
	// Read in as much as possible from the queue, very likely less than the NCS is requesting.
	uint32_t bufLenDefault = 200 * 1024;
	uint32_t bufLen = 0;
	if (msg->parameters().size() > 0) {
		bufLen = msg->parameters()[0]->getUint32();
	}
	
	if (bufLen == 0) { 
		bufLen = bufLenDefault; 
	}
	else {
		bufLen *= 1024;
	}
	
	// Allocate a new buffer. This will have the remote's specified or the custom size.
	char* buffer = new char[bufLen];
	sample_mutex.lock();
	int count = 0;
	while (!sample_queue.empty() && count < bufLen) {
		uint16_t sample = sample_queue.front();
		buffer[count++] = ((uint8_t*) &sample)[0];
		buffer[count++] = ((uint8_t*) &sample)[1];
		sample_queue.pop();
	}
	
	sample_mutex.unlock();
	
	// Always set EOF to false since we're streaming.
	NymphType* fileEof = new NymphType(false);
	
	// Clean up the message we got.
	msg->discard();
	
	std::vector<NymphType*> values;
	values.push_back(new NymphType(buffer, count, true));
	values.push_back(fileEof);
	NymphType* returnValue = 0;
	std::string result;
	if (!NymphRemoteServer::callMethod(session, "session_data", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(session, result);
		return;
	}
	
	delete returnValue;
}


// --- MEDIA SEEK CALLBACK ---
// Generally shouldn't get called. If it does, ignore it.
void MediaSeekCallback(uint32_t session, NymphMessage* msg, void* data) {
	// NOP
	
}


static void stream_destroy(void *d) {
	struct impl *impl = (struct impl*) d;
	spa_hook_remove(&impl->stream_listener);
	impl->stream = NULL;
}


static void stream_state_changed(void *d, enum pw_stream_state old,
		enum pw_stream_state state, const char *error) {
	struct impl *impl = (struct impl*) d;
	switch (state) {
	case PW_STREAM_STATE_ERROR:
	case PW_STREAM_STATE_UNCONNECTED:
		pw_impl_module_schedule_destroy(impl->module);
		break;
	case PW_STREAM_STATE_PAUSED:
		// Terminate the NCS sessions.
		impl->client->disconnectServer(ncs_handle);
		sample_mutex.lock();
		while (!sample_queue.empty()) {
			sample_queue.pop();	// Clear the queue.
		}
		
		sample_mutex.unlock();
		
		break;
	case PW_STREAM_STATE_STREAMING:
		// Start session with NCS.
		// First write the prepared WAV header into the queue.
		sample_mutex.lock();
		for (int i = 0; i < 22; i++) { // WAV header is 44 bytes, so we read in 22 16-bit chunks.
			sample_queue.push(wav_header[i]);
		}
		
		sample_mutex.unlock();
		
		// Connect to the remote NCS. 
		// TODO: handle connection error.
		impl->client->connectServer(ip, port, ncs_handle);
		
		break;
	default:
		break;
	}
}


static void playback_stream_process(void *d) {
	struct impl *impl = (struct impl*) d;
	struct pw_buffer *buf;
	struct spa_data *bd;
	void *data;
	uint32_t offs, size;

	if ((buf = pw_stream_dequeue_buffer(impl->stream)) == NULL) {
		pw_log_debug("out of buffers: %m");
		return;
	}

	bd = &buf->buffer->datas[0];

	offs = SPA_MIN(bd->chunk->offset, bd->maxsize);
	size = SPA_MIN(bd->chunk->size, bd->maxsize - offs);
	data = SPA_PTROFF(bd->data, offs, void);

	/* write buffer contents here */
	//pw_log_info("got buffer of size %d and data %p", size, data);
	
	// Write samples into queue.
	sample_mutex.lock();
	sample_queue.push((uint16_t) *data);
	sample_mutex.unlock();

	pw_stream_queue_buffer(impl->stream, buf);
}

static const struct pw_stream_events playback_stream_events = {
	PW_VERSION_STREAM_EVENTS,
	.destroy = stream_destroy,
	.state_changed = stream_state_changed,
	.process = playback_stream_process
};

static int create_stream(struct impl *impl)
{
	int res;
	uint32_t n_params;
	const struct spa_pod *params[1];
	uint8_t buffer[1024];
	struct spa_pod_builder b;

	impl->stream = pw_stream_new(impl->core, "NymphCast sink", impl->stream_props);
	impl->stream_props = NULL;

	if (impl->stream == NULL)
		return -errno;

	pw_stream_add_listener(impl->stream,
			&impl->stream_listener,
			&playback_stream_events, impl);

	n_params = 0;
	spa_pod_builder_init(&b, buffer, sizeof(buffer));
	params[n_params++] = spa_format_audio_raw_build(&b,
			SPA_PARAM_EnumFormat, &impl->info);

	if ((res = pw_stream_connect(impl->stream,
			PW_DIRECTION_INPUT,
			PW_ID_ANY,
			PW_STREAM_FLAG_AUTOCONNECT |
			PW_STREAM_FLAG_MAP_BUFFERS |
			PW_STREAM_FLAG_RT_PROCESS,
			params, n_params)) < 0)
		return res;

	return 0;
}

static void core_error(void *data, uint32_t id, int seq, int res, const char *message)
{
	struct impl *impl = data;

	pw_log_error("error id:%u seq:%d res:%d (%s): %s",
			id, seq, res, spa_strerror(res), message);

	if (id == PW_ID_CORE && res == -EPIPE)
		pw_impl_module_schedule_destroy(impl->module);
}

static const struct pw_core_events core_events = {
	PW_VERSION_CORE_EVENTS,
	.error = core_error,
};


static void core_destroy(void *d) {
	struct impl *impl = d;
	spa_hook_remove(&impl->core_listener);
	impl->core = NULL;
	pw_impl_module_schedule_destroy(impl->module);
}


static const struct pw_proxy_events core_proxy_events = {
	.destroy = core_destroy,
};


static void impl_destroy(struct impl *impl) {
	if (impl->stream)
		pw_stream_destroy(impl->stream);
	if (impl->core && impl->do_disconnect)
		pw_core_disconnect(impl->core);

	pw_properties_free(impl->stream_props);
	pw_properties_free(impl->props);

	free(impl);
}


static void module_destroy(void *data) {
	struct impl *impl = data;
	spa_hook_remove(&impl->module_listener);
	impl_destroy(impl);
}


static const struct pw_impl_module_events module_events = {
	PW_VERSION_IMPL_MODULE_EVENTS,
	.destroy = module_destroy,
};


// --- INIT WAV HEADER ---
void init_wav_header() {
	uint8_t* buf = &wav_header;
	
	char riff[] = "RIFF";
	memcpy(buf, &riff, 4);
	buf += 4;
	
	uint32_t size = 0xffffffff; // Max size for no particular reason.
	memcpy(buf, &size, 4);
	buf += 4;
	
	char wav[] = "WAVE";
	memcpy(buf, &wav, 4);
	buf += 4;
	
	char fmt[] = "fmt ";
	memcpy(buf, &fmt, 4);
	buf += 4;
	
	uint32_t chunksize = 16;
	memcpy(buf, &chunksize, 4);
	buf += 4;
	
	uint16_t audio_format = 1; // PCM.
	memcpy(buf, &audio_format, 2);
	buf += 2;
	
	uint16_t nchan = 2; // Stereo channels.
	memcpy(buf, &nchan, 2);
	buf += 2;
	
	uint32_t freq = 48000; // Sample rate frequency.
	memcpy(buf, &freq, 4);
	buf += 4;
	
	uint32_t byteps = 192000;
	memcpy(buf, &byteps, 4);
	buf += 4;
	
	uint16_t bytepb = 4; // Bytes per block. (channels * bytes per sample).
	memcpy(buf, &bytepb, 2);
	buf += 2;
	
	uint16_t bitspersample = 16;
	memcpy(buf, &bitspersample, 2);
	buf += 2;
	
	char data[] = "data";
	memcpy(buf, &data, 4);
	buf += 4;
	
	uint32_t datasize = size - 44;
	memcpy(buf, &datasize, 4);
	buf += 4;
}


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
	
	// Init NC client & set the callbacks we wish to use.
	impl->client = new NymphCastClient;
	impl->client.setMediaCallbacks(&MediaReadCallback, &MediaSeekCallback);
	
	// Prepare WAV header.
	init_wav_header();

	pw_log_debug("module %p: new %s", impl, args);if (args == NULL)
		args = "";

	props = pw_properties_new_string(args);
	if (props == NULL) {
		res = -errno;
		pw_log_error( "can't create properties: %m");
		goto error;
	}
	impl->props = props;

	impl->stream_props = pw_properties_new(NULL, NULL);
	if (impl->stream_props == NULL) {
		res = -errno;
		pw_log_error( "can't create properties: %m");
		goto error;
	}

	impl->module = module;
	impl->context = context;

	
	if (pw_properties_get(props, "nc.ip") == NULL || pw_properties_get(props, "nc.port") == NULL) {
		pw_log_error("Missing nc.ip or nc.port");
		res = -EINVAL;
		goto error;
	}
	
	ip = std::string(pw_properties_get(props, "nc.ip"));
	port = pw_properties_get_uint32(props, "nc.port");

	if (pw_properties_get(props, PW_KEY_NODE_VIRTUAL) == NULL)
		pw_properties_set(props, PW_KEY_NODE_VIRTUAL, "true");

	if (pw_properties_get(props, PW_KEY_MEDIA_CLASS) == NULL)
		pw_properties_set(props, PW_KEY_MEDIA_CLASS, "Audio/Sink");

	if (pw_properties_get(props, PW_KEY_NODE_NAME) == NULL)
		pw_properties_setf(props, PW_KEY_NODE_NAME, "example-sink-%u-%u", pid, id);
	if (pw_properties_get(props, PW_KEY_NODE_DESCRIPTION) == NULL)
		pw_properties_set(props, PW_KEY_NODE_DESCRIPTION,
				pw_properties_get(props, PW_KEY_NODE_NAME));

	if ((str = pw_properties_get(props, "stream.props")) != NULL)
		pw_properties_update_string(impl->stream_props, str, strlen(str));

	copy_props(impl, props, PW_KEY_AUDIO_RATE);
	copy_props(impl, props, PW_KEY_AUDIO_CHANNELS);
	copy_props(impl, props, SPA_KEY_AUDIO_POSITION);
	copy_props(impl, props, PW_KEY_NODE_NAME);
	copy_props(impl, props, PW_KEY_NODE_DESCRIPTION);
	copy_props(impl, props, PW_KEY_NODE_GROUP);
	copy_props(impl, props, PW_KEY_NODE_LATENCY);
	copy_props(impl, props, PW_KEY_NODE_VIRTUAL);
	copy_props(impl, props, PW_KEY_MEDIA_CLASS);
	
	// Obtain the IP address & port string from the 'args' string.
	// Format is <ip>:<port>, where the address is in IPv4 string format. Port is an integer.
	/* std::string argstr = std::string(args);
	std::size_t pos = argstr.find_last_of(':');
	std::string ip = argstr.substr(0, pos);
	uint32_t port = (uint32_t) std::stoi(argstr.substr(pos + 1)); */
	
	// TODO: Set up the audio stuff?
	
	impl->core = pw_context_get_object(impl->context, PW_TYPE_INTERFACE_Core);
	if (impl->core == NULL) {
		str = pw_properties_get(props, PW_KEY_REMOTE_NAME);
		impl->core = pw_context_connect(impl->context,
				pw_properties_new(
					PW_KEY_REMOTE_NAME, str,
					NULL),
				0);
		impl->do_disconnect = true;
	}
	if (impl->core == NULL) {
		res = -errno;
		pw_log_error("can't connect: %m");
		goto error;
	}

	pw_proxy_add_listener((struct pw_proxy*)impl->core,
			&impl->core_proxy_listener,
			&core_proxy_events, impl);
	pw_core_add_listener(impl->core,
			&impl->core_listener,
			&core_events, impl);

	if ((res = create_stream(impl)) < 0)
		goto error;

	pw_impl_module_add_listener(module, &impl->module_listener, &module_events, impl);

	pw_impl_module_update_properties(module, &SPA_DICT_INIT_ARRAY(module_props));

	return 0;

error:
	impl_destroy(impl);
	return res;
}
