package com.serenegiant.opencv;
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

import android.annotation.SuppressLint;
import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.Surface;

import com.serenegiant.glutils.EglTask;
import com.serenegiant.glutils.GLDrawer2D;
import com.serenegiant.glutils.GLHelper;
import com.serenegiant.glutils.ShaderConst;
import com.serenegiant.mediaeffect.MediaSource;
import com.serenegiant.utils.BuildCheck;
import com.serenegiant.utils.FpsCounter;

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;

public class ImageProcessor {
//	private static final boolean DEBUG = false; // FIXME set false on production
	private static final String TAG = ImageProcessor.class.getSimpleName();

	private static final int REQUEST_DRAW = 1;
	private static final int REQUEST_UPDATE_SIZE = 2;

	/**
	 * callback listener to notify image processing result
	 */
	public interface ImageProcessorCallback {
		/**
		 * when receive result image as ByteBuffer
		 * @param frame
		 */
		public void onFrame(final ByteBuffer frame);

		/**
		 * when receive something result as float array
		 * @param type
		 * @param result
		 */
		public void onResult(final int type, final float[] result);
	}

	/** for access control */
	private final Object mSync = new Object();
	private final ImageProcessorCallback mCallback;
	private volatile boolean isProcessingRunning;
	private ProcessingTask mProcessingTask;
	private Handler mAsyncHandler;

	private final int mSrcWidth, mSrcHeight;
	private volatile boolean requestUpdateExtractionColor;
	/** for calculation of frame rate */
	private final FpsCounter mResultFps = new FpsCounter();

	/** reference of native object, never change name, remove */
	private long mNativePtr;

	/**
	 * Constructor
	 * @param src_width width of source image
	 * @param src_height height of source image
	 * @param callback
	 * @throws NullPointerException
	 */
	public ImageProcessor(final int src_width, final int src_height,
		final ImageProcessorCallback callback) throws NullPointerException {

		if (callback == null) {
			throw new NullPointerException("callback should not be null");
		}
		mSrcWidth = src_width;
		mSrcHeight = src_height;
		mCallback = callback;
		mNativePtr = nativeCreate(new WeakReference<ImageProcessor>(this));
		final HandlerThread thread = new HandlerThread("OnFrameAvailable");
		thread.start();
		mAsyncHandler = new Handler(thread.getLooper());
	}

	/**
	 * start worker thread of ImageProcessor
	 * this will block current thread until worker thread run
	 * @param width
	 * @param height
	 */
	public void start(final int width, final int height) {
		if (mProcessingTask == null) {
			mProcessingTask = new ProcessingTask(this, mSrcWidth, mSrcHeight, width, height);
			new Thread(mProcessingTask, "VideoStream$rendererTask").start();
			mProcessingTask.waitReady();
			synchronized (mSync) {
				for ( ; !isProcessingRunning ; ) {
					try {
						mSync.wait();
					} catch (final InterruptedException e) {
						break;
					}
				}
			}
		}
	}

	/**
	 * stop worker thread for ImageProcessor
	 */
	public void stop() {
		final ProcessingTask task = mProcessingTask;
		mProcessingTask = null;
		if (task != null) {
			synchronized (mSync) {
				isProcessingRunning = false;
				mSync.notifyAll();
			}
			task.release();
		}
	}

	/**
	 * release all related resources
	 */
	public void release() {
		stop();
		if (mAsyncHandler != null) {
			try {
				mAsyncHandler.getLooper().quit();
			} catch (final Exception e) {
			}
			mAsyncHandler = null;
		}
		nativeRelease(mNativePtr);
	}

	/** get Surface to receive source images */
	public Surface getSurface() {
		return mProcessingTask.getSurface();
	}

	/** get SurfaceTexture to receive source images */
	public SurfaceTexture getSurfaceTexture() {
		return mProcessingTask.getSurfaceTexture();
	}

	/** update frame rate of image processing */
	public void updateFps() {
		mResultFps.update();
	}

	/**
	 * get current frame rate of image processing
	 * @return
	 */
	public float getFps() {
		return mResultFps.getFps();
	}

	/**
	 * get total frame rate from start
	 * @return
	 */
	public float getTotalFps() {
		return mResultFps.getTotalFps();
	}
//================================================================================
	// type of result image(currently these values are just ignored on native side)
	// should match values on native side.
	/** result is src image */
	public static final int RESULT_FRAME_TYPE_SRC = 0;
	/** result is after image processing image */
	public static final int RESULT_FRAME_TYPE_DST = 1;
	/** result is src image with drawing something */
	public static final int RESULT_FRAME_TYPE_SRC_LINE = 2;
	/** result is after image processing image with drawing something */
	public static final int RESULT_FRAME_TYPE_DST_LINE = 3;

