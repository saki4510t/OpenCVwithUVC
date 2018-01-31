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

#ifndef UTILS_H_
#define UTILS_H_

#include <jni.h>
#include <time.h>
#include "utilbase.h"

// Utility functions
/**
 * get current time millis
 */
long getTimeMilliseconds(void);
/**
 * get elapsed time as microseconds from previous call
 * @param env
 */
jfloat getDeltaTimeMicroseconds(clock_t &prev_time);

/**
 * return whether or not the specified field is null
 * @param env
 * @param java_obj
 * @param field_name
 * @param field_type
 * @return return true if the field does not exist or the field value is null or the filed type is wrong
 */
bool isNullField(JNIEnv *env, jobject java_obj, const char *field_name, const char *field_type);

/**
 * return a field value as boolean
 * @param env
 * @param java_obj
 * @return	return the value, returan false(0) if the field does not exist
 */
bool getField_bool(JNIEnv *env, jobject java_obj, const char *field_name);
bool setField_bool(JNIEnv *env, jobject java_obj, const char *field_name, bool val);

/**
 * return the static int field value
 * @param env
 * @param java_obj
 * @return return the value, return 0 if the field does not exist
 */
jint getStaticField_int(JNIEnv *env, jobject java_obj, const char *field_name);

/**
 * set specified value into the static int field
 * @param env
 * @param java_obj
 * @param field_name
 * @params val
 */
jint setStaticField_int(JNIEnv *env, jobject java_obj, const char *field_name, jint val);

/**
 * get int field that has specified name from specified Java object
 * @param env
 * @param java_obj
 * @return return the value, return 0 if the field does not exist
 */
jint getField_int(JNIEnv *env, jobject java_obj, const char *field_name);
jint __getField_int(JNIEnv *env, jobject java_obj, jclass clazz, const char *field_name);

/**
 * set the value to int field of specified Java object
 * @param env
 * @param java_obj
 * @param field_name
 * @params val
 */
jint setField_int(JNIEnv *env, jobject java_obj, const char *field_name, jint val);
jint __setField_int(JNIEnv *env, jobject java_obj, jclass clazz, const char *field_name, jint val);

/**
 * get long field that has specified name from specified Java object
 * @param env
 * @param java_obj
 * @return return the value, return 0 if the field does not exist
 */
jlong getField_long(JNIEnv* env, jobject java_obj, const char* field_name);
jlong __getField_long(JNIEnv* env, jobject java_obj, jclass clazz, const char* field_name);

/**
 * set the value into the long field
 * @param env
 * @param java_obj
 * @param field_name
 * @params val
 */
jlong setField_long(JNIEnv* env, jobject java_obj, const char* field_name, jlong val);
jlong __setField_long(JNIEnv* env, jobject java_obj, jclass clazz, const char* field_name, jlong val);

/**
 * get static float field that has specified name from specified Java object
 * @param env
 * @param java_obj
 * @param field_name
 * @return return the value, return 0 if the field does not exist
 */
jfloat getStaticField_float(JNIEnv *env, jobject java_obj, const char *field_name);

/**
 * 指定したJavaオブジェクトの指定した名前のstatic float型のフィールド値を取得
 * @param env
 * @param java_obj
 * @param field_name
 */
jfloat setStaticField_float(JNIEnv *env, jobject java_obj, const char *field_name, jfloat val);

/**
 * get the value of float field that has specified name from specified Java object
 * @param env
 * @param java_obj
 * @param field_name
 * @return return the value, return 0 if the field does not exist
 */
jfloat getField_float(JNIEnv *env, jobject java_obj, const char *field_name);
jfloat __getField_float(JNIEnv *env, jobject java_obj, jclass clazz, const char *field_name);

/**
 * set float value into the specific Java object & field
 * @param env
 * @param java_obj
 * @param field_name
 * @params val
 */
jfloat setField_float(JNIEnv* env, jobject java_obj, const char* field_name, jfloat val);
jfloat __setField_float(JNIEnv *env, jobject java_obj, jclass clazz, const char *field_name, jfloat val);

/**
 * return specific Java object and its field value as a naitive pointer
 * @return pointer value
 */
ID_TYPE getField_NativeObj(JNIEnv *env, jobject java_obj, const char *field_name, const char *field_type);

/**
 * return jobject filed that is specified type from specified field.
 * you should check the field exist and is not null with #isNullField
 * before you call this function.
 * @param env
 * @param java_obj
 * @param field_name
 * @param field_type
 * @return jobject
 */
jobject getField_obj(JNIEnv *env, jobject java_obj, const char *field_name, const char *obj_type);

/**
 * return id from specified field name and type
 * @param env
 * @param java_obj
 * @param field_name
 * @param field_type
 * @return ID_TYPE
 */
ID_TYPE getField_obj_id(JNIEnv *env, jobject java_obj, const char *field_name, const char *obj_type);

/**
 * @param env: this param should not be null
 * @param java_obj: this param should not be null
 * @return
 */
inline void *getField_nativePtr(JNIEnv *env, jobject java_obj, const char *field_name) {
//	LOGV("get_nativeRec:");
	return reinterpret_cast<void *>(getField_long(env, java_obj, field_name));
}

/**
 * @param env: this param should not be null
 * @param java_obj: this param should not be null
 */
inline void setField_nativePtr(JNIEnv *env, jobject java_obj, const char *field_name, void *ptr) {
//	LOGV("get_nativeRec:");
	setField_long(env, java_obj, field_name, reinterpret_cast<ID_TYPE>(ptr));
}

jobject getStaticField_obj(JNIEnv *env, jobject java_obj, const char *field_name, const char *field_type);
jobject getStaticField_obj(JNIEnv *env, jobject java_obj, const char *field_name, const char *field_type);

int prepareBytebufferId(JNIEnv *env, jclass &byteBufClass, jmethodID &byteBufArrayID);
jlong getByteBuffer(JNIEnv *env, jobject byte_buffer_obj, void *dst_buf, jint _offset, jint _size, jclass byteBufClass, jmethodID byteBufArrayID);

jint registerNativeMethods(JNIEnv *env, const char *class_name, JNINativeMethod *methods, int num_methods);
void setVM(JavaVM *);
JavaVM *getVM();
JNIEnv *getEnv();

#endif /* UTILS_H_ */
