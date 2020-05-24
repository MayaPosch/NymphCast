package com.nyanko.nymphcastplayer.ui.main;

import android.net.Uri;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MediaContent {
	/**
	 * An array of sample (dummy) items.
	 */
	public static final List<MediaItem> ITEMS = new ArrayList<MediaItem>();

	/**
	 * A map of sample (dummy) items, by ID.
	 */
	public static final Map<String, MediaItem> ITEM_MAP = new HashMap<String, MediaItem>();

	private static void addItem(MediaItem item) {
		ITEMS.add(item);
		ITEM_MAP.put(item.id, item);
	}

	private static String makeDetails(int position) {
		StringBuilder builder = new StringBuilder();
		builder.append("Details about Item: ").append(position);
		for (int i = 0; i < position; i++) {
			builder.append("\nMore details information here.");
		}
		return builder.toString();
	}

	/**
	 * An item representing a piece of content.
	 */
	public static class MediaItem {
		public final String id;
		public final String content;
		public final String details;
		public final Uri uri;

		public MediaItem(String id, String content, String details, Uri uri) {
			this.id = id;
			this.content = content;
			this.details = details;
			this.uri = uri;
		}

		@Override
		public String toString() {
			return content;
		}
	}
}