	/**
	 * request to change result image type
	 * value is just ignored on native side now
	 * @param result_frame_type
	 */
	public void setResultFrameType(final int result_frame_type) {
		final int result = nativeSetResultFrameType(mNativePtr, result_frame_type);
		if (result != 0) {
			throw new IllegalStateException("nativeSetResultFrameType:result=" + result);
		}
	}

	/**
	 * get result image type
	 * @return
	 * @throws IllegalStateException
	 */
	public int getResultFrameType() throws IllegalStateException {
		final int result = nativeGetResultFrameType(mNativePtr);
		if (result < 0) {
			throw new IllegalStateException("nativeGetResultFrameType:result=" + result);
		}
		return result;
	}

//================================================================================
	/**
	 * callback method from native side
	 * never change/remove method name unless you know actually what you do.
	 * @param weakSelf
	 * @param type
	 * @param frame
	 * @param result
	 */
	private static void callFromNative(final WeakReference<ImageProcessor> weakSelf,
		final int type, final ByteBuffer frame, final float[] result) {
		final ImageProcessor self = weakSelf != null ? weakSelf.get() : null;
		if (self != null) {
			try {
				self.handleResult(type, result);
				if (frame != null) {
					self.handleOpenCVFrame(frame);
				}
			} catch (final Exception e) {
				Log.w(TAG, e);
			}
		}
	}

	/**
	 * actual callback method when receive something processing result as float array
	 * @param result
	 */
	private void handleResult(final int type, final float[] result) {
		mResultFps.count();
		try {
			mCallback.onResult(type, result);
		} catch (final Exception e) {
		}
	}

	/**
	 * actual callback method when receive images from native side.
	 * @param frame
	 */
	private void handleOpenCVFrame(final ByteBuffer frame) {
		mCallback.onFrame(frame);
	}

	private class ProcessingTask extends EglTask {
		private int mTexId;
		private SurfaceTexture mSourceTexture;
		private Surface mSourceSurface;
		final float[] mTexMatrix = new float[16];
		/** size of src images */
		private final int WIDTH, HEIGHT;
		/** size of processing images */
		private int mVideoWidth, mVideoHeight;
		private GLDrawer2D mSrcDrawer;
		private MediaSource mMediaSource;

		public ProcessingTask(final ImageProcessor parent,
			final int src_width, final int src_height,
			final int video_width, final int video_height) {
			
			super(null, 0);
			WIDTH = src_width;
			HEIGHT = src_height;
			mVideoWidth = video_width;
			mVideoHeight = video_height;
		}

		public Surface getSurface() {
			return mSourceSurface;
		}

		public SurfaceTexture getSurfaceTexture() {
			return mSourceTexture;
		}

		@SuppressLint("NewApi")
		@Override
		protected void onStart() {
			mSrcDrawer = new GLDrawer2D(true/*isOES*/);	// use GL_TEXTURE_EXTERNAL_OES
			flipMatrix(true);	// swap upside-down
			mTexId = GLHelper.initTex(ShaderConst.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_NEAREST);
			mSourceTexture = new SurfaceTexture(mTexId);
			mSourceTexture.setDefaultBufferSize(WIDTH, HEIGHT);
			mSourceSurface = new Surface(mSourceTexture);
			if (BuildCheck.isLollipop()) {
				mSourceTexture.setOnFrameAvailableListener(
					mOnFrameAvailableListener, mAsyncHandler);
			} else {
				mSourceTexture.setOnFrameAvailableListener(mOnFrameAvailableListener);
			}
			handleResize(mVideoWidth, mVideoHeight);
			// start image processing on native side
			nativeStart(mNativePtr, mVideoWidth, mVideoHeight);
			synchronized (mSync) {
				isProcessingRunning = true;
				mSync.notifyAll();
			}
			mResultFps.reset();
//--------------------------------------------------------------------------------
// there is gl context and you can use OpenGL|ES functions here
//--------------------------------------------------------------------------------
		}

