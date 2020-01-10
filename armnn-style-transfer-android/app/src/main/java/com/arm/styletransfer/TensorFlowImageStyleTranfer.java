/*
 *  Copyright © 2019 Arm Ltd. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
package com.arm.styletransfer;

import android.content.Context;
import android.graphics.Bitmap;
import android.os.SystemClock;
import android.util.Log;

import org.tensorflow.lite.Interpreter;
import org.tensorflow.lite.nnapi.NnApiDelegate;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * A classifier specialized to label images using TensorFlow.
 */
public class TensorFlowImageStyleTranfer {

    private static final String TAG = "TFImageStyleTranfer";

    public static final String MODEL_FILE = "MODEL_FILE";
    public static final String ENABLE_NNAPI = "ENABLE_NNAPI";

    public static final String MODEL_FILE_DES_GLANEUSES = "des_glaneuses.tflite";
    public static final String MODEL_FILE_LA_MUSE = "la_muse.tflite";
    public static final String MODEL_FILE_MIRROR = "mirror.tflite";
    public static final String MODEL_FILE_UDNIE = "udnie.tflite";
    public static final String MODEL_FILE_WAVE_CROP = "wave_crop.tflite";

    /** Dimensions of inputs. */
    private static final int DIM_BATCH_SIZE = 1;
    private static final int DIM_PIXEL_SIZE = 3;


    /** Cache to hold image data. */
    private ByteBuffer imgData = null;

    /** Inference results (Tensorflow Lite output). */
    private float[][][][] outputVector = null;
    private Bitmap outputImage = null;
    private long timecost = 0;

    /** Pre-allocated buffer for intermediate bitmap pixels */
    private int[] intValues;

    /** TensorFlow Lite engine */
    private Interpreter tfLite;
    private Interpreter.Options tfLiteOptions = new Interpreter.Options();
    private NnApiDelegate delegate;

    private int mInputImageWidth;
    private int mInputImageHeight;

    private String mModelFile;
  

    /**
     * Initializes a TensorFlow Lite session for classifying images.
     */
    public TensorFlowImageStyleTranfer(Context context, int inputImageWidth, int inputImageHeight, String modelFile, boolean enableNNAPI)
            throws IOException {

        mModelFile = modelFile;
        if (enableNNAPI && TensorFlowHelper.canNNAPIEnabled()){
            delegate = new NnApiDelegate();
            this.tfLiteOptions.addDelegate(delegate);
            this.tfLite = new Interpreter(TensorFlowHelper.loadModelFile(context, mModelFile), tfLiteOptions);
        } else {
            this.tfLite = new Interpreter(TensorFlowHelper.loadModelFile(context, mModelFile));
        }

        imgData =
                ByteBuffer.allocateDirect(
                        4 * DIM_BATCH_SIZE * inputImageWidth * inputImageHeight * DIM_PIXEL_SIZE);
        imgData.order(ByteOrder.nativeOrder());
        outputVector = new float[DIM_BATCH_SIZE][inputImageWidth][inputImageHeight][DIM_PIXEL_SIZE ];

        // Pre-allocate buffer for image pixels.
        intValues = new int[inputImageWidth * inputImageHeight];
        mInputImageWidth = inputImageWidth;
        mInputImageHeight = inputImageHeight;
    }

    /**
     * Clean up the resources used by the classifier.
     */
    public void destroyStyler() {
        tfLite.close();
        tfLite = null;
    }


    /**
     * @param image Bitmap containing the image to be classified. The image can be
     *              of any size, but preprocessing might occur to resize it to the
     *              format expected by the classification process, which can be time
     *              and power consuming.
     */
    public Bitmap doStyleTransfer(Bitmap image) {
        TensorFlowHelper.convertBitmapToByteBuffer(image, intValues, imgData);

        long startTime = SystemClock.uptimeMillis();
        // Here's where the magic happens!!!
        try {
            tfLite.run(imgData, outputVector);
        } catch (Exception e) {
            Log.e(TAG, "Exception = " + e);
        }
        long endTime = SystemClock.uptimeMillis();
        timecost = endTime - startTime;
        Log.d(TAG, "Timecost to run model inference: " + timecost);
        Log.d(TAG, "outputVector = " + outputVector);

        outputImage = Bitmap.createBitmap(mInputImageWidth, mInputImageHeight, Bitmap.Config.ARGB_8888);

        for(int he=0; he<mInputImageHeight; he++) {
            for(int wd=0; wd<mInputImageWidth; wd++){
                float r_pixelValue = outputVector[0][wd][he][0];
                float g_pixelValue = outputVector[0][wd][he][1];
                float b_pixelValue = outputVector[0][wd][he][2];

                int r_pixelInt = (int)r_pixelValue;
                int g_pixelInt = (int)g_pixelValue;
                int b_pixelInt = (int)b_pixelValue;

                int color = (0xff) << 24 | (r_pixelInt & 0xff) << 16 | (g_pixelInt & 0xff) << 8 | (b_pixelInt & 0xff);
                outputImage.setPixel(wd,he, color);
            }
        }

        // Get the results with the highest confidence and map them to their labels
        return outputImage;
    }

    public long getLastTimecost()
    {
        return timecost;
    }

}
