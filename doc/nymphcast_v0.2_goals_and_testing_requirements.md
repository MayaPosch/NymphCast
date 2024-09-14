# NymphCast v0.2 #

**Goals and Testing Requirements**

## 0. Overview ##

This document covers the second phase of the development of the NymphCast project and its components.

## 1. Goals ##

New features in v0.2 include:

**1.1. NymphCast Server**
- New platforms: Android, Haiku, ESP32.
- Pipewire modules: discovery, creation of NymphCast sinks.
- Subtitle support: all ffmpeg-supported types, instant display after seeking.
- GUI mode: integrate ROM & save files syncing with NCMS.

**1.2. Player**
- New platform: Haiku.
- Local audio & desktop capture and streaming.
- Stream selection & language (audio, subtitle) preferences.

**1.3. Nymphcast MediaServer**
- Announce itself on start.
- Add game ROMs & save file support.
- Sync game ROMs & saves with other NCMS instances, if any.
- Handle duplicate filenames for ROMs/saves. (rename/ignore/copy newer/older)
- Periodic scanning for new content (multimedia, games)
