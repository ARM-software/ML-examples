/*
 *  Copyright © 2019 Arm Ltd. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
package com.arm.styletransfer;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageButton;

import com.arm.styletransferapp.R;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    private ImageButton mMainDesGlaneusesButton;
    private ImageButton mMainLaMuseButton;
    private ImageButton mMainMirrorButton;
    private ImageButton mMainUdnieButton;
    private ImageButton mMainWaveCropButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mMainDesGlaneusesButton = findViewById(R.id.main_des_glaneuses_button);
        mMainDesGlaneusesButton.setOnClickListener(this);
        mMainLaMuseButton = findViewById(R.id.main_la_muse_button);
        mMainLaMuseButton.setOnClickListener(this);
        mMainMirrorButton = findViewById(R.id.main_mirror_button);
        mMainMirrorButton.setOnClickListener(this);
        mMainUdnieButton = findViewById(R.id.main_udnie_button);
        mMainUdnieButton.setOnClickListener(this);
        mMainWaveCropButton = findViewById(R.id.main_wave_crop_button);
        mMainWaveCropButton.setOnClickListener(this);
    }

    @Override
    public void onClick(View view) {
        Intent intent = new Intent(this, ImageClassifierActivity.class);
        switch (view.getId()) {
            case R.id.main_des_glaneuses_button:
                intent.putExtra(TensorFlowImageStyleTranfer.MODEL_FILE,TensorFlowImageStyleTranfer.MODEL_FILE_DES_GLANEUSES);
                startActivity(intent);
                break;
            case R.id.main_la_muse_button:
                intent.putExtra(TensorFlowImageStyleTranfer.MODEL_FILE,TensorFlowImageStyleTranfer.MODEL_FILE_LA_MUSE);
                startActivity(intent);
                break;

            case R.id.main_mirror_button:
                intent.putExtra(TensorFlowImageStyleTranfer.MODEL_FILE,TensorFlowImageStyleTranfer.MODEL_FILE_MIRROR);
                startActivity(intent);
                break;

            case R.id.main_udnie_button:
                intent.putExtra(TensorFlowImageStyleTranfer.MODEL_FILE,TensorFlowImageStyleTranfer.MODEL_FILE_UDNIE);
                startActivity(intent);
                break;

            case R.id.main_wave_crop_button:
                intent.putExtra(TensorFlowImageStyleTranfer.MODEL_FILE,TensorFlowImageStyleTranfer.MODEL_FILE_WAVE_CROP);
                startActivity(intent);
                break;
        }

    }
}
