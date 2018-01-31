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

NDK_TOOLCHAIN_VERSION := clang
# API >= 18 for OpenGL|ES3
APP_PLATFORM := android-18

# Cコンパイラオプション
APP_CFLAGS += -DHAVE_PTHREADS
APP_CFLAGS += -DNDEBUG					# LOG_ALLを無効にする・assertを無効にする場合
APP_CFLAGS += -DLOG_NDEBUG				# デバッグメッセージを出さないようにする時

# C++コンパイラオプション
APP_CPPFLAGS += -std=c++0x
APP_CPPFLAGS += -fexceptions			# 例外を有効にする
APP_CPPFLAGS += -frtti					# RTTI(実行時型情報)を有効にする

# 最適化設定
APP_CFLAGS += -DAVOID_TABLES
APP_CFLAGS += -O3 -fstrict-aliasing
APP_CFLAGS += -fprefetch-loop-arrays

# 警告を消す設定
APP_CFLAGS += -Wno-parentheses
APP_CFLAGS += -Wno-switch
APP_CFLAGS += -Wno-extern-c-compat
APP_CFLAGS += -Wno-empty-body
APP_CFLAGS += -Wno-deprecated-register
APP_CPPFLAGS += -Wreturn-type
APP_CPPFLAGS += -Wno-multichar

# 出力アーキテクチャ
APP_ABI := armeabi-v7a x86

# STLライブラリ GNU-STLじゃないとリンクできない
APP_STL := gnustl_shared

# 出力オプション
APP_OPTIM := release
