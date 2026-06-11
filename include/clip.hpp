#ifndef __CLIP_H__
#define __CLIP_H__

//#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <deque>

bool GetBits( UINT8 CF_FORMAT );
bool getLatestImage();

bool bitmapToImage( PBITMAPINFO bInfo );
void imageWriteThread();
void togglePFullscreen();

extern int system_screen_width;
extern int system_screen_height;

extern long update_interval;

extern bool fullscreen_only;
extern bool spawned_imageThread;
extern std::deque<PBITMAPINFO> BInfo_Queue;

#endif