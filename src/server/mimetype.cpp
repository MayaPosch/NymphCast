/*
	mimetype.cpp - Maps file extensions to file mime types.
	
	2020/12/09, Maya Posch
*/



#include "mimetype.h"


// Static initialisations.
std::map<std::string, std::string> MimeType::mimes {
	{"*3gpp", "audio/3gpp"},
	{"*jpm", "video/jpm"},
	{"*mp3", "audio/mp3"},
	{"*wav", "audio/wave"},
	{"3g2", "video/3gpp2"},
	{"3gp", "video/3gpp"},
	{"aac", "audio/aac"},
	{"avi", "video/x-msvideo"},
	{"adp", "audio/adpcm"},
	{"adp", "audio/adpcm"},
	{"apng", "image/apng"},
	{"au", "audio/basic"},
	{"bmp", "image/bmp"},
	{"cgm", "image/cgm"},
	{"drle", "image/dicom-rle"},
	{"emf", "image/emf"},
	{"exr", "image/aces"},
	{"fits", "image/fits"},
	{"flac", "audio/flac"},
	{"g3", "image/g3fax"},
	{"gif", "image/gif"},
	{"h261", "video/h261"},
	{"h263", "video/h263"},
	{"h264", "video/h264"},
	{"heic", "image/heic"},
	{"heics", "image/heic-sequence"},
	{"heif", "image/heif"},
	{"heifs", "image/heif-sequence"},
	{"ief", "image/ief"},
	{"jls", "image/jls"},
	{"jp2", "image/jp2"},
	{"jpe", "image/jpeg"},
	{"jpeg", "image/jpeg"},
	{"jpf", "image/jpx"},
	{"jpg", "image/jpeg"},
	{"jpg2", "image/jp2"},
	{"jpgm", "video/jpm"},
	{"jpgv", "video/jpeg"},
	{"jpm", "image/jpm"},
	{"jpx", "image/jpx"},
	{"kar", "audio/midi"},
	{"ktx", "image/ktx"},
	{"mkv", "video/x-matroska"},
	{"m1v", "video/mpeg"},
	{"m2a", "audio/mpeg"},
	{"m2v", "video/mpeg"},
	{"m3a", "audio/mpeg"},
	{"m4a", "audio/mp4"},
	{"mid", "audio/midi"},
	{"midi", "audio/midi"},
	{"mj2", "video/mj2"},
	{"mjp2", "video/mj2"},
	{"mov", "video/quicktime"},
	{"mp2", "audio/mpeg"},
	{"mp2a", "audio/mpeg"},
	{"mp3", "audio/mpeg"},
	{"mp4", "video/mp4"},
	{"mp4a", "audio/mp4"},
	{"mp4v", "video/mp4"},
	{"mpe", "video/mpeg"},
	{"mpeg", "video/mpeg"},
	{"mpg", "video/mpeg"},
	{"mpg4", "video/mp4"},
	{"mpga", "audio/mpeg"},
	{"oga", "audio/ogg"},
	{"ogg", "audio/ogg"},
	{"ogv", "video/ogg"},
	{"opus", "audio/opus"},
	{"png", "image/png"},
	{"qt", "video/quicktime"},
	{"rmi", "audio/midi"},
	{"s3m", "audio/s3m"},
	{"sgi", "image/sgi"},
	{"sil", "audio/silk"},
	{"snd", "audio/basic"},
	{"spx", "audio/ogg"},
	{"svg", "image/svg+xml"},
	{"svgz", "image/svg+xml"},
	{"t38", "image/t38"},
	{"tfx", "image/tiff-fx"},
	{"tif", "image/tiff"},
	{"tiff", "image/tiff"},
	{"ts", "video/mp2t"},
	{"wav", "audio/wav"},
	{"weba", "audio/webm"},
	{"webm", "video/webm"},
	{"webp", "image/webp"},
	{"wmf", "image/wmf"},
	{"xm", "audio/xm"},
	{"m3u", "application/mpegurl"},
	{"m3u8", "application/mpegurl"},
};


// --- GET MIME TYPE ---
std::string MimeType::getMimeType(std::string extension) {
	std::map<std::string, std::string>::const_iterator it;
	it = mimes.find(extension);
	if (it == mimes.end()) { return std::string(); }
	
	return it->second;
}


// --- HAS EXTENSION ---
bool MimeType::hasExtension(std::string extension, uint8_t &type) {
	std::map<std::string, std::string>::const_iterator it;
	it = mimes.find(extension);
	if (it == mimes.end()) { return false; }
	
	//std::string ts = it->second.substr(0, it->second.find('/'));
	std::string ts = it->second.substr(0, 5);
	if (ts == "audio") {		type = 0; }
	else if (ts == "video") {	type = 1; }
	else if (ts == "image") {	type = 2; }
	else if (ts == "appli") {	type = 3; }
	else { return false; }
	
	return true;
}
