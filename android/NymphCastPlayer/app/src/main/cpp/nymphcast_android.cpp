/*
	nymphcast_android.cpp - Android core for the NymphCast client library.

	Features:
			- Drives a Java-based UI via a JNI-based interface.

	2020/05/21, Maya Posch
*/

#include "com_nyanko_nymphcastplayer_NymphCast.h"

#include <nymphcast_client.h>
#include <atomic>
#include <mutex>


NymphCastClient client;
std::atomic<uint32_t> activeHandle;
std::atomic<int64_t> activeIndex;
std::atomic<bool> connected = { false };
std::vector<NymphCastRemote> remotes;
std::mutex remotesMutex;


// --- STATUS UPDATE CALLBACK ---
void statusUpdateCallback(uint32_t handle, NymphPlaybackStatus status) {
	if (status.playing) {
		// Remote player is active. Read out 'status.status' to get the full status.
		/* ui->playToolButton->setEnabled(false);
		ui->stopToolButton->setEnabled(true);

		// Set position & duration.
		QTime position(0, 0);
		position.addSecs((int64_t) status.position);
		QTime duration(0, 0);
		duration.addSecs(status.duration);
		ui->durationLabel->setText(position.toString("hh:mm:ss") + " / " +
														duration.toString("hh:mm:ss"));

		ui->positionSlider->setValue((status.position / status.duration) * 100);

		ui->volumeSlider->setValue(status.volume); */
	}
	else {
		// Remote player is not active.
		/* ui->playToolButton->setEnabled(true);
		ui->stopToolButton->setEnabled(false);

		ui->durationLabel->setText("0:00 / 0:00");
		ui->positionSlider->setValue(0);
		ui->volumeSlider->setValue(0); */
	}
}


// --- NYMPHCAST ---
// Set up NymphCast client library.
extern "C" JNIEXPORT void JNICALL Java_com_nyanko_nymphcastplayer_NymphCast_NymphCast
	(JNIEnv *env, jobject obj) {
	// NymphCast client SDK callbacks.
	using namespace std::placeholders;
	client.setStatusUpdateCallback(statusUpdateCallback);
}


// --- SET CLIENT ID ---
extern "C" JNIEXPORT void JNICALL Java_com_nyanko_nymphcastplayer_NymphCast_setClientId
	(JNIEnv* env, jobject obj, jstring id) {
	jboolean isCopy;
	const char* convValue = env->GetStringUTFChars(id, &isCopy);
	std::string idStr = convValue;
	env->ReleaseStringUTFChars(id, convValue);

	client.setClientId(idStr);
}


// --- GET APPLICATION LIST ---
extern "C" JNIEXPORT jstring JNICALL Java_com_nyanko_nymphcastplayer_NymphCast_getApplicationList
	(JNIEnv* env, jobject obj) {
	jstring result = nullptr;
	if (!connected) { return result; }

	// Get list of apps from the remote server.
	std::string appList = client.getApplicationList(activeHandle);
	result = env->NewStringUTF(appList.c_str());
	return result;
}


// --- SEND APPLICATION MESSAGE ---
extern "C" JNIEXPORT jstring JNICALL Java_com_nyanko_nymphcastplayer_NymphCast_sendApplicationMessage
	(JNIEnv* env, jobject obj, jstring appId, jstring msg) {
	jstring result = nullptr;
	if (!connected) { return result; }

	jboolean isCopy;
	const char* convValue = env->GetStringUTFChars(appId, &isCopy);
	std::string appIdStr = convValue;
	env->ReleaseStringUTFChars(appId, convValue);

	convValue = env->GetStringUTFChars(msg, &isCopy);
	std::string msgStr = convValue;
	env->ReleaseStringUTFChars(msg, convValue);

	std::string response = client.sendApplicationMessage(activeHandle, appIdStr, msgStr);

	result = env->NewStringUTF(response.c_str());
	return result;
}


