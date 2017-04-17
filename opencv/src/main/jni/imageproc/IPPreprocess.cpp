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

#include "IPPreprocess.h"

IPPreprocess::IPPreprocess()
{
	ENTER();

	EXIT();
}

IPPreprocess::~IPPreprocess() {
	ENTER();

	EXIT();
}

/** 映像の前処理 */
/*protected*/
int IPPreprocess::pre_process(cv::Mat &frame, cv::Mat &src, cv::Mat &result, const int &result_frame_type) {

	ENTER();

	int res = 0;

	try {
//		// グレースケールに変換(RGBA->Y)
		cv::cvtColor(frame, src, cv::COLOR_RGBA2GRAY, 1);
		cv::cvtColor(src, result, cv::COLOR_GRAY2RGBA);

	} catch (cv::Exception e) {
		LOGE("pre_process failed:%s", e.msg.c_str());
		res = -1;
	}

    RETURN(res, int);
}
