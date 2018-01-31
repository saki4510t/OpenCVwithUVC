#/*
# * UVCCamera
# * library and sample to access to UVC web camera on non-rooted Android device
# *
# * Copyright (c) 2014-2018 saki t_saki@serenegiant.com
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# *  You may obtain a copy of the License at
# *
# *     http://www.apache.org/licenses/LICENSE-2.0
# *
# *  Unless required by applicable law or agreed to in writing, software
# *  distributed under the License is distributed on an "AS IS" BASIS,
# *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# *  See the License for the specific language governing permissions and
# *  limitations under the License.
# *
# * All files in the folder are under this Apache License, Version 2.0.
# * Files in the jni/libjpeg, jni/libusb, jin/libuvc, jni/rapidjson, opencv3 folder
# * may have a different license, see the respective files.
#*/

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

OPENCV_CAMERA_MODULES:=off
OPENCV_INSTALL_MODULES:=on
OPENCV_LIB_TYPE:=SHARED

# OpenCV3 (3.20)を使う
include $(LOCAL_PATH)/../opencv3/OpenCV.mk
LOCAL_SHARED_LIBRARIES += opencv_java3

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/ \

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
#マクロ定義
LOCAL_CFLAGS += -DANDROID_NDK
LOCAL_CFLAGS += -DNDEBUG							# LOG_ALLを無効にする・assertを無効にする場合
LOCAL_CFLAGS += -DLOG_NDEBUG						# デバッグメッセージを出さないようにする時
#LOCAL_CFLAGS += -DUSE_LOGALL						# define USE_LOGALL macro to enable all debug string

# public関数のみエクスポートする
LOCAL_CFLAGS += -Wl,--version-script,ImageProcessor.map

LOCAL_LDLIBS += -L$(SYSROOT)/usr/lib -ldl	# to avoid NDK issue(no need for static library)
LOCAL_LDLIBS += -llog
LOCAL_LDLIBS += -lz							# zlib これを入れとかんとOpenCVのリンクに失敗する
LOCAL_LDLIBS += -lm
#LOCAL_LDLIBS += -lEGL -lGLESv1_CM			# OpenGL|ES 1.1ライブラリ
#LOCAL_LDLIBS += -lEGL -lGLESv2				# OpenGL|ES 2.0ライブラリ
LOCAL_LDLIBS += -lEGL -lGLESv3				# OpenGL|ES 2.0|ES 3ライブラリ

LOCAL_SHARED_LIBRARIES += common

LOCAL_SRC_FILES := \
	IPBase.cpp \
	IPFrame.cpp \
	ImageProcessor.cpp \

LOCAL_ARM_MODE := arm
LOCAL_MODULE := imageproc
include $(BUILD_SHARED_LIBRARY)

