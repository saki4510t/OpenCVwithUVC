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

#define LOG_TAG "utils"
#ifndef LOG_NDEBUG
#define	LOG_NDEBUG
#endif
#undef USE_LOGALL

#include "utilbase.h"
#include "common_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/**
 * get current time millis
 */
long getTimeMilliseconds(void) {
    struct timeval  now;
    gettimeofday(&now, NULL);
    return (long)(now.tv_sec * 1000 + now.tv_usec / 1000);
}

/**
 * get elapsed time as microseconds from previous call
 * @param env
 */
float getDeltaTimeMicroseconds(clock_t &prev_time) {
	clock_t current = clock();
	clock_t delta = current - prev_time;
	prev_time = current;
	return (delta * 1000000.f) / CLOCKS_PER_SEC;
}

/**
 * return whether or not the specified field is null
 * @param env: this param should not be null
 * @param java_obj: this param should not be null
 * @param field_name
 * @param field_type
 * @return return true if the field does not exist or the field value is null or the filed type is wrong
 */
bool isNullField(JNIEnv *env, jobject java_obj, const char *field_name, const char *field_type) {
	LOGV("isNullField:");

	jclass clazz = env->GetObjectClass(java_obj);

	jfieldID id = env->GetFieldID(clazz, field_name, field_type);
	bool is_null = false;
	if (LIKELY(id)) {
		jobject obj = env->GetObjectField(java_obj, id);
		if (!obj) {
			LOGD("isNullField:field %s(type=%s) is NULL!", field_name, field_type);
			is_null = true;
		}
#ifdef ANDROID_NDK
		env->DeleteLocalRef(obj);
#endif
	} else {
		LOGD("isNullField:fieldname '%s' not found or fieldtype '%s' mismatch", field_name, field_type);
		env->ExceptionClear();	// clear java.lang.NoSuchFieldError exception
		is_null = true;
	}
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return is_null;
}

/**
 * return a field value as boolean
 * @param env: this param should not be null
 * @param java_obj: this param should not be null
 * @return	return the value, return false(0) if the field does not exist.
 */
bool getField_bool(JNIEnv *env, jobject java_obj, const char *field_name) {
	LOGV("getField_bool:");

	jclass clazz = env->GetObjectClass(java_obj);

	jfieldID id = env->GetFieldID(clazz, field_name, "Z");
	bool val;
	if (LIKELY(id))
		val = env->GetBooleanField(java_obj, id);
	else {
		LOGE("getField_bool:field '%s' not found", field_name);
		env->ExceptionClear();	// clear java.lang.NoSuchFieldError exception
		val = false;
	}
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return val;
}

/**
 * set the value into int field
 * @param env: this param should not be null
 * @param java_obj: this param should not be null
 * @param field_name
 * @params val
 */
bool setField_bool(JNIEnv *env, jobject java_obj, const char *field_name, bool val) {
	LOGV("setField_bool:");

	jclass clazz = env->GetObjectClass(java_obj);
	jfieldID id = env->GetFieldID(clazz, field_name, "Z");
	if (LIKELY(id))
		env->SetBooleanField(java_obj, id, val);
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return val;
}

/**
 * return the static int field value
 * @param env: this param should not be null
 * @param java_obj: this param should not be null
 * @return	return the value, return 0 if the field does not exist.
 */
jint getStaticField_int(JNIEnv *env, jobject java_obj, const char *field_name) {
	LOGV("getField_int:");

	jclass clazz = env->GetObjectClass(java_obj);

	jfieldID id = env->GetStaticFieldID(clazz, field_name, "I");
	jint val;
	if (LIKELY(id))
		val = env->GetStaticIntField(clazz, id);
	else {
		LOGE("getField_int:field '%s' not found", field_name);
		env->ExceptionClear();	// clear java.lang.NoSuchFieldError exception
		val = 0;
	}
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return val;
}

/**
 * set specified value into the static int field
 * @param env: this param should not be null
 * @param java_obj: this param should not be null
 * @param field_name
 * @params val
 */
jint setStaticField_int(JNIEnv *env, jobject java_obj, const char *field_name, jint val) {
	LOGV("setField_int:");

	jclass clazz = env->GetObjectClass(java_obj);

	jfieldID id = env->GetStaticFieldID(clazz, field_name, "I");
	if (LIKELY(id))
		env->SetStaticIntField(clazz, id, val);
	else {
		LOGE("setField_int:field '%s' not found", field_name);
		env->ExceptionClear();	// clear java.lang.NoSuchFieldError exception
	}
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return val;
}

