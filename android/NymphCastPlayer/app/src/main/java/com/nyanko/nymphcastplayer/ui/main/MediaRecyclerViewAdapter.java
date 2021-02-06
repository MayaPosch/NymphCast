package com.nyanko.nymphcastplayer.ui.main;

import android.view.MenuItem;
import android.widget.PopupMenu;
import androidx.recyclerview.widget.RecyclerView;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.nyanko.nymphcastplayer.MainActivity;
import com.nyanko.nymphcastplayer.ui.main.MediaFragment.OnListFragmentInteractionListener;
import com.nyanko.nymphcastplayer.ui.main.MediaContent.MediaItem;

import com.nyanko.nymphcastplayer.R;

import java.util.List;

/**
 * {@link RecyclerView.Adapter} that can display a {@link MediaItem} and makes a call to the
 * specified {@link OnListFragmentInteractionListener}.
 */
public class MediaRecyclerViewAdapter extends RecyclerView.Adapter<MediaRecyclerViewAdapter.ViewHolder> {

	private final List<MediaItem> mValues;
	private final OnListFragmentInteractionListener mListener;

	public MediaRecyclerViewAdapter(List<MediaItem> items, OnListFragmentInteractionListener listener) {
		mValues = items;
		mListener = listener;
	}

	@Override
	public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
		View view = LayoutInflater.from(parent.getContext())
				.inflate(R.layout.fragment_media, parent, false);
		return new ViewHolder(view);
	}

	@Override
	public void onBindViewHolder(final ViewHolder holder, int position) {
		holder.mItem = mValues.get(position);
		holder.mIdView.setText(mValues.get(position).id);
		holder.mContentView.setText(mValues.get(position).content);

		holder.mView.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (null != mListener) {
					openOptionMenu(v, position);
				}
			}
		});
	}

	private void openOptionMenu(View v, final int position){
		PopupMenu popup = new PopupMenu(v.getContext(), v);
		popup.getMenuInflater().inflate(R.menu.media_popup_layout, popup.getMenu());
		popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
			@Override
			public boolean onMenuItemClick(MenuItem item) {
				switch (item.getItemId()) {
					case R.id.media_menu_play:
						// Notify the active callbacks interface (the activity, if the
						// fragment is attached to one) that an item has been selected.

						MediaItem mi = MediaContent.ITEMS.get(position);
						mListener.onListFragmentInteraction(mi);
						break;
					case R.id.media_menu_info:
						// TODO: Show information about track.
						break;
					case R.id.media_menu_cancel:
						// Do nothing.
						break;
					default:
						return false;
				}

				return true;
			}
		});

		popup.show();
	}

	@Override
	public int getItemCount() {
		return mValues.size();
	}

	public class ViewHolder extends RecyclerView.ViewHolder {
		public final View mView;
		public final TextView mIdView;
		public final TextView mContentView;
		public MediaItem mItem;

		public ViewHolder(View view) {
			super(view);
			mView = view;
			mIdView = (TextView) view.findViewById(R.id.item_number);
			mContentView = (TextView) view.findViewById(R.id.content);
			//mItem = (TextView) view.findViewById(R.id.mediaid);
		}

		@Override
		public String toString() {
			return super.toString() + " '" + mContentView.getText() + "'";
		}
	}
}
