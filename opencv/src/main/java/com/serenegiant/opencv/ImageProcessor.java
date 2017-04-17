package com.serenegiant.opencv;
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
//	private static final boolean DEBUG = false; // FIXME 実働時はfalseにすること
	private static final String TAG = ImageProcessor.class.getSimpleName();

	private static final int REQUEST_DRAW = 1;
	private static final int REQUEST_UPDATE_SIZE = 2;

	/**
	 * 処理結果を通知するためのコールバックリスナー
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
	public ImageProcessor(final int src_width, final int src_height, final ImageProcessorCallback callback)
		throws NullPointerException {

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
	// 結果映像の種類定数
	/** 結果映像として元映像を返す */
	public static final int RESULT_FRAME_TYPE_SRC = 0;
	/** 結果映像として処理後映像を返す */
	public static final int RESULT_FRAME_TYPE_DST = 1;
	/** 結果映像として元映像に解析結果を書き込んで返す */
	public static final int RESULT_FRAME_TYPE_SRC_LINE = 2;
	/** 結果映像として処理後映像に解析結果を書き込んで返す */
	public static final int RESULT_FRAME_TYPE_DST_LINE = 3;

	/**
	 * 結果映像の種類を変更
	 * @param result_frame_type
	 */
	public void setResultFrameType(final int result_frame_type) {
		final int result = nativeSetResultFrameType(mNativePtr, result_frame_type);
		if (result != 0) {
			throw new IllegalStateException("nativeSetResultFrameType:result=" + result);
		}
	}

	/**
	 * 結果映像の種類を取得
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
	 * native側からの結果コールバック
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
	 * native側からの結果コールバックの実際の処理
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
	 * OpenCVで処理した映像を受け取った時の処理
	 * @param frame
	 */
	private void handleOpenCVFrame(final ByteBuffer frame) {
		mCallback.onFrame(frame);
	}

	private class ProcessingTask extends EglTask {
		/** 映像をテクスチャとして受け取る時のテクスチャ名(SurfaceTexture生成時/分配描画に使用) */
		private int mTexId;
		/** 映像を受け取るtsめのSurfaceTexture */
		private SurfaceTexture mSourceTexture;
		/** 映像を受け取るためのSurfaceTextureから取得したSurface */
		private Surface mSourceSurface;
		/** mSourceTextureのテクスチャ変換行列 */
		final float[] mTexMatrix = new float[16];
		/** ソース映像サイズ */
		private final int WIDTH, HEIGHT;
		/** 映像サイズ */
		private int mVideoWidth, mVideoHeight;
		private GLDrawer2D mSrcDrawer;
		// 映像受け取り用
		private MediaSource mMediaSource;

		public ProcessingTask(final ImageProcessor parent, final int src_width, final int src_height, final int video_width, final int video_height) {
			super(null, 0);
			WIDTH = src_width;
			HEIGHT = src_height;
			mVideoWidth = video_width;
			mVideoHeight = video_height;
		}

		/** 映像受け取り用Surfaceを取得 */
		public Surface getSurface() {
			return mSourceSurface;
		}

		/** 映像受け取り用SurfaceTextureを取得 */
		public SurfaceTexture getSurfaceTexture() {
			return mSourceTexture;
		}

		@SuppressLint("NewApi")
		@Override
		protected void onStart() {
			// ソース映像の描画用
			mSrcDrawer = new GLDrawer2D(true/*isOES*/);	// GL_TEXTURE_EXTERNAL_OESを使う
			flipMatrix(true);	// 上下入れ替え
			mTexId = GLHelper.initTex(ShaderConst.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_NEAREST);
			mSourceTexture = new SurfaceTexture(mTexId);
			mSourceTexture.setDefaultBufferSize(WIDTH, HEIGHT);
			mSourceSurface = new Surface(mSourceTexture);
			if (BuildCheck.isLollipop()) {
				mSourceTexture.setOnFrameAvailableListener(mOnFrameAvailableListener, mAsyncHandler);
			} else {
				mSourceTexture.setOnFrameAvailableListener(mOnFrameAvailableListener);
			}
			handleResize(mVideoWidth, mVideoHeight);
			// native側の処理を開始
			nativeStart(mNativePtr, mVideoWidth, mVideoHeight);
			synchronized (mSync) {
				isProcessingRunning = true;
				mSync.notifyAll();
			}
			mResultFps.reset();
		}

		/**
		 * 諸般の事情で上下をひっくり返すために射影行列を計算
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
			makeCurrent();
			// native側の処理を停止させる
			nativeStop(mNativePtr);
			// 破棄処理
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
		protected Object processRequest(final int request, final int arg1, final int arg2, final Object obj) {
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
		 * 実際の描画処理(ワーカースレッド上で実行)
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
			// SurfaceTextureで受け取った画像をプレフィルター用にセット
			mMediaSource.setSource(mSrcDrawer, mTexId, mTexMatrix);
			// if you want to apply image effect by OpenGL|ES, execute here
			mMediaSource.getOutputTexture().bind();
			// Native側でglReadPixelsを使ってフレームバッファから画像データを取得する
			// Nexus6Pで直接glReadPixelsを使って読み込むと約5ミリ秒かかる
			// PBOのピンポンバッファを使うと約1/10の0.5ミリ秒で返ってくる
			nativeHandleFrame(mNativePtr, mVideoWidth, mVideoHeight, 0);
			mMediaSource.getOutputTexture().unbind();
			// 何も描画しないとハングアップする機種があるので塗りつぶす(と言っても1x1だから負荷は気にしなくて良い)
			makeCurrent();
			GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
			GLES20.glFlush();
		}

		/**
		 * 映像サイズ変更処理(ワーカースレッド上で実行)
		 * @param width
		 * @param height
		 */
		private void handleResize(final int width, final int height) {
			mVideoWidth = width;
			mVideoHeight = height;
			// プレフィルタ用
			if (mMediaSource != null) {
				mMediaSource.resize(width, height);
			} else {
				mMediaSource = new MediaSource(width, height);
			}
		}

		/**
		 * TextureSurfaceで映像を受け取った際のコールバックリスナー
		 */
		private final SurfaceTexture.OnFrameAvailableListener
			mOnFrameAvailableListener = new SurfaceTexture.OnFrameAvailableListener() {

			@Override
			public void onFrameAvailable(final SurfaceTexture surfaceTexture) {
				// 前の映像フレームが残っていたらクリアする
				removeRequest(REQUEST_DRAW);
				// 新しく処理要求する
				offer(REQUEST_DRAW);
			}
		};
	}

	/**
	 * 飽和計算(int)
	 * @param v
	 * @param min
	 * @param max
	 * @return
	 */
	public static final int sat(final int v, final int min, final int max) {
		return v <= min ? min : (v >= max ? max : v);
	}

	/**
	 * 飽和計算(float)
	 * @param v
	 * @param min
	 * @param max
	 * @return
	 */
	public static final float sat(final float v, final float min, final float max) {
		return v <= min ? min : (v >= max ? max : v);
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

	private static native int nativeStart(final long id_native, final int width, final int height);
	private static native int nativeStop(final long id_native);
	private static native int nativeHandleFrame(final long id_native, final int width, final int height, final int tex_name);
	private static native int nativeSetResultFrameType(final long id_native, final int showDetects);
	private static native int nativeGetResultFrameType(final long id_native);
}
