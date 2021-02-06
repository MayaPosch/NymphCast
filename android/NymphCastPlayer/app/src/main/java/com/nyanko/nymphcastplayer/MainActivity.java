package com.nyanko.nymphcastplayer;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;

import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.snackbar.Snackbar;
import com.google.android.material.tabs.TabLayout;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.viewpager2.widget.ViewPager2;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Environment;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.view.View;

import com.google.android.material.tabs.TabLayoutMediator;
import com.nyanko.nymphcastplayer.ui.main.Audio;
import com.nyanko.nymphcastplayer.ui.main.MediaContent;
import com.nyanko.nymphcastplayer.ui.main.MediaFragment;
import com.nyanko.nymphcastplayer.ui.main.RemotesContent;
import com.nyanko.nymphcastplayer.ui.main.SectionsPagerAdapter;

import java.util.ArrayList;

public class MainActivity extends AppCompatActivity
        implements MediaFragment.OnListFragmentInteractionListener {

	public static NymphCast nymphCast = new NymphCast();

    public static Context contextOfApplication;
    public static Context getContextOfApplication() {
        return contextOfApplication;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        contextOfApplication = getApplicationContext();

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        SectionsPagerAdapter sectionsPagerAdapter = new SectionsPagerAdapter(this);
        ViewPager2 viewPager = findViewById(R.id.view_pager);
        viewPager.setAdapter(sectionsPagerAdapter);
        TabLayout tabs = findViewById(R.id.tabs);
        new TabLayoutMediator(tabs, viewPager, new TabLayoutMediator.TabConfigurationStrategy() {
            @Override
            public void onConfigureTab(@NonNull TabLayout.Tab tab, int position) {
                switch(position) {
                    case 0:
                        tab.setText("Remotes");
                        break;
                    case 1:
                        tab.setText("Media");
                        break;
                    case 2:
                        tab.setText("Apps");
                        break;
                    case 3:
                        tab.setText("MediaServer");
                        break;
                    default:
                        tab.setText("Foo");
                }
            }
        }).attach();

        FloatingActionButton fab = findViewById(R.id.fab);

        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
                nymphCast.findServers();
            }
        });


        // Load the initial remotes.
		// TODO: Call the init() function in the NymphCast client library here to start the core code.
        //loadRemotes();
		//nymphCast.findServers();
    }

    /* public void loadRemotes() {
        // TODO: load from NymphCast library.
		// TODO:
        RemotesContent.RemoteItem item = new RemotesContent.RemoteItem("0", "0", "Zero");
        RemotesContent.addItem(item);
    } */

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
