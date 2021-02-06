package com.nyanko.nymphcastplayer;


import android.util.Log;

import com.nyanko.nymphcastplayer.ui.main.RemotesContent;
import com.nyanko.nymphcastplayer.ui.main.RemotesFragment;

public class NymphCast {
	static {
		System.loadLibrary("nymphcast_jni");
	}

	private native void NymphCast();

	// NymphCast API.
	public native void setClientId(String id);

	public native String getApplicationList();
	public native String sendApplicationMessage(String appId, String mesage);

	public native void findServers();
	public native boolean connectServer(long index);
	public native boolean disconnectServer(long index);

	public native boolean castFile(String filename);
	public native boolean castUrl(String url);

	public native boolean playbackStart();
	public native boolean playbackStop();

	// --- Non-native ---
	public void setRemoteList(String[] remotes) {
		Log.i("NymphCast", "SetRemoteList called.");
		// Clear the old list, replace with items from the array.
		RemotesContent.ITEMS.clear();

		// Add the new items.
		for (int i = 0; i < remotes.length; ++i) {
			RemotesContent.ITEMS.add(new RemotesContent.RemoteItem(Integer.toString(i), remotes[i],
																					remotes[i]));
		}

		RemotesFragment.mAdapter.notifyDataSetChanged();
	}
}
