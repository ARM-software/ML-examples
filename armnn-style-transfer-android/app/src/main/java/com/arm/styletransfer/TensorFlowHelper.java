/*
 *  Copyright © 2019 Arm Ltd. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
package com.arm.styletransfer;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.os.Build;
import android.util.Log;


import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.List;
import java.util.PriorityQueue;

/**
 * Helper functions for the TensorFlow image classifier.
 */
public class TensorFlowHelper {
    private static final String TAG = "TensorFlowHelper";
    private static final int RESULTS_TO_SHOW = 3;

    /**
     * Memory-map the model file in Assets.
     */
    public static MappedByteBuffer loadModelFile(Context context, String modelFile)
            throws IOException {

        Log.d(TAG,"modelFile = "+modelFile);
        AssetFileDescriptor fileDescriptor = context.getAssets().openFd(modelFile);
        FileInputStream inputStream = new FileInputStream(fileDescriptor.getFileDescriptor());
        FileChannel fileChannel = inputStream.getChannel();
        long startOffset = fileDescriptor.getStartOffset();
        long declaredLength = fileDescriptor.getDeclaredLength();
        return fileChannel.map(FileChannel.MapMode.READ_ONLY, startOffset, declaredLength);
    }

    public static byte [] float2ByteArray (float value)
    {
        return ByteBuffer.allocate(4).putFloat(value).array();
    }


    public static float toFloat(byte[] bytes) {
        return ByteBuffer.wrap(bytes).getFloat();
    }


    public static boolean canNNAPIEnabled(){
        return android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.P;
    }

    /** Writes Image data into a {@code ByteBuffer}. */
    public static void convertBitmapToByteBuffer(Bitmap bitmap, int[] intValues, ByteBuffer imgData) {
        if (imgData == null) {
            return;
        }
        imgData.rewind();
        bitmap.getPixels(intValues, 0, bitmap.getWidth(), 0, 0,
                bitmap.getWidth(), bitmap.getHeight());
        // Encode the image pixels into a byte buffer representation matching the expected
        // input of the Tensorflow model
        int pixel = 0;
        for (int i = 0; i < bitmap.getWidth(); ++i) {
            for (int j = 0; j < bitmap.getHeight(); ++j) {
                final int val = intValues[pixel++];

                float  r_color = (float)((val >> 16) & 0xFF);
                float  g_color = (float)((val >> 8) & 0xFF);
                float  b_color = (float)(val & 0xFF);

                byte[] r_fbytes = float2ByteArray(r_color);
                byte[] g_fbytes = float2ByteArray(g_color);
                byte[] b_fbytes = float2ByteArray(b_color);

                imgData.put(r_fbytes[3]);
                imgData.put(r_fbytes[2]);
                imgData.put(r_fbytes[1]);
                imgData.put(r_fbytes[0]);

                imgData.put(g_fbytes[3]);
                imgData.put(g_fbytes[2]);
                imgData.put(g_fbytes[1]);
                imgData.put(g_fbytes[0]);

                imgData.put(b_fbytes[3]);
                imgData.put(b_fbytes[2]);
                imgData.put(b_fbytes[1]);
                imgData.put(b_fbytes[0]);

            }
        }
    }
}
