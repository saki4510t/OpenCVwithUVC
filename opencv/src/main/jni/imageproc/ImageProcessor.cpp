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

#include <jni.h>
#include <stdlib.h>
#include <algorithm>

#include "utilbase.h"
#include "common_utils.h"
#include "JNIHelp.h"
#include "Errors.h"

#include "ImageProcessor.h"

struct fields_t {
    jmethodID callFromNative;
    jmethodID arrayID;
};
static fields_t fields;

using namespace android;

ImageProcessor::ImageProcessor(JNIEnv* env, jobject weak_thiz_obj, jclass clazz)
:	mWeakThiz(env->NewGlobalRef(weak_thiz_obj)),
	mClazz((jclass)env->NewGlobalRef(clazz)),
	mIsRunning(false),
	mResultFrameType(RESULT_FRAME_TYPE_DST_LINE)
{
	ENTER();

	EXIT();
}

ImageProcessor::~ImageProcessor() {
}

void ImageProcessor::release(JNIEnv *env) {
	ENTER();

	if (LIKELY(env)) {
		if (mWeakThiz) {
			env->DeleteGlobalRef(mWeakThiz);
			mWeakThiz = NULL;
		}
		if (mClazz) {
			env->DeleteGlobalRef(mClazz);
			mClazz = NULL;
		}
	}
	clearFrames();

	EXIT();
}

/**
 * request start image processing
 * This is called on gl context and you can execute OpenGL|ES functions
 */
int ImageProcessor::start(const int &width, const int &height) {
	ENTER();
	int result = -1;

	if (!isRunning()) {
		mMutex.lock();
		{
			initFrame(width, height);
			mIsRunning = true;
			result = pthread_create(&processor_thread, NULL, processor_thread_func, (void *)this);
		}
		mMutex.unlock();
	} else {
		LOGW("already running");
	}

	RETURN(result, int);
}

/**
 * request stop image processing
 * This is called on gl context and you can execute OpenGL|ES functions
 */
int ImageProcessor::stop() {
	ENTER();

	bool b = isRunning();
	if (LIKELY(b)) {
		mIsRunning = false;
		releaseFrame();
		mMutex.lock();
		{
			MARK("signal to processor thread");
			mSync.broadcast();
		}
		mMutex.unlock();
		MARK("wait for processing thread finishes");
		if (pthread_join(processor_thread, NULL) != EXIT_SUCCESS) {
			LOGW("terminate processor thread: pthread_join failed");
		}
	}
	clearFrames();

	RETURN(0, int);
}

void ImageProcessor::setResultFrameType(const int &result_frame_type) {
	ENTER();

// if you want to pass some parameters while image processing,
// you should do access control like here.
	Mutex::Autolock lock(mMutex);

	mResultFrameType = result_frame_type % RESULT_FRAME_TYPE_MAX;

	EXIT();
};


/** static member thread function */
/*private*/
void *ImageProcessor::processor_thread_func(void *vptr_args) {
	ENTER();

	ImageProcessor *processor = reinterpret_cast<ImageProcessor *>(vptr_args);
	if (LIKELY(processor)) {
		// attach to JavaVM so that ImageProcessor can call method(s) on Java class
		JavaVM *vm = getVM();
		CHECK(vm);
		JNIEnv *env;
		vm->AttachCurrentThread(&env, NULL);
		CHECK(env);
		processor->do_process(env);
		LOGD("image processing loop finished, detach from JavaVM");
		vm->DetachCurrentThread();
		LOGD("detach finished");
	}

	PRE_EXIT();
	pthread_exit(NULL);
}

/** actual member thread function */
/*private*/
void ImageProcessor::do_process(JNIEnv *env) {
	ENTER();

	cv::Mat src, result;
	long last_queued_time_ms;

	for ( ; mIsRunning ; ) {
		// wait for image
		cv::Mat frame = getFrame(last_queued_time_ms);
		if (UNLIKELY(!mIsRunning)) break;
		if (LIKELY(!frame.empty())) {
			try {
//--------------------------------------------------------------------------------
// local copy
// if you want to pass some parameters while image processing,
// you should do access control like here.
				int result_frame_type;
				mMutex.lock();
				{
					result_frame_type = mResultFrameType;
				}
				mMutex.unlock();
//--------------------------------------------------------------------------------
// do something you want
// for a sample, convert to gray scale and return it as rgba here now.
				switch (result_frame_type) {
				default:
					// convert to gray scale(RGBA->Y)
					cv::cvtColor(frame, src, cv::COLOR_RGBA2GRAY, 1);
					// convert gray scale to rgba(for callback)
					cv::cvtColor(src, result, cv::COLOR_GRAY2RGBA);
					break;
				}
				if (UNLIKELY(!mIsRunning)) break;
//--------------------------------------------------------------------------------
// call method on Java class
				callJavaCallback(env, result, last_queued_time_ms);
//--------------------------------------------------------------------------------
			} catch (cv::Exception e) {
				LOGE("do_process failed:%s", e.msg.c_str());
				continue;
			} catch (...) {
				LOGE("do_process unknown exception:");
				break;
			}
			recycle(frame);
		}
	}

	EXIT();
}

#define RESULT_NUM 20

/*private*/
int ImageProcessor::callJavaCallback(JNIEnv *env, cv::Mat &result, const long &last_queued_time_ms) {
	ENTER();

	float detected[RESULT_NUM];

	if (LIKELY(mIsRunning && fields.callFromNative && mClazz && mWeakThiz)) {
		jfloatArray detected_array = env->NewFloatArray(RESULT_NUM);
		env->SetFloatArrayRegion(detected_array, 0, RESULT_NUM, detected);
		// result image
		jobject buf_frame = env->NewDirectByteBuffer(result.data, result.total() * result.elemSize());
		// call method on Java class
		env->CallStaticVoidMethod(mClazz, fields.callFromNative, mWeakThiz, 0, buf_frame, detected_array);
		env->ExceptionClear();
		if (LIKELY(detected_array)) {
			env->DeleteLocalRef(detected_array);
		}
		if (buf_frame) {
			env->DeleteLocalRef(buf_frame);
		}
	}

	RETURN(0, int);
}

