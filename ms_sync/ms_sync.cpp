/*
	ms_sync.cpp - test NymphRPC master-slave synchronisation.
*/


#include <string.h>
#include <iostream>

#include <Poco/Timestamp.h>
#include <Poco/NumberFormatter.h>
#include <Poco/Condition.h>
#include <Poco/Thread.h>

#include <nymph/nymph.h>

#include "chronotrigger.h"

// Globals
ChronoTrigger ct;
std::string loggerName = "MS_Sync";
uint32_t slaveLatencyMax = 0;	// Max latency to slave remote in milliseconds (? microseconds)
// ---


// --- CT CB ---
// ChronoTrigger callback.
void ct_cb(int /*unused*/) {
	// Fetch time, output to stdout.
	Poco::Timestamp ts;
	int64_t finished = ts.epochMicroseconds();
	NYMPH_LOG_DEBUG("Slave at: " + Poco::NumberFormatter::format(finished) + " microseconds.");
}


// Client disconnects from server.
// bool disconnect()
NymphMessage* disconnect(int session, NymphMessage* msg, void* data) {
	
	// Remove the client ID from the list.
	/* std::map<int, CastClient>::iterator it;
	it = clients.find(session);
	if (it != clients.end()) {
		clients.erase(it);
	}
	
	NYMPH_LOG_INFORMATION("Current server mode: " + Poco::NumberFormatter::format(serverMode));
	
	// Disconnect any slave remotes if we're connected.
	if (serverMode == NCS_MODE_MASTER) {
		NYMPH_LOG_DEBUG("# of slave remotes: " + 
								Poco::NumberFormatter::format(slave_remotes.size()));
		for (int i = 0; i < slave_remotes.size(); ++i) {
			// Disconnect from slave remote.
			NymphCastSlaveRemote& rm = slave_remotes[i];
			NYMPH_LOG_DEBUG("Disconnecting slave: " + rm.name);
			std::string result;
			if (!NymphRemoteServer::disconnect(rm.handle, result)) {
				// Failed to connect, error out. Disconnect from any already connected slaves.
				NYMPH_LOG_ERROR("Slave disconnect error: " + result);
			}
		}
		
		slave_remotes.clear();
	}
	
	NYMPH_LOG_INFORMATION("Switching to stand-alone server mode.");
	serverMode = NCS_MODE_STANDALONE; */
	
	NymphMessage* returnMsg = msg->getReplyMessage();
	returnMsg->setResultValue(new NymphType(true));
	msg->discard();
	
	return returnMsg;
}

// --- CONNECT MASTER ---
// Master server calls this to turn this server instance into a slave.
// This disables the regular client connection functionality for the duration of the master/slave
// session.
// Returns the timestamp when the message was received.
// sint64 connectMaster(sint64)
NymphMessage* connectMaster(int session, NymphMessage* msg, void* data) {
	NYMPH_LOG_INFORMATION("Received master connect request, slave mode initiation requested.");
	
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Switch to slave mode, if possible.
	// Return error if we're currently playing content in stand-alone mode.
	//if (ffplay.playbackActive()) {
		returnMsg->setResultValue(new NymphType((int64_t) 0));
	//}
	//else {
		// FIXME: for now we just return the current time.
		NYMPH_LOG_INFORMATION("Switching to slave server mode.");
		//serverMode = NCS_MODE_SLAVE;
		//DataBuffer::setFileSize(it->second.filesize);
		//DataBuffer::setSessionHandle(session);
		
		Poco::Timestamp ts;
		int64_t now = (int64_t) ts.epochMicroseconds();
		returnMsg->setResultValue(new NymphType(now));
	//}
	
	// TODO: Obtain timestamp, compare with current time.
	//time_t then = ((NymphSint64*) msg->parameters()[0])->getValue();
	
	// TODO: Send delay request to master.
	
	// TODO: Determine final latency and share with master.
	
	msg->discard();
	
	return returnMsg;
}