/**
 * get int field that has specified name from specified Java object
 * @param env: this param should not be null
 * @param java_obj: this param should not be null
 * @return retun the value, return 0 if the field does not exist
 */
jint getField_int(JNIEnv *env, jobject java_obj, const char *field_name) {
	LOGV("getField_int:");

	jclass clazz = env->GetObjectClass(java_obj);
	jint val = __getField_int(env, java_obj, clazz, field_name);
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return val;
}

/**
 * @param env: this param should not be null
 * @param java_obj: this param should not be null
 */
jint __getField_int(JNIEnv *env, jobject java_obj, jclass clazz, const char *field_name) {
	LOGV("__getField_int:");

	jfieldID id = env->GetFieldID(clazz, field_name, "I");
	jint val;
	if (LIKELY(id))
		val = env->GetIntField(java_obj, id);
	else {
		LOGE("__getField_int:field '%s' not found", field_name);
		env->ExceptionClear();	// clear java.lang.NoSuchFieldError exception
		val = 0;
	}
	return val;
}

/**
 * set the value into int field
 * @param env: this param should not be null
 * @param java_obj: this param should not be null
 * @param field_name
 * @params val
 */
jint setField_int(JNIEnv *env, jobject java_obj, const char *field_name, jint val) {
	LOGV("setField_int:");

	jclass clazz = env->GetObjectClass(java_obj);
	__setField_int(env, java_obj, clazz, field_name, val);
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return val;
}

/**
 * @param env: this param should not be null
 * @param bullet_obj: this param should not be null
 */
jint __setField_int(JNIEnv *env, jobject java_obj, jclass clazz, const char *field_name, jint val) {
	LOGV("__setField_int:");

	jfieldID id = env->GetFieldID(clazz, field_name, "I");
	if (LIKELY(id))
		env->SetIntField(java_obj, id, val);
	else {
		LOGE("__setField_int:field '%s' not found", field_name);
		env->ExceptionClear();	// clear java.lang.NoSuchFieldError exception
	}
	return val;
}

/**
 * return the long field value
 * @param env: this param should not be null
 * @param bullet_obj: this param should not be null
 * @return retun the value, return 0 if the field does not exist
  */
jlong getField_long(JNIEnv *env, jobject java_obj, const char *field_name) {
	LOGV("getField_long:");

	jclass clazz = env->GetObjectClass(java_obj);
	jlong val = __getField_long(env, java_obj, clazz, field_name);
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return val;
}

/**
 * @param env: this param should not be null
 * @param bullet_obj: this param should not be null
 */
jlong __getField_long(JNIEnv *env, jobject java_obj, jclass clazz, const char *field_name) {
	LOGV("__getField_long:");

	jfieldID id = env->GetFieldID(clazz, field_name, "J");
	jlong val;
	if (LIKELY(id))
		val = env->GetLongField(java_obj, id);
	else {
		LOGE("__getField_long:field '%s' not found", field_name);
		env->ExceptionClear();	// clear java.lang.NoSuchFieldError exception
		val = 0;
	}
	return val;
}

/**
 * set the value into the long field
 * @param env: this param should not be null
 * @param bullet_obj: this param should not be null
 * @param field_name
 * @params val
 */
jlong setField_long(JNIEnv *env, jobject java_obj, const char *field_name, jlong val) {
	LOGV("setField_long:");

	jclass clazz = env->GetObjectClass(java_obj);
	__setField_long(env, java_obj, clazz, field_name, val);
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return val;
}

/**
 * @param env: this param should not be null
 * @param bullet_obj: this param should not be null
 */
jlong __setField_long(JNIEnv *env, jobject java_obj, jclass clazz, const char *field_name, jlong val) {
	LOGV("__setField_long:");

	jfieldID field = env->GetFieldID(clazz, field_name, "J");
	if (LIKELY(field))
		env->SetLongField(java_obj, field, val);
	else {
		LOGE("__setField_long:field '%s' not found", field_name);
		env->ExceptionClear();	// clear java.lang.NoSuchFieldError exception
	}
	return val;
}

/**
 * return the static float field value
 * @param env: this param should not be null
 * @param java_obj: this param should not be null
 * @param field_name
 * @return retun the value, return 0 if the field does not exist
 */