//********************************************************************************
// register native method to Java class
//********************************************************************************
static void nativeClassInit(JNIEnv* env, jclass clazz) {
	ENTER();

	fields.callFromNative = env->GetStaticMethodID(clazz, "callFromNative",
         "(Ljava/lang/ref/WeakReference;ILjava/nio/ByteBuffer;[F)V");
	if (UNLIKELY(!fields.callFromNative)) {
		LOGW("can't find com.serenegiant.ImageProcessor#callFromNative");
	}
	env->ExceptionClear();
    jclass byteBufClass = env->FindClass("java/nio/ByteBuffer");

	if (LIKELY(byteBufClass)) {
		fields.arrayID = env->GetMethodID(byteBufClass, "array", "()[B");
		if (!fields.arrayID) {
			LOGE("Can't find java/nio/ByteBuffer#array");
		}
	} else {
		LOGE("Can't find java/nio/ByteBuffer");
	}
	env->ExceptionClear();

	EXIT();
}

static ID_TYPE nativeCreate(JNIEnv *env, jobject thiz,
	jobject weak_thiz_obj) {

	ImageProcessor *processor = NULL;

	jclass clazz = env->GetObjectClass(thiz);
	if (LIKELY(clazz)) {
		processor = new ImageProcessor(env, weak_thiz_obj, clazz);
		setField_long(env, thiz, "mNativePtr", reinterpret_cast<ID_TYPE>(processor));
	} else {
		jniThrowRuntimeException(env, "can't find com.serenegiant.ImageProcessor");
	}

	RETURN(reinterpret_cast<ID_TYPE>(processor), ID_TYPE);
}

static void nativeRelease(JNIEnv *env, jobject thiz,
	ID_TYPE id_native) {

	ENTER();

	setField_long(env, thiz, "mNativePtr", 0);
	ImageProcessor *processor = reinterpret_cast<ImageProcessor *>(id_native);
	if (LIKELY(processor)) {
		// terminating
		processor->release(env);
		SAFE_DELETE(processor);
	}

	EXIT();
}

static jint nativeStart(JNIEnv *env, jobject thiz,
	ID_TYPE id_native, jint width, jint height) {

	ENTER();

	jint result = -1;
	ImageProcessor *processor = reinterpret_cast<ImageProcessor *>(id_native);
	if (LIKELY(processor)) {
		result = processor->start(width, height);
	}

	RETURN(result, jint);
}

static jint nativeStop(JNIEnv *env, jobject thiz,
	ID_TYPE id_native) {

	ENTER();

	jint result = -1;
	ImageProcessor *processor = reinterpret_cast<ImageProcessor *>(id_native);
	if (LIKELY(processor)) {
		result = processor->stop();
	}

	RETURN(result, jint);
}

static int nativeHandleFrame(JNIEnv *env, jobject thiz,
	ID_TYPE id_native, jint width, jint height, jint tex_name) {

	ENTER();

	jint result = -1;
	ImageProcessor *processor = reinterpret_cast<ImageProcessor *>(id_native);
	if (LIKELY(processor)) {
		result = processor->handleFrame(width, height, tex_name);
	}

	RETURN(result, jint);
}

static jint nativeSetResultFrameType(JNIEnv *env, jobject thiz,
	ID_TYPE id_native, jint result_frame_type) {

	ENTER();

	jint result = -1;
	ImageProcessor *processor = reinterpret_cast<ImageProcessor *>(id_native);
	if (LIKELY(processor)) {
		processor->setResultFrameType(result_frame_type);
		result = 0;
	}

	RETURN(result, jint);
}

static jint nativeGetResultFrameType(JNIEnv *env, jobject thiz,
	ID_TYPE id_native) {

	ENTER();

	jint result = 0;
	ImageProcessor *processor = reinterpret_cast<ImageProcessor *>(id_native);
	if (LIKELY(processor)) {
		result = processor->getResultFrameType();
	}

	RETURN(result, jint);
}

//================================================================================
static JNINativeMethod methods[] = {
	{ "nativeClassInit",			"()V",   (void*)nativeClassInit },
	{ "nativeCreate",				"(Ljava/lang/ref/WeakReference;)J", (void *) nativeCreate },
	{ "nativeRelease",				"(J)V", (void *) nativeRelease },
	{ "nativeStart",				"(JII)I", (void *) nativeStart },
	{ "nativeStop",					"(J)I", (void *) nativeStop },
	{ "nativeHandleFrame",			"(JIII)I", (void *) nativeHandleFrame },
	{ "nativeSetResultFrameType",	"(JI)I", (void *) nativeSetResultFrameType },
	{ "nativeGetResultFrameType",	"(J)I", (void *) nativeGetResultFrameType },
};


int register_ImageProcessor(JNIEnv *env) {
	if (registerNativeMethods(env,
		"com/serenegiant/opencv/ImageProcessor",
		methods, NUM_ARRAY_ELEMENTS(methods)) < 0) {
		return -1;
	}
	return 0;
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
#if LOCAL_DEBUG
    LOGD("JNI_OnLoad");
#endif

    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    // register native methods
    int result = register_ImageProcessor(env);

	setVM(vm);

#if LOCAL_DEBUG
    LOGD("JNI_OnLoad:finished:result=%d", result);
#endif
    return JNI_VERSION_1_6;
}