// --- RECEIVE DATA MASTER ---
// Receives data chunks for playback from a master receiver. (Slave-only)
// uint8 receiveDataMaster(blob data, bool done, sint64 when)
NymphMessage* receiveDataMaster(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Extract data blob and add it to the buffer.
	NymphType* mediaData = msg->parameters()[0];
	bool done = msg->parameters()[1]->getBool();
	
	// Write string into buffer.
	//DataBuffer::write(mediaData->getChar(), mediaData->string_length());
	
	// Playback is started in its own function, which is called by the master when it's ready.
	/* int64_t then = 0;
	if (!ffplay.playbackActive()) { */
		int64_t when = msg->parameters()[2]->getInt64();
		
		// Start the player when the delay in 'when' has been reached.
		/* std::condition_variable cv;
		std::mutex cv_m;
		std::unique_lock<std::mutex> lk(cv_m);
		//std::chrono::system_clock::time_point then = std::chrono::system_clock::from_time_t(when);
		std::chrono::microseconds dur(when);
		std::chrono::time_point<std::chrono::system_clock> then(dur);
		//while (cv.wait_until(lk, then) != std::cv_status::timeout) { }
		while (cv.wait_for(lk, dur) != std::cv_status::timeout) { } */
		
		// Start player.
		//ffplay.playTrack(when);
		ct.start(when / 1000, true); // milliseconds, as single-shot.
	/* }
	
	if (done) {
		DataBuffer::setEof(done);
	} */
	
	msg->discard();
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	return returnMsg;
}


struct NymphCastSlaveRemote {
	std::string name;
	std::string ipv4;
	std::string ipv6;
	uint16_t port;
	uint32_t handle;
	int64_t delay;
};


void logFunction(int level, std::string logStr) {
	std::cout << level << " - " << logStr << std::endl;
}


