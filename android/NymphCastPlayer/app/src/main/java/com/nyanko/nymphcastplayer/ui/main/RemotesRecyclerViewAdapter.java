package com.nyanko.nymphcastplayer.ui.main;

import android.view.MenuItem;
import android.widget.PopupMenu;
import androidx.recyclerview.widget.RecyclerView;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.nyanko.nymphcastplayer.MainActivity;
import com.nyanko.nymphcastplayer.ui.main.RemotesFragment.OnListFragmentInteractionListener;

import com.nyanko.nymphcastplayer.R;
import java.util.List;

/**
 * {@link RecyclerView.Adapter} that can display a {@link RemotesContent.RemoteItem} and makes a call to the
 * specified {@link OnListFragmentInteractionListener}.
 * TODO: Replace the implementation with code for your data type.
 */
public class RemotesRecyclerViewAdapter extends RecyclerView.Adapter<RemotesRecyclerViewAdapter.ViewHolder> {

	private final List<RemotesContent.RemoteItem> mValues;
	private final OnListFragmentInteractionListener mListener;

	public RemotesRecyclerViewAdapter(List<RemotesContent.RemoteItem> items, OnListFragmentInteractionListener listener) {
		mValues = items;
		mListener = listener;
	}

	@Override
	public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
		View view = LayoutInflater.from(parent.getContext())
				.inflate(R.layout.fragment_remotes, parent, false);
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
				openOptionMenu(v, position);
			}
		});
	}

	private void openOptionMenu(View v, final int position){
		PopupMenu popup = new PopupMenu(v.getContext(), v);
		popup.getMenuInflater().inflate(R.menu.remotes_popup_layout, popup.getMenu());
		popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
			@Override
			public boolean onMenuItemClick(MenuItem item) {
				//Toast.makeText(getBaseContext(), "You selected the action : " + item.getTitle()+" position "+position, Toast.LENGTH_SHORT).show();
				switch (item.getItemId()) {
					case R.id.remotes_menu_connect:
						// TODO: Connect to remote.
						// Call the JNI function in the NymphCast library for this.
						boolean res = MainActivity.nymphCast.connectServer(position);

						// Check that we successfully connected.
						if (res) {
							// Update UI.
						}
						else {
							// Display connection error.
						}

						break;
					case R.id.remotes_menu_auto_connect:
						// TODO: Set auto-connect flag in options.
						break;
					case R.id.remotes_menu_info:
						// TODO: Show the info dialogue for this remote.
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
		public RemotesContent.RemoteItem mItem;

		public ViewHolder(View view) {
			super(view);
			mView = view;
			mIdView = (TextView) view.findViewById(R.id.item_number);
			mContentView = (TextView) view.findViewById(R.id.content);
		}

		@Override
		public String toString() {
			return super.toString() + " '" + mContentView.getText() + "'";
		}
	}
}