jfloat getStaticField_float(JNIEnv *env, jobject java_obj, const char *field_name) {
	LOGV("getField_float:");

	jclass clazz = env->GetObjectClass(java_obj);

	jfieldID id = env->GetStaticFieldID(clazz, field_name, "F");
	jfloat val;
	if (LIKELY(id))
		val = env->GetStaticFloatField(clazz, id);
	else {
		LOGE("getStaticField_float:field '%s' not found", field_name);
		env->ExceptionClear();	// clear java.lang.NoSuchFieldError exception
		val = 0.f;
	}
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return val;
}

/**
 * set the value into static float field
 * @param env: this param should not be null
 * @param bullet_obj: this param should not be null
 * @param field_name
 */
jfloat setStaticField_float(JNIEnv *env, jobject java_obj, const char *field_name, jfloat val) {
	LOGV("getField_float:");

	jclass clazz = env->GetObjectClass(java_obj);

	jfieldID id = env->GetStaticFieldID(clazz, field_name, "F");
	if (LIKELY(id))
		env->SetStaticFloatField(clazz, id, val);
	else {
		LOGE("setStaticField_float:field '%s' not found", field_name);
		env->ExceptionClear();	// clear java.lang.NoSuchFieldError exception
	}
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return val;
}

/**
 * return the value of float field
 * @param env: this param should not be null
 * @param bullet_obj: this param should not be null
 * @param field_name
 * @return retun the value, return 0 if the field does not exist
 */
jfloat getField_float(JNIEnv *env, jobject java_obj, const char *field_name) {
	LOGV("getField_float:");

	jclass clazz = env->GetObjectClass(java_obj);

	jfloat val = __getField_float(env, java_obj, clazz, field_name);
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return val;
}

/**
 * @param env: this param should not be null
 * @param bullet_obj: this param should not be null
 */
jfloat __getField_float(JNIEnv *env, jobject java_obj, jclass clazz, const char *field_name) {
	LOGV("__getField_float:");

	jfieldID id = env->GetFieldID(clazz, field_name, "F");
	jfloat val;
	if ((id))
		val = env->GetFloatField(java_obj, id);
	else {
		LOGE("__getField_float:field '%s' not found", field_name);
		env->ExceptionClear();	// clear java.lang.NoSuchFieldError exception
		val = 0.f;
	}
	return val;
}

/**
 * set the value into the float field
 * @param env: this param should not be null
 * @param bullet_obj: this param should not be null
 * @param field_name
 * @params val
 */
jfloat setField_float(JNIEnv *env, jobject java_obj, const char *field_name, jfloat val) {
	LOGV("setField_Float:");

	jclass clazz = env->GetObjectClass(java_obj);
	__setField_float(env, java_obj, clazz, field_name, val);
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return val;
}

/**
 * @param env: this param should not be null
 * @param bullet_obj: this param should not be null
 */
jfloat __setField_float(JNIEnv *env, jobject java_obj, jclass clazz, const char *field_name, jfloat val) {
	LOGV("__setField_Float:");

	jfieldID field = env->GetFieldID(clazz, field_name, "F");
	if (LIKELY(field))
		env->SetFloatField(java_obj, field, val);
	else {
		LOGE("__setField_Float:field '%s' not found", field_name);
		env->ExceptionClear();	// clear java.lang.NoSuchFieldError exception
	}
	return val;
}

/**
 * return jobject filed that is specified type from specified field.
 * you should check the field exist and is not null with #isNullField
 * before you call this function.
 * @param env: this param should not be null
 * @param bullet_obj: this param should not be null
 * @param field_name
 * @param field_type
 * @return jobject
 */
jobject getField_obj(JNIEnv *env, jobject java_obj, const char *field_name, const char *field_type) {
	LOGV("getField_obj:");

	jclass clazz = env->GetObjectClass(java_obj);

	jobject obj = NULL;
	jfieldID id = env->GetFieldID(clazz, field_name, field_type);
	if (LIKELY(id)) {
		obj = env->GetObjectField(java_obj, id);
	} else {
		LOGE("getField_obj:field object %s(type='%s') not found", field_name, field_type);
		env->ExceptionClear();	// clear java.lang.NoSuchFieldError exception
	}
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return obj;
}