int main() {
	// Set up ChronoTrigger callback.
	ct.setCallback(ct_cb, 0);
	
	// Set up the callback functions
	std::cout << "Initialising server...\n";
	long timeout = 5000; // 5 seconds.
	NymphRemoteClient::init(logFunction, NYMPH_LOG_LEVEL_INFO, timeout);
	
	std::cout << "Registering methods...\n";
	std::vector<NymphTypes> parameters;
	
	// Master server calls this to turn this server instance into a slave.
	// uint8 connectMaster(sint64)
	parameters.clear();
	parameters.push_back(NYMPH_SINT64);
	NymphMethod connectMasterFunction("connectMaster", parameters, NYMPH_SINT64, connectMaster);
	NymphRemoteClient::registerMethod("connectMaster", connectMasterFunction);
	
	// Receives data chunks for playback.
	// uint8 receiveDataMaster(blob data, bool done, sint64)
	parameters.clear();
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_BOOL);
	parameters.push_back(NYMPH_SINT64);
	NymphMethod receivedataMasterFunction("receiveDataMaster", parameters, NYMPH_UINT8, receiveDataMaster);
	NymphRemoteClient::registerMethod("receiveDataMaster", receivedataMasterFunction);
	
	// Client disconnects from server.
	// bool disconnect()
	parameters.clear();
	NymphMethod disconnectFunction("disconnect", parameters, NYMPH_BOOL, disconnect);
	NymphRemoteClient::registerMethod("disconnect", disconnectFunction);
	
	// Start server on port 4004.
	NymphRemoteClient::start(4004);
	
	// Send connection request to slave, determine latency.
	NymphCastSlaveRemote rm;
	rm.name = "localhost";
	rm.ipv4 = "127.0.0.1";
	rm.ipv6 = "";
	rm.port = 4004;
	rm.handle = 0;
	rm.delay = 0;
	
	std::string result;
	if (!NymphRemoteServer::connect(rm.ipv4, 4004, rm.handle, 0, result)) {
		// Failed to connect, error out. Disconnect from any already connected slaves.
		NYMPH_LOG_ERROR("Slave connection error: " + result);
		return 1;
		
		/* for (; i >= 0; --i) {
			NymphCastSlaveRemote& drm = slave_remotes[i];
			NymphRemoteServer::disconnect(drm.handle, result);
		}
		
		returnMsg->setResultValue(new NymphType((uint8_t) 1));
		msg->discard();
		
		return returnMsg; */
	}
	
	// Send the current timestamp to the slave remote as part of the latency determination.
	Poco::Timestamp ts;
	int64_t now = (int64_t) ts.epochMicroseconds();
	std::vector<NymphType*> values;
	values.push_back(new NymphType(now));
	NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(rm.handle, "connectMaster", values, returnValue, result)) {
		NYMPH_LOG_ERROR("Slave connect master failed: " + result);
		return 1;
		
		// TODO: disconnect from slave remotes.
		/* returnMsg->setResultValue(new NymphType((uint8_t) 1));
		msg->discard();
		
		return returnMsg; */
	}
	
	// Get new time. This should be roughly twice the latency to the slave remote.
	ts.update();
	int64_t pong = ts.epochMicroseconds();
	time_t theirs = returnValue->getInt64();
	delete returnValue;
	if (theirs == 0) {
		NYMPH_LOG_ERROR("Configuring remote as slave failed.");
		return 1;
		
		// TODO: disconnect from slave remotes.
		/* returnMsg->setResultValue(new NymphType((uint8_t) 1));
		msg->discard();
		
		return returnMsg; */
	}
	
	// Use returned time stamp to calculate the delay.
	// FIXME: using stopwatch-style local time to determine latency for now.
	
	//rm.delay = theirs - now;
	rm.delay = pong - now;
	NYMPH_LOG_DEBUG("Slave delay: " + Poco::NumberFormatter::format(rm.delay) + 
						" microseconds.");
	NYMPH_LOG_DEBUG("Current max slave delay: " + 
						Poco::NumberFormatter::format(slaveLatencyMax));
	if (rm.delay > slaveLatencyMax) { 
		slaveLatencyMax = rm.delay;
		NYMPH_LOG_DEBUG("Max slave latency increased to: " + 
							Poco::NumberFormatter::format(slaveLatencyMax) + " microseconds.");
	}
	
	// Send start request to slave
	// When passing the message through to slave remotes, add the timestamp to the message.
	// This timestamp is the current time plus the largest master-slave latency times 2.
	// Timing: 	Multiply the max slave latency by the number of slaves. After sending this delay
	// 			to the first slave (minus half its recorded latency), 
	// 			subtract the time it took to send to this slave from the
	//			first delay, then send this new delay to the second slave, and so on.
	int64_t then = 0;
	//Poco::Timestamp ts;
	//int64_t now = 0;
	int64_t countdown = 0;
	
	now = (int64_t) ts.epochMicroseconds();
	//then = now + (slaveLatencyMax * 2);
	countdown = slaveLatencyMax; // * slave_remotes.size();
	
	int64_t send = 0;
	//then = slaveLatencyMax - rm.delay;
	then = countdown - (rm.delay / 2);
	ts.update();
	send = (int64_t) ts.epochMicroseconds();
	
	// Prepare data vector.
	//NymphType* media = new NymphType((char*) mediaData->getChar(), mediaData->string_length());
	std::string data = "FooBar";
	bool done = true;
	NymphType* media = new NymphType((char*) data.c_str(), data.length());
	NymphType* doneBool = new NymphType(done);
	//std::vector<NymphType*> values;
	values.clear();
	values.push_back(media);
	values.push_back(doneBool);
	values.push_back(new NymphType(then));
	
	//std::string result;
	//NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(rm.handle, "receiveDataMaster", values, returnValue, result)) {
		// TODO:
	}
	
	delete returnValue;
	
	ts.update();
	int64_t receive = (int64_t) ts.epochMicroseconds();
	countdown -= (receive - send);
	
	// Start locally too.
	// Wait out the countdown.
	std::condition_variable cv;
	std::mutex cv_m;
	std::unique_lock<std::mutex> lk(cv_m);
	std::chrono::microseconds dur(countdown);
	while (cv.wait_for(lk, dur) != std::cv_status::timeout) { }
	
	// Note time at countdown end.
	ts.update();
	int64_t finished = ts.epochMicroseconds();
	NYMPH_LOG_DEBUG("Finished at: " + Poco::NumberFormatter::format(finished) + " microseconds.");
	
	// Cleanup.
	if (!NymphRemoteServer::disconnect(rm.handle, result)) {
		// Failed to connect, error out. Disconnect from any already connected slaves.
		NYMPH_LOG_ERROR("Slave disconnect error: " + result);
	}
	
	NymphRemoteClient::shutdown();
	
	Poco::Thread::sleep(2000); // Wait for 2 seconds.
	ct.stop(); // Finish ChronoTrigger timer.
	
	return 0;
}
