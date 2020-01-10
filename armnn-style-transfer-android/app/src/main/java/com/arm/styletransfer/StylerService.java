/*
 *  Copyright © 2019 Arm Ltd. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.arm.styletransfer;

import android.app.Service;
import android.content.Intent;
import android.graphics.Bitmap;
import android.media.Image;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;
import android.util.Size;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicBoolean;

public class StylerService extends Service {

    private static final String TAG = "StylerService";
    private String mModelFile = null;
    private String mBitMap = null;

    // Binder given to clients
    private final IBinder mBinder = new LocalBinder();

    // Matches the images used to train the TensorFlow model
    private static final Size MODEL_IMAGE_SIZE = new Size(200, 200);

    private ImagePreprocessor mImagePreprocessor;
    private TensorFlowImageStyleTranfer mTensorFlowImageStyleTranfer;

    private AtomicBoolean mReady = new AtomicBoolean(false);

    private Bitmap mStyledImage = null;
    private boolean mEnableNNAPI;


    /**
     * Class used for the client Binder.  Because we know this service always
     * runs in the same process as its clients, we don't need to deal with IPC.
     */
    public class LocalBinder extends Binder {
        StylerService getService() {
            // Return this instance of LocalService so clients can call public methods
            return StylerService.this;
        }
    }

    public interface StylerListener {
        void onImageProcecessed();
    }

    private StylerListener mStylerListener;

    public StylerService() {
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }


    @Override
    public void onCreate() {
        Log.d(TAG, "onCreate");
    }


    public int onStartCommand(Intent intent, int flags, int startId) {
        if(intent == null){
            stopSelf();
        } else {
            mModelFile = intent.getStringExtra(TensorFlowImageStyleTranfer.MODEL_FILE);
            mEnableNNAPI = intent.getBooleanExtra(TensorFlowImageStyleTranfer.ENABLE_NNAPI, false);
            Log.d(TAG, "mModelFile = " + mModelFile);
            init();

            mImagePreprocessor =
                    new ImagePreprocessor(320, 240,
                            MODEL_IMAGE_SIZE.getWidth(), MODEL_IMAGE_SIZE.getHeight());

            try {
                mTensorFlowImageStyleTranfer = new TensorFlowImageStyleTranfer(StylerService.this,
                        MODEL_IMAGE_SIZE.getWidth(), MODEL_IMAGE_SIZE.getHeight(), mModelFile, mEnableNNAPI);
            } catch (IOException e) {
                throw new IllegalStateException("Cannot initialize TFLite Classifier", e);
            }
            setReady(true);
        }
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy");

        try {
            if (mTensorFlowImageStyleTranfer != null){
                mTensorFlowImageStyleTranfer.destroyStyler();
            }
        } catch (Throwable t) {
            // close quietly
        }

        mTensorFlowImageStyleTranfer = null;
    }

    private void init() {
        Log.d(TAG,"init is being called");
        setReady(true);
     }

    /**
     * Mark the system as ready for a new image capture
     */
    private void setReady(boolean ready) {
        mReady.set(ready);
    }

    public Bitmap getStyledImage() {
        return mStyledImage;
    }

    public long getTimecost() {
        return mTensorFlowImageStyleTranfer.getLastTimecost();
    }

    /**
     * Verify and initiate a new image capture
     */
    public void startStyleTransfer(Image image) {
        Log.d(TAG, "startImageCapture");

        mStyledImage = mTensorFlowImageStyleTranfer.doStyleTransfer(mImagePreprocessor.preprocessImage(image));
        if (mStylerListener != null){
            Log.d(TAG,"Calling mStylerListener");
            mStylerListener.onImageProcecessed();
        } else {
            Log.d(TAG,"mStylerListener is null");
        }

        setReady(true);
    }


    public void setStylerListener(StylerListener stylerListener) {
        mStylerListener = stylerListener;
    }

}
