package com.nyanko.nymphcastplayer.ui.main;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class RemotesContent {
	/**
	 * An array of remote names.
	 */
	public static List<RemotesContent.RemoteItem> ITEMS = new ArrayList<RemotesContent.RemoteItem>();

	/**
	 * A map of sample (dummy) items, by ID.
	 */
	public static final Map<String, RemotesContent.RemoteItem> ITEM_MAP = new HashMap<String, RemotesContent.RemoteItem>();

	public static void addItem(RemotesContent.RemoteItem item) {
		ITEMS.add(item);
		ITEM_MAP.put(item.id, item);
	}

	private static RemotesContent.RemoteItem createItem(int position) {
		return new RemotesContent.RemoteItem(String.valueOf(position), "Item " + position, makeDetails(position));
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
	public static class RemoteItem {
		public final String id;
		public final String content;
		public final String details;

		public RemoteItem(String id, String content, String details) {
			this.id = id;
			this.content = content;
			this.details = details;
		}

		@Override
		public String toString() {
			return content;
		}
	}
}
