package com.nyanko.nymphcastplayer;


import android.content.ContentResolver;
import android.content.ContentUris;
import android.database.Cursor;
import android.net.Uri;
import java.util.ArrayList;
import android.provider.MediaStore;
import com.nyanko.nymphcastplayer.MediaItem;


public class NymphCast {
	public static ArrayList<MediaItem> loadAudio(Context appContext) {
		//Context appContext = MainActivity.getContextOfApplication();
		ContentResolver contentResolver = appContext.getContentResolver();

		Uri uri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
		String selection = MediaStore.Audio.Media.IS_MUSIC + "!= 0";
		String sortOrder = MediaStore.Audio.Media.TITLE + " ASC";
		Cursor cursor = contentResolver.query(uri, null, selection, null, sortOrder);
		
		ArrayList<MediaItem> items = new ArrayList<MediaItem>();
		if (cursor != null && cursor.getCount() > 0) {
			//audioList = new ArrayList<Audio>();
			//MediaContent.ITEMS.clear();
			//audioList.clear();
			while (cursor.moveToNext()) {
				long id = cursor.getLong(cursor.getColumnIndex(MediaStore.Audio.Media._ID));
				//Uri data = cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.DATA));
				String title = cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.TITLE));
				String album = cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.ALBUM));
				String artist = cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.ARTIST));

				Uri contentUri = ContentUris.withAppendedId(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, id);
				
				items.add(new MediaItem(title, album, artist, getPath(appContext, contentUri)));
				
				//strings[strings.length()] = getPath(appContext, contentUri) + "," + artist + " - " title;
				// Save to audioList and MediaContent.
				//audioList.add(new Audio(contentUri, title, album, artist));
				//MediaContent.ITEMS.add(new MediaContent.MediaItem(title, artist, album, contentUri));
			}
		}

		if (cursor != null) {
			cursor.close();
		}
	}
	
	@RequiresApi(api = Build.VERSION_CODES.KITKAT)
    public static String getPath(final Context context, final Uri uri) {
        final boolean isKitKat = Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT;

        // DocumentProvider
        if (isKitKat && DocumentsContract.isDocumentUri(context, uri)) {
            System.out.println("getPath() uri: " + uri.toString());
            System.out.println("getPath() uri authority: " + uri.getAuthority());
            System.out.println("getPath() uri path: " + uri.getPath());

            // ExternalStorageProvider
            if ("com.android.externalstorage.documents".equals(uri.getAuthority())) {
                final String docId = DocumentsContract.getDocumentId(uri);
                final String[] split = docId.split(":");
                final String type = split[0];
                System.out.println("getPath() docId: " + docId + ", split: " + split.length + ", type: " + type);

                // This is for checking Main Memory
                if ("primary".equalsIgnoreCase(type)) {
                    if (split.length > 1) {
                        return Environment.getExternalStorageDirectory() + "/" + split[1] + "/";
                    } else {
                        return Environment.getExternalStorageDirectory() + "/";
                    }
                    // This is for checking SD Card
                } else {
                    return "storage" + "/" + docId.replace(":", "/");
                }

            }
        }
        return null;
    }

    @RequiresApi(api = Build.VERSION_CODES.KITKAT)
    public void onListFragmentInteraction(MediaContent.MediaItem item) {
        // A media item got selected for playback.
		Uri contentUri = item.uri;

		String filename = getPath(this, contentUri);
		nymphCast.castFile(filename);

		// Open URI and pass file handle to the native function.
		/* String fileOpenMode = "r";
		ParcelFileDescriptor parcelFd = resolver.openFileDescriptor(contentUri, fileOpenMode);
		if (parcelFd != null) {
			int fd = parcelFd.detachFd();
			// Pass the integer value "fd" into your native code. Remember to call
			// close(2) on the file descriptor when you're done using it.
		} */
    }
}