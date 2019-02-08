/*
	NymphCastServer.cpp - Server that accepts NymphCast client sessions to play back audio.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			- 
		
	2019/01/25, Maya Posch
*/


#include <iostream>
#include <vector>
#include <csignal>

#include <portaudio.h>
#include <nymph/nymph.h>

#include <Poco/Condition.h>
#include <Poco/Thread.h>

using namespace Poco;


#define SAMPLE_RATE (44100)


// Global objects.
Condition gCon;
Mutex gMutex;
PaStream *stream;
// ---


void signal_handler(int signal) {
	gCon.signal();
}


struct paTestData {
    float left_phase;
    float right_phase;
};


// PortAudio callback.
static int patestCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData ) {
    /* Cast data passed through stream to our structure. */
    paTestData *data = (paTestData*)userData; 
    float *out = (float*)outputBuffer;
    unsigned int i;
    (void) inputBuffer; /* Prevent unused variable warning. */
    
    for( i=0; i<framesPerBuffer; i++ )     {
        *out++ = data->left_phase;  /* left */
        *out++ = data->right_phase;  /* right */
        /* Generate simple sawtooth phaser that ranges between -1.0 and 1.0. */
        data->left_phase += 0.01f;
        /* When signal reaches top, drop back down. */
        if( data->left_phase >= 1.0f ) data->left_phase -= 2.0f;
        /* higher pitch so we can distinguish left and right. */
        data->right_phase += 0.03f;
        if( data->right_phase >= 1.0f ) data->right_phase -= 2.0f;
    }
    return 0;
}


// Callback for the connect function.
NymphMessage* connect(int session, NymphMessage* msg, void* data) {
	std::cout << "Received message for session: " << session << ", msg ID: " << msg->getMessageId() << "\n";
	
	std::string clientStr = ((NymphString*) msg->parameters()[0])->getValue();
	std::cout << "Client string: " << clientStr << "\n";
	
	// Start the audio stream.
	PaError err = Pa_StartStream(stream);
	if (err != paNoError) {
		std::cerr << "PortAudio initialisation error: " << Pa_GetErrorText(err) << std::endl;
		// TODO: handle.
	}
	
	// TODO: call err = Pa_StopStream( stream );
	
	NymphMessage* returnMsg = msg->getReplyMessage();
	//NymphString* world = new NymphString(echoStr);
	NymphBoolean* retVal = new NymphBoolean(true);
	returnMsg->setResultValue(retVal);
	return returnMsg;
}


// --- LOG FUNCTION ---
void logFunction(int level, std::string logStr) {
	std::cout << level << " - " << logStr << std::endl;
}


int main() {
	// Initialise the server.
	std::cout << "Initialising server...\n";
	long timeout = 5000; // 5 seconds.
	NymphRemoteClient::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
	
	// Initialise the PortAudio library.
	PaError err = Pa_Initialize();
	if (err != paNoError) {
		std::cerr << "PortAudio initialisation error: " << Pa_GetErrorText(err) << std::endl;
		// TODO: handle.
	}
	
	// Open audio output.
	paTestData data;
    err = Pa_OpenDefaultStream( &stream,
                                0,          /* no input channels */
                                2,          /* stereo output */
                                paFloat32,  /* 32 bit floating point output */
                                SAMPLE_RATE,
                                paFramesPerBufferUnspecified,        /* frames per buffer, i.e. the number
                                                   of sample frames that PortAudio will
                                                   request from the callback. Many apps
                                                   may want to use
                                                   paFramesPerBufferUnspecified, which
                                                   tells PortAudio to pick the best,
                                                   possibly changing, buffer size.*/
                                patestCallback, /* this is your callback function */
                                &data ); /*This is a pointer that will be passed to
                                                   your callback*/
    if (err != paNoError) {
		std::cerr << "PortAudio initialisation error: " << Pa_GetErrorText(err) << std::endl;
		// TODO: handle.
	}
	
	
	// Define all of the RPC methods we want to export for clients.
	std::cout << "Registering methods...\n";
	std::vector<NymphTypes> parameters;
	
	// Client connects to server.
	// bool connect(string client_id)
	parameters.push_back(NYMPH_STRING);
	NymphMethod connectFunction("connect", parameters, NYMPH_STRING);
	connectFunction.setCallback(connect);
	NymphRemoteClient::registerMethod("connect", connectFunction);
	
	// Client disconnects from server.
	// bool disconnect(string client_id)
	
	// Client starts a session.
	// Return value: OK (0), ERROR (1).
	// int session_start()
	
	// Client sends meta data for the track.
	// Returns: OK (0), ERROR (1).
	// int session_meta(string artist, string album, int track, string name)
	
	// Client sends a chunk of track data.
	// Returns: OK (0), ERROR (1).
	// int session_data(string buffer)
	
	// Client ends the session.
	// Returns: OK (0), ERROR (1).
	// int session_end()
	
	// Install signal handler to terminate the server.
	signal(SIGINT, signal_handler);
	
	// Start server on port 4004.
	NymphRemoteClient::start(4004);
	
	// Loop until the SIGINT signal has been received.
	gMutex.lock();
	gCon.wait(gMutex);
	
	// Clean-up
	NymphRemoteClient::shutdown();
	err = Pa_Terminate();
	if (err != paNoError) {
		std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
	}
	
	// Wait before exiting, giving threads time to exit.
	Thread::sleep(2000); // 2 seconds.
	
	return 0;
}
