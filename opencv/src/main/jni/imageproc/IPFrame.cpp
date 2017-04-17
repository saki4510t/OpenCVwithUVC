/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2017 saki t_saki@serenegiant.com
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

#if 1	// デバッグ情報を出さない時は1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
	#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
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
	// glReadPixelsに使うピンポンバッファ用PBOの準備
	pbo_width = width;
	pbo_height = height;
	pbo_size = (GLsizeiptr)width * height * 4;
	// バッファ名を2つ生成
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

/** OpenGL|ESのフレームバッファから映像を取得, Java側の描画スレッドから呼ばれる */
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
		// OpenGLのフレームバッファをMatに読み込んでキューする
		frame = obtainFromPool(pbo_width, pbo_height);
#if USE_PBO
		const int read_ix = pbo_ix; // 今回読み込むPBOのインデックス
		const int next_ix = pbo_ix = (pbo_ix + 1) % 2;	// 読み込み要求するPBOのインデックス
		//　読み込み要求を行うPBOをbind
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[next_ix]);
		// 非同期でPBOへ読み込み要求
		glReadPixels(0, 0, pbo_width, pbo_height, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		// 実際にデータを取得するPBOをbind
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[read_ix]);
		// PBO内のデータにアクセスできるようにマップする
		const uint8_t *read_data = (uint8_t *)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, pbo_size, GL_MAP_READ_BIT);
		if (LIKELY(read_data)) {
			// ここでコピーする
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
// フレームキュー
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

/** フレームキューからフレームを取得, フレームキューが空ならブロックする */
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

/** フレームプールからフレームを取得, フレームプールが空なら新規に生成する  */
/*protected*/
cv::Mat IPFrame::obtainFromPool(const int &width, const int &height) {
	ENTER();

	Mutex::Autolock lock(mPoolMutex);

	cv::Mat frame;
	if (LIKELY(!mPool.empty())) {
		frame = mPool.back();
		mPool.pop_back();
	}
	frame.create(height, width, CV_8UC4);	// XXX rows=height, cols=widthなので注意

	RET(frame);
}

/** フレームプールにフレームを返却する */
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

/** フレームをキューに追加する. キュー内のフレームが最大数を超えたら先頭をプールに返却する */
/*protected*/
int IPFrame::addFrame(cv::Mat &frame) {
	ENTER();

	int result = 0;
	cv::Mat *temp = NULL;

	mFrameMutex.lock();
	{
		last_queued_time_ms = getTimeMilliseconds();
		if (mFrames.size() >= MAX_QUEUED_FRAMES) {
			// キュー中のフレーム数が最大数を超えたので先頭をプールに返却する
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

