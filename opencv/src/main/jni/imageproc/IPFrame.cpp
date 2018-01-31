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

#if 1	// set 0 to enable debug output, otherwise set 1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// disable LOGV/LOGD/MARK
	#endif
	#undef USE_LOGALL			// enable specific LOGx only
#else
//	#define USE_LOGALL
	#define USE_LOGD
	#undef LOG_NDEBUG
	#undef NDEBUG
#endif

#include "utilbase.h"

#include "IPFrame.h"

#include "common_utils.h"

#define USE_PBO 1

IPFrame::IPFrame()
: pbo_ix(0), pbo_width(0), pbo_height(0), pbo_size(0) {

	ENTER();

#if USE_PBO
	pbo[0] = pbo[1] = 0;
#endif

	EXIT();
}

IPFrame::~IPFrame() {
	ENTER();

	clearFrames();

	EXIT();
}

//================================================================================
//
//================================================================================
void IPFrame::initFrame(const int &width, const int &height) {
	ENTER();

	Mutex::Autolock lock(mPboMutex);

#if USE_PBO
	// prepare PBOs for glReadPixels
	pbo_width = width;
	pbo_height = height;
	pbo_size = (GLsizeiptr)width * height * 4;
	// generate 2 PBO
	glGenBuffers(2, pbo);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[0]);
	glBufferData(GL_PIXEL_PACK_BUFFER, pbo_size, NULL, GL_DYNAMIC_READ);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[1]);
	glBufferData(GL_PIXEL_PACK_BUFFER, pbo_size, NULL, GL_DYNAMIC_READ);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	pbo_ix = 0;
#endif

	EXIT();
}

void IPFrame::releaseFrame() {
	ENTER();

	LOGI("");
#if USE_PBO
	mPboMutex.lock();
	{
		pbo_size = 0;
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		glDeleteBuffers(2, pbo);
		pbo[0] = pbo[1] = 0;
		pbo_width = pbo_height = 0;
	}
	mPboMutex.unlock();
#endif

	mFrameMutex.lock();
	{
		mFrameSync.broadcast();
	}
	mFrameMutex.unlock();

	LOGI("finished");

	EXIT();
}

/** get image from frame buffer of OpenGL|ES, call this on drawing thread of Java */
/*public*/
int IPFrame::handleFrame(const int &, const int &, const int &) {
	ENTER();

	cv::Mat frame;
	mPboMutex.lock();
	{
		if (UNLIKELY(!pbo_size)) {
			mPboMutex.unlock();
			RETURN(1, int);	// dropped
		}
		// re-use cv::Mat
		frame = obtainFromPool(pbo_width, pbo_height);
#if USE_PBO
		// index of PBO that read now
		const int read_ix = pbo_ix;
		// index of PBO to request read(will actually read into memory on next frame
		const int next_ix = pbo_ix = (pbo_ix + 1) % 2;
		//　bind PBO to read asynchronously
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[next_ix]);
		// request asynchronously read frame buffer into PBO
		glReadPixels(0, 0, pbo_width, pbo_height, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		// bind PBO to read image data from PBO
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[read_ix]);
		// map PBO
		const uint8_t *read_data = (uint8_t *)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, pbo_size, GL_MAP_READ_BIT);
		if (LIKELY(read_data)) {
			// copy PBO into memory
			memcpy(frame.data, read_data, pbo_size);
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		}
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
#else
		glReadPixels(0, 0, pbo_width, pbo_height, GL_RGBA, GL_UNSIGNED_BYTE, frame.data);
#endif
	}
	mPboMutex.unlock();

	if (LIKELY(canAddFrame())) {
		addFrame(frame);
	} else {
		recycle(frame);
	}

	RETURN(0, int);
}

//================================================================================
// frame queue
//================================================================================
/*protected*/
void IPFrame::clearFrames() {
	ENTER();

	mPoolMutex.lock();
	{
		mPool.clear();
	}
	mPoolMutex.unlock();

	mFrameMutex.lock();
	{
		for ( ; !mFrames.empty(); ) {
			mFrames.pop();
		}
	}
	mFrameMutex.unlock();

	EXIT();
}

/** get frame, if frame queue is empty, block until ready */
/*protected*/
cv::Mat IPFrame::getFrame(long &last_queued_ms) {
	ENTER();

	cv::Mat result;

	Mutex::Autolock lock(mFrameMutex);

	if (UNLIKELY(mFrames.empty())) {
		mFrameSync.wait(mFrameMutex);
	}
	if (pbo_size && !mFrames.empty()) {
		result = mFrames.front();
		mFrames.pop();
		last_queued_ms = last_queued_time_ms;
	}

	RET(result);
}

/** obtain empty frame(cv::Mat) from frame pool,
  * if frame pool is empty, generate new one  */
/*protected*/
cv::Mat IPFrame::obtainFromPool(const int &width, const int &height) {
	ENTER();

	Mutex::Autolock lock(mPoolMutex);

	cv::Mat frame;
	if (LIKELY(!mPool.empty())) {
		frame = mPool.back();
		mPool.pop_back();
	}
	frame.create(height, width, CV_8UC4);	// XXX Note: rows=height, cols=width, as 8 bits RGBA

	RET(frame);
}

/** return frame into frame pool to recycle/re-use */
/*protected*/
void IPFrame::recycle(cv::Mat &frame) {
	ENTER();

	if (LIKELY(!frame.empty())) {
		Mutex::Autolock lock(mPoolMutex);

		if (mPool.size() < MAX_POOL_SIZE) {
			mPool.push_back(frame);
		}
	}

	EXIT();
}

/** append frame to frame queue.
  * if number of frames in frame queue exceeds limit,
  * return top of the frame(s) to frame pool */
/*protected*/
int IPFrame::addFrame(cv::Mat &frame) {
	ENTER();

	int result = 0;
	cv::Mat *temp = NULL;

	mFrameMutex.lock();
	{
		last_queued_time_ms = getTimeMilliseconds();
		if (mFrames.size() >= MAX_QUEUED_FRAMES) {
			// return to frame pool
			temp = &mFrames.front();
			mFrames.pop();
		}
		mFrames.push(frame);
		mFrameSync.signal();
	}
	mFrameMutex.unlock();

	if (UNLIKELY(temp)) {
		recycle(*temp);
		result = -1;
	}

	RETURN(result, int);
}

