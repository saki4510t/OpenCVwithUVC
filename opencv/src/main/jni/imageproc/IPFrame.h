/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2018 saki t_saki@serenegiant.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * All files in the folder are under this Apache License, Version 2.0.
 * Files in the jni/libjpeg, jni/libusb, jin/libuvc, jni/rapidjson, opencv3
 * folder may have a different license, see the respective files.
*/

#ifndef FLIGHTDEMO_IPFRAME_H
#define FLIGHTDEMO_IPFRAME_H

#include <queue>
#include <vector>
#include <GLES3/gl3.h>		// API>=18
#include <GLES3/gl3ext.h>	// API>=18
#include "opencv2/opencv.hpp"

#include "Mutex.h"
#include "Condition.h"

// max number of frames in frame queue
#define MAX_QUEUED_FRAMES 1
// max number of frames in frame pool
#define MAX_POOL_SIZE 2

using namespace android;

class IPFrame {
private:
	mutable Mutex mPboMutex;
	mutable Mutex mFrameMutex;
	mutable Mutex mPoolMutex;
	Condition mFrameSync;
	// frame pool
	std::vector<cv::Mat> mPool;
	// frame queue
	std::queue<cv::Mat> mFrames;
	long last_queued_time_ms;
	// PBO name for asynchronous call of glReadPixels
	GLuint pbo[2];
	int pbo_ix;
	int pbo_width, pbo_height;
	volatile GLsizeiptr pbo_size;
protected:
	IPFrame();
	virtual ~IPFrame();
	void initFrame(const int &width, const int &height);
	void releaseFrame();

	cv::Mat getFrame(long &last_queued_ms);
	cv::Mat obtainFromPool(const int &width, const int &height);
	void recycle(cv::Mat &frame);
	inline const bool canAddFrame() { Mutex::Autolock lock(mFrameMutex);  return mFrames.size() < MAX_QUEUED_FRAMES; };
	int addFrame(cv::Mat &frame);
	void clearFrames();

	inline const int width() const { return pbo_width; };
	inline const int height() const { return pbo_height; };
public:
	int handleFrame(const int &, const int &, const int &);
};
#endif //FLIGHTDEMO_IPFRAME_H
