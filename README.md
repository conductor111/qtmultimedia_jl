# qtmultimedia_jl

This modified version of qtmulitimedia allows:
    
    1. Use gstreamer as video-backend for qtmultimedia on Windows platform.
    2. Use QMediaPlayer as QMediaObject for QMediaRecorder (it's not implemented in base Qt version).
    3. Create video from image sequence (from any source - files or capture device) to play and record (play and record video created from image sequence).

This version is for Visual Studio 2015 (Visual Studio 2017 is not supported). Base version of Qt library is 5.9.6. gstreamer-1.0-x86_64-1.14.3 was used. Windows platform only.
    