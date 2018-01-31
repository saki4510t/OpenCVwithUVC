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

#ifndef FLIGHTDEMO_IPBASE_H
#define FLIGHTDEMO_IPBASE_H

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "opencv2/opencv.hpp"

#define RESULT_FRAME_TYPE_NON 0			// 数値のみ返す
#define RESULT_FRAME_TYPE_SRC 1
#define RESULT_FRAME_TYPE_DST 2
#define RESULT_FRAME_TYPE_SRC_LINE 3
#define RESULT_FRAME_TYPE_DST_LINE 4
#define RESULT_FRAME_TYPE_MAX 5

typedef struct Coeff4 {
	float a, b, c, d;
} Coeff4_t;

#define EPS 1e-8

typedef enum DetectType {
	TYPE_NON = -1,
	TYPE_LINE = 0,
	TYPE_CURVE = 1,
} DetectType_t;

class IPBase {
protected:
	static const cv::Scalar COLOR_YELLOW;
	static const cv::Scalar COLOR_GREEN;
	static const cv::Scalar COLOR_ORANGE;
	static const cv::Scalar COLOR_ACUA;
	static const cv::Scalar COLOR_PINK;
	static const cv::Scalar COLOR_BLUE;
	static const cv::Scalar COLOR_RED;
	static const cv::Scalar COLOR_WHITE;
	static const cv::Scalar COLOR_BLACK;

	IPBase();
	virtual ~IPBase();

	static void clear_stringstream(std::stringstream &ss);
	// call RotatedRect with specific color
	static void draw_rect(cv::Mat img, cv::RotatedRect rect, cv::Scalar color);
	static inline int sign(const double v) {
		return (v > 0) - (v < 0);
	}

};
#endif //FLIGHTDEMO_IPBASE_H