jobject getStaticField_obj(JNIEnv *env, jobject java_obj, const char *field_name, const char *field_type) {
	LOGV("getField_obj:");

	jclass clazz = env->GetObjectClass(java_obj);

	jobject obj = NULL;
	jfieldID id = env->GetStaticFieldID(clazz, field_name, field_type);
	if (LIKELY(id)) {
		obj = env->GetStaticObjectField(clazz, id);
	} else {
		LOGE("getStaticField_obj:field object %s(type='%s') not found", field_name, field_type);
		env->ExceptionClear();	// clear java.lang.NoSuchFieldError exception
	}
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return obj;
}

int prepareBytebufferId(JNIEnv *env, jclass &byteBufClass, jmethodID &byteBufArrayID) {
	// ByteBufferがdirectBufferでない時にJava側からbyte[]を取得するためのメソッドidを取得
	byteBufClass = env->FindClass("java/nio/ByteBuffer");
	if (!byteBufClass) {
		LOGE("Can't find java/nio/ByteBuffer");
		env->ExceptionClear();
		return -1;
	}
    byteBufArrayID = env->GetMethodID(byteBufClass, "array", "()[B");
	if (!byteBufArrayID) {
		LOGE("Can't find java/nio/ByteBuffer#array");
		env->ExceptionClear();
		return -1;
	}
	return 0;
}

/**
 * JavaのByteBufferインスタンスからbyte配列をコピーする。
 * byteBufClassとbyteBufArrayIDはprepareBytebufferIdで取得しておくこと
 * @param byte_buffer_obj JavaのByteBufferインスタンス
 * @param dst_buf コピー先の配列, nullならサイズを取得するだけ
 * @param size 配列のサイズ[バイト数]
 * @param byteBufClass ByteArrayがnative bufferでは無かった時の配列アクセス用
 * @param byteBufArrayID ByteArrayがnative bufferでは無かった時の配列アクセス用
 */
jlong getByteBuffer(JNIEnv *env, jobject byte_buffer_obj, void *dst_buf, jint _offset, jint _size, jclass byteBufClass, jmethodID byteBufArrayID) {

	jlong size = 0;
	uint8_t *buf = (uint8_t *)env->GetDirectBufferAddress(byte_buffer_obj);
	jbyteArray byteArray = NULL;
	if (LIKELY(buf)) {
		size = env->GetDirectBufferCapacity(byte_buffer_obj);
	} else {
		// 引数のByteBufferがnative bufferでは無かった時
		// ByteBuffer#arrayを呼び出して内部のbyte[]を取得できるかどうか試みる
		byteArray = (jbyteArray)env->CallObjectMethod(byte_buffer_obj, byteBufArrayID);
		if LIKELY(byteArray != NULL) {
	        buf = (uint8_t *)env->GetByteArrayElements(byteArray, NULL);
	        size = env->GetArrayLength(byteArray);
		} else {
			// byte[]を取得できなかった時
			LOGE("byteArray is null");
		}
	}
	if (UNLIKELY(size - _offset < _size)) {
        LOGE("nativeWritePacket saw wrong dstSize=%lld,offset=%d,size=%d", size, _offset, _size);
        _size = size - _offset;
	}
	if (LIKELY(buf)) {
		if (dst_buf && (size > 0) && (_size > 0)) {
			memcpy(dst_buf, &buf[_offset], _size);
		}
	    if (byteArray != NULL) {
	        env->ReleaseByteArrayElements(byteArray, (jbyte *)buf, 0);
	    }
	} else {
		LOGE("getByteBuffer failed");
	}
	return _size;
}

jint registerNativeMethods(JNIEnv* env, const char *class_name, JNINativeMethod *methods, int num_methods) {
	int result = 0;

	jclass clazz = env->FindClass(class_name);
	env->ExceptionClear();
	if (LIKELY(clazz)) {
		int result = env->RegisterNatives(clazz, methods, num_methods);
		if (UNLIKELY(result < 0)) {
			LOGE("registerNativeMethods failed(class=%s)", class_name);
//			env->ExceptionClear();
		}
	} else {
		LOGE("registerNativeMethods: class'%s' not found", class_name);
//		env->ExceptionClear();
	}
	return result;
}

static JavaVM *savedVm;
void setVM(JavaVM *vm) {
	savedVm = vm;
}

JavaVM *getVM() {
	return savedVm;
}

JNIEnv *getEnv() {
    JNIEnv *env = NULL;
    if (savedVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
    	env = NULL;
    }
    return env;
}