// --- FIND SERVERS ---
extern "C" JNIEXPORT void JNICALL Java_com_nyanko_nymphcastplayer_NymphCast_findServers
	(JNIEnv* env, jobject obj) {
	remotesMutex.lock();
	remotes = client.findServers();
	remotesMutex.unlock();

	// Create the new array
	jobjectArray remoteArray = env->NewObjectArray(remotes.size(), env->FindClass("java/lang/String"), 0);
	for (int i = 0; i < remotes.size(); ++i) {
		env->SetObjectArrayElement(remoteArray, i, env->NewStringUTF(remotes[i].ipv4.c_str()));
	}

	// Update the UI list.
	jclass ncj = env->GetObjectClass(obj);
	jmethodID setRemoteList = env->GetMethodID(ncj, "setRemoteList", "([Ljava/lang/String;)V");
	env->CallVoidMethod(obj, setRemoteList, remoteArray);
}


// --- CONNECT SERVER ---
extern "C" JNIEXPORT jboolean JNICALL Java_com_nyanko_nymphcastplayer_NymphCast_connectServer
	(JNIEnv* env, jobject obj, jlong index) {
	if (connected && activeIndex == index) { return true; }

	// Use the index with the list of remotes to determine which remote to connect to.
	// If we are already connected, we have to disconnect from the currently connected remote first.
	// TODO: enable multi-remote feature.
	if (connected) {
		client.disconnectServer(activeHandle);
	}

	if (index > remotes.size()) { return false; }

	remotesMutex.lock();
	uint32_t h;
	bool retval = client.connectServer(remotes[index].ipv4, h);
	activeHandle = h;
	remotesMutex.unlock();

	if (!retval) {
		return false;
	}

	connected = true;
	return true;
}


// --- DISCONNECT SERVER ---
extern "C" JNIEXPORT jboolean JNICALL Java_com_nyanko_nymphcastplayer_NymphCast_disconnectServer
	(JNIEnv* env, jobject obj, jlong index) {
	// Use the index with the list of remotes to determine whether we are connected to the
	// indicated remote.
	jboolean result;
	result = false;
	if (!connected || activeIndex != index) { return result; }

	client.disconnectServer(activeHandle);
	connected = false;

	result = true;
	return result;
}


// -- CAST FILE ---
extern "C" JNIEXPORT jboolean JNICALL Java_com_nyanko_nymphcastplayer_NymphCast_castFile
	(JNIEnv* env, jobject obj, jstring filename) {
	jboolean isCopy;
	const char* convValue = (env)->GetStringUTFChars(filename, &isCopy);
	std::string filenameStr = convValue;
	env->ReleaseStringUTFChars(filename, convValue);

	client.castFile(activeHandle, filenameStr);

	jboolean result = true;
	return result;
}


// --- CAST URL ---
extern "C" JNIEXPORT jboolean JNICALL Java_com_nyanko_nymphcastplayer_NymphCast_castUrl
	(JNIEnv* env, jobject obj, jstring url) {
	jboolean isCopy;
	const char* convValue = (env)->GetStringUTFChars(url, &isCopy);
	std::string urlStr = convValue;
	env->ReleaseStringUTFChars(url, convValue);

	client.castUrl(activeHandle, urlStr);

	jboolean result = true;
	return result;
}


// -- PLAYBACK START ---
extern "C" JNIEXPORT jboolean JNICALL Java_com_nyanko_nymphcastplayer_NymphCast_playbackStart
	(JNIEnv* env, jobject obj) {
	if (!connected) { return false; }

	// FIXME: start playing new track, or resuming here?
	client.playbackStart(activeHandle);

	return true;
}


// -- PLAYBACK STOP ---
extern "C" JNIEXPORT jboolean JNICALL Java_com_nyanko_nymphcastplayer_NymphCast_playbackStop
	(JNIEnv* env, jobject obj) {
	if (!connected) { return false; }

	client.playbackStop(activeHandle);

	return true;
}
