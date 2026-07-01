#ifndef __CLIP_H__
#define __CLIP_H__

//#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <deque>
#include <vector>
#include <opencv2/opencv.hpp>

extern int system_screen_width;
extern int system_screen_height;
extern long update_interval;

extern bool fullscreen_only;
extern bool autoSave;
extern bool spawned_imageThread;
extern bool savedImageArray;

extern cv::Mat image_array;
extern std::deque<std::vector<BYTE>> BInfo_Queue;

bool GetBits( UINT8 CF_FORMAT );
bool getLatestImage();
bool bitmapToImage( std::vector<BYTE> &dib );

void imageWriteThread();
void writeImageToFile();

void togglePFullscreen();
void togglePAutosave();
void showClipImage();
#endif