package com.nyanko.nymphcastplayer.ui.main;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.viewpager2.adapter.FragmentStateAdapter;

import com.nyanko.nymphcastplayer.R;

/**
 * A [FragmentPagerAdapter] that returns a fragment corresponding to
 * one of the sections/tabs/pages.
 */
public class SectionsPagerAdapter extends FragmentStateAdapter {
    public SectionsPagerAdapter(@NonNull FragmentActivity fa) {
        super(fa);
    }

	@NonNull
	@Override
	public Fragment createFragment(int position) {
		if (position == 0) {
			return RemotesFragment.newInstance(2);
		}
		else if (position == 1) {
			return MediaFragment.newInstance(1);
		}
		else if (position == 2) {
			return AppsFragment.newInstance("foo", "bar");
		}

		return RemotesFragment.newInstance(3);
	}

	@Override
	public int getItemCount() {
		return 3;
	}
}
