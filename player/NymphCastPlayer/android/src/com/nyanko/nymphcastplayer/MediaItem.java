package com.nyanko.nymphcastplayer


public static class MediaItem {
	private final String title;
	private final String album;
	private final String artist;
	private final String path;

	public MediaItem(String title, String album, String artist, String path) {
		this.title = title;
		this.album = album;
		this.artist = artist;
		this.path = path;
	}
	
	public String getTitle() { return title; }
	public String getAlbum() { return album; }
	public String getArtist() { return artist; }
	public String getPath() { return path; }
}