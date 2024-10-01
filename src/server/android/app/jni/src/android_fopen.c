#include <stdio.h>

#include "android_fopen.h"
#include <errno.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <SDL2/SDL.h>
#include <jni.h>


static int android_read(void* cookie, char* buf, int size) {
  return AAsset_read((AAsset*)cookie, buf, size);
}

static int android_write(void* cookie, const char* buf, int size) {
  return EACCES; // can't provide write access to the apk
}

static fpos_t android_seek(void* cookie, fpos_t offset, int whence) {
  return AAsset_seek((AAsset*)cookie, offset, whence);
}

static int android_close(void* cookie) {
  AAsset_close((AAsset*)cookie);
  return 0;
}

static AAssetManager* createAssetManager() {
	JNIEnv* env = (JNIEnv*) SDL_AndroidGetJNIEnv();

    jobject activity = (jobject) SDL_AndroidGetActivity();
    jclass activity_class = (*env)->GetObjectClass(env, activity);
    //jclass activity_class(env->GetObjectClass(activity));

    jmethodID activity_class_getAssets = (*env)->GetMethodID(env, activity_class, "getAssets", 
														"()Landroid/content/res/AssetManager;");
														
	// activity.getAssets();
	AAssetManager* pAssetManager = NULL;
    jobject asset_manager = (*env)->CallObjectMethod(env, activity, activity_class_getAssets); 
    jobject global_asset_manager = (*env)->NewGlobalRef(env, asset_manager);

    pAssetManager = AAssetManager_fromJava(env, global_asset_manager);
	return pAssetManager;
}

// must be established by someone else...
static AAssetManager* android_asset_manager = NULL;
void android_fopen_set_asset_manager(AAssetManager* manager) {
	android_asset_manager = manager;
}

FILE* android_fopen(const char* fname, const char* mode) {
	if (android_asset_manager == NULL) {
		android_asset_manager =  createAssetManager();
	}
	
	if(mode[0] == 'w') return NULL;

	AAsset* asset = AAssetManager_open(android_asset_manager, fname, 0);
	if(!asset) return NULL;

	return funopen(asset, android_read, android_write, android_seek, android_close);
}
