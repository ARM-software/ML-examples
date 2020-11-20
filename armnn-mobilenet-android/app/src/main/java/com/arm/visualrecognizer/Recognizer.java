/*
 *  Copyright © 2020 Arm Ltd. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
package com.arm.visualrecognizer;

import android.content.Context;
import android.graphics.Bitmap;
import android.os.Build;
import android.os.Trace;
import android.util.Log;

import androidx.annotation.NonNull;

import org.tensorflow.lite.DataType;
import org.tensorflow.lite.Interpreter;
import org.tensorflow.lite.gpu.GpuDelegate;
import org.tensorflow.lite.nnapi.NnApiDelegate;
import org.tensorflow.lite.support.common.FileUtil;
import org.tensorflow.lite.support.common.ops.CastOp;
import org.tensorflow.lite.support.common.ops.NormalizeOp;
import org.tensorflow.lite.support.image.ImageProcessor;
import org.tensorflow.lite.support.image.TensorImage;
import org.tensorflow.lite.support.image.ops.ResizeOp;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.nio.MappedByteBuffer;
import java.util.Vector;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class Recognizer {
    static final String TAG = "Recognizer";
    static final String MODEL_FILE_NAME = "mobilenet_v2_1.0_224_quantized_1_default_1.tflite";
    static int IMAGE_HEIGHT = 224;
    static int IMAGE_WIDTH = 224;

    Interpreter tfLiteInterpreter;
    NnApiDelegate nnApiDelegate;
    boolean loadedSuccessfully = false;
    boolean useNNAPI = true;
    Context context;
    final static Vector<String> labelList = new Vector<String>();
    private  ExecutorService pool;
    RecognitionListener listener = null;
    final int POOL_SIZE = 4;

    public class Result {
        private String label;
        private int probability;
        long executionTime;

        public  Result(String label, byte probability, long executionTime) {
            this.label = label;
            this.probability = (int) 0xFF & probability;
            this.executionTime = executionTime;
        }
        public String getLabel() {
            return label;
        }
        public int  getPropability() {
            return probability;
        }

        public long  getExecutionTime() {
            return this.executionTime;
        }

        @NonNull
        @Override
        public String toString() {
            String s = String.format("Recognized %s with %d confidence in %d ns", getLabel(), getPropability() * 100, getExecutionTime());
            return s;
        }
    }

    public interface RecognitionListener {
        void OnRecognized(Result result);
    }


    public Recognizer(Context context) {
        pool = Executors.newFixedThreadPool(POOL_SIZE);
        this.context = context;
        loadLabels();
        //prepareInterpretor();
    }

    public void setRecognitionListener(RecognitionListener listener) {
        this.listener = listener;
    }

    public void setUseNNAPI(boolean useNNAPI) {
        this.useNNAPI = useNNAPI;
    }

    void loadLabels() {
        try {
            if (labelList == null || labelList.size() == 0) {
                InputStream labelStream = context.getAssets().open("labels.txt");
                BufferedReader br = new BufferedReader(new InputStreamReader(labelStream));
                String line;
                while (null != (line = br.readLine())) {
                    Log.v(TAG, String.format("LABEL: %s", line));
                    labelList.add(line);
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

void prepareInterpretor()  {
    Interpreter.Options options  = new Interpreter.Options();
    if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && this.useNNAPI) {
        nnApiDelegate = new NnApiDelegate();
        options.addDelegate(nnApiDelegate);
    }
    try {
        MappedByteBuffer tfLiteModel = FileUtil.loadMappedFile(context, MODEL_FILE_NAME);
        tfLiteInterpreter = new Interpreter(tfLiteModel, options);
        loadedSuccessfully = true;
    }catch (IOException exc) {
        Log.e(TAG, exc.getMessage());
        exc.printStackTrace();
        loadedSuccessfully = false;
    }
}

    void closeInterpretor() {
        if(tfLiteInterpreter!=null)
            tfLiteInterpreter.close();
        if(nnApiDelegate!=null)
            nnApiDelegate.close();
    }

    public void recognize(Bitmap sourceImage) {

        prepareInterpretor();

        sourceImage = sourceImage.copy(Bitmap.Config.ARGB_8888, true);
        final byte outputBuffer[][] = new byte[1][1001];
        ImageProcessor imageProcessor = new ImageProcessor.Builder()
                .add(new ResizeOp(IMAGE_HEIGHT, IMAGE_WIDTH, ResizeOp.ResizeMethod.BILINEAR))
                .build();
        TensorImage tfImage = new TensorImage(DataType.UINT8);
        Trace.beginSection("preparing image");
        tfImage.load(sourceImage);
        tfImage = imageProcessor.process(tfImage);
        Trace.endSection();
        final ByteBuffer inputBuffer = tfImage.getBuffer();
        Log.i(TAG,String.format("Size of input buffer:%d", inputBuffer.array().length));

        Runnable recognitionTask = new Runnable() {
            //byte[][] output = outputBuffer;
            //ByteBuffer input = inputBuffer;
            @Override
            public void run() {
                long startTime = System.nanoTime();
                Trace.beginSection("Recognition");
                tfLiteInterpreter.run(inputBuffer, outputBuffer);
                Trace.endSection();
                long endTime = System.nanoTime();
                closeInterpretor();
                long nsExecutionTime = endTime - startTime;
                int itemID = getStongestPosition(outputBuffer);
                String label = getLabel(itemID);
                byte probability = outputBuffer[0][itemID] ;
                Result r = new Result(label, probability, nsExecutionTime);
                if(Recognizer.this.listener != null) {
                    Recognizer.this.listener.OnRecognized(r);
                }
            }
        };
        pool.execute(recognitionTask);
    }

    public int getStongestPosition(byte[][] output) {
        byte[] results = output[0];
        int maxIndex = 0;
        int  maxValue = 0;
        for(int i=0;i<results.length;++i) {
            int unsignedResult = results[i] & 0xFF;
            if(unsignedResult>maxValue) {
                maxIndex = i;
                maxValue = unsignedResult;
            }
        }
        return maxIndex;
    }

    public String getLabel(int index) {
        if(index < labelList.size())
            return labelList.get(index);
        return String.format("Unknown %d", index);
    }
}