		/**
		 * calc projection matrix
		 * @param verticalFlip
		 */
		private void flipMatrix(final boolean verticalFlip) {
			final float[] mat = new float[32];
			final float[] mvpMatrix = mSrcDrawer.getMvpMatrix();
			System.arraycopy(mvpMatrix, 0, mat, 16, 16);
			Matrix.setIdentityM(mat, 0);
			if (verticalFlip) {
				Matrix.scaleM(mat, 0, 1f, -1f, 1f);
			} else {
				Matrix.scaleM(mat, 0, -1f, 1f, 1f);
			}
			Matrix.multiplyMM(mvpMatrix, 0, mat, 0, mat, 16);
		}

		@Override
		protected void onStop() {
			synchronized (mSync) {
				isProcessingRunning = false;
				mSync.notifyAll();
			}
//--------------------------------------------------------------------------------
// there is gl context and you can use OpenGL|ES functions here
//--------------------------------------------------------------------------------
			makeCurrent();
			// stop image processing on native side
			nativeStop(mNativePtr);
			// release resources
			mSourceSurface = null;
			if (mSourceTexture != null) {
				mSourceTexture.release();
				mSourceTexture = null;
			}
			if (mMediaSource != null) {
				mMediaSource.release();
				mMediaSource = null;
			}
			if (mSrcDrawer != null) {
				mSrcDrawer.release();
				mSrcDrawer = null;
			}
		}

		@Override
		protected Object processRequest(final int request,
			final int arg1, final int arg2, final Object obj) {
			
			switch (request) {
			case REQUEST_DRAW:
				handleDraw();
				break;
			case REQUEST_UPDATE_SIZE:
				handleResize(arg1, arg2);
				break;
			}
			return null;
		}

		@Override
		protected boolean onError(final Exception e) {
			Log.w(TAG, e);
			return true;
		}

		/**
		 * drawing(run on worker thread)
		 */
		private void handleDraw() {
			try {
				makeCurrent();
				mSourceTexture.updateTexImage();
				mSourceTexture.getTransformMatrix(mTexMatrix);
			} catch (final Exception e) {
				Log.e(TAG, "ProcessingTask#draw:thread id =" + Thread.currentThread().getId(), e);
				return;
			}
			mMediaSource.setSource(mSrcDrawer, mTexId, mTexMatrix);
//--------------------------------------------------------------------------------
// there is gl context and you can use OpenGL|ES functions here
// Most functions on OpenCV is relatively heavy on most of Android devices
// and please consider to use OpenGL|ES instead of OpenCV
// You can find some image effect classes in libcommon repository
// under `com.serenegiant.glutils` and `com.serenegiant.mediaeffect` package.
//--------------------------------------------------------------------------------
			// pass image to native side(as cv::mat)
			mMediaSource.getOutputTexture().bind();
			nativeHandleFrame(mNativePtr, mVideoWidth, mVideoHeight, 0);
			mMediaSource.getOutputTexture().unbind();
			// workaround to avoid hung-up
			makeCurrent();
			GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
			GLES20.glFlush();
		}

		/**
		 * change images side(run on worker thread)
		 * @param width
		 * @param height
		 */
		private void handleResize(final int width, final int height) {
			mVideoWidth = width;
			mVideoHeight = height;
			if (mMediaSource != null) {
				mMediaSource.resize(width, height);
			} else {
				mMediaSource = new MediaSource(width, height);
			}
		}

		private final SurfaceTexture.OnFrameAvailableListener
			mOnFrameAvailableListener = new SurfaceTexture.OnFrameAvailableListener() {

			@Override
			public void onFrameAvailable(final SurfaceTexture surfaceTexture) {
				removeRequest(REQUEST_DRAW);
				offer(REQUEST_DRAW);
			}
		};
	}

	private static boolean isInit;
	private static native void nativeClassInit();
	static {
		if (!isInit) {
			System.loadLibrary("gnustl_shared");
			System.loadLibrary("common");
			System.loadLibrary("opencv_java3");
			System.loadLibrary("imageproc");
			nativeClassInit();
			isInit = true;
		}
	}

	private native long nativeCreate(final WeakReference<ImageProcessor> weakSelf);
	private native void nativeRelease(final long id_native);

	private static native int nativeStart(final long id_native,
		final int width, final int height);
	private static native int nativeStop(final long id_native);
	private static native int nativeHandleFrame(final long id_native,
		final int width, final int height, final int tex_name);
	private static native int nativeSetResultFrameType(final long id_native,
		final int showDetects);
	private static native int nativeGetResultFrameType(final long id_native);
}
