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

#ifndef FLIGHTDEMO_IMAGEPROCESSOR_H
#define FLIGHTDEMO_IMAGEPROCESSOR_H
#endif //FLIGHTDEMO_IMAGEPROCESSOR_H

#include "Mutex.h"
#include "Condition.h"
#include "IPBase.h"
#include "IPFrame.h"

using namespace android;

class ImageProcessor : virtual public IPFrame {
private:
	jobject mWeakThiz;
	jclass mClazz;
	volatile bool mIsRunning;
	int mResultFrameType;

	mutable Mutex mMutex;
	Condition mSync;
	pthread_t processor_thread;
	static void *processor_thread_func(void *vptr_args);
	void do_process(JNIEnv *env);
	int callJavaCallback(JNIEnv *env, cv::Mat &result, const long &last_queued_time_ms);
protected:
public:
	ImageProcessor(JNIEnv* env, jobject weak_thiz_obj, jclass clazz);
	virtual ~ImageProcessor();
	void release(JNIEnv *env);
	int start(const int &width, const int &height);
	int stop();
	inline const bool isRunning() const { return mIsRunning; };
	void setResultFrameType(const int &result_frame_type);
	inline const int getResultFrameType() const { return mResultFrameType; };
};