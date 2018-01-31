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

#include "utilbase.h"

#include "IPBase.h"

const cv::Scalar IPBase::COLOR_YELLOW = cv::Scalar(255, 255, 0);
const cv::Scalar IPBase::COLOR_GREEN = cv::Scalar(0, 255, 0);
const cv::Scalar IPBase::COLOR_ORANGE = cv::Scalar(255, 127, 0);
const cv::Scalar IPBase::COLOR_ACUA = cv::Scalar(0, 255, 255);
const cv::Scalar IPBase::COLOR_PINK = cv::Scalar(255, 127, 255);
const cv::Scalar IPBase::COLOR_BLUE = cv::Scalar(0, 0, 255);
const cv::Scalar IPBase::COLOR_RED = cv::Scalar(255, 0, 0);
const cv::Scalar IPBase::COLOR_WHITE = cv::Scalar(255, 255, 255);
const cv::Scalar IPBase::COLOR_BLACK = cv::Scalar(0, 0, 0);

IPBase::IPBase() {
	ENTER();

	EXIT();
}

IPBase::~IPBase() {
	ENTER();

	EXIT();
}

// stringstreamをクリアして再利用できるようにする
void IPBase::clear_stringstream(std::stringstream &ss) {
	static const std::string empty_string;

	ss.str(empty_string);
	ss.clear();
	ss << std::dec;     // clear()でも元に戻らないので、毎回指定する。
}

// RotatedRectを指定線色で描画する
void IPBase::draw_rect(cv::Mat img, cv::RotatedRect rect, cv::Scalar color) {
	cv::Point2f vertices[4];
	rect.points(vertices);
	for (int i = 0; i < 4; i++) {
		cv::line(img, vertices[i], vertices[(i+1)%4], color);
	}
}
