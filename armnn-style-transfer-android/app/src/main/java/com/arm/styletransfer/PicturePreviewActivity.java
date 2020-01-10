/*
 *  Copyright © 2019 Arm Ltd. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.arm.styletransfer;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;


import com.arm.styletransferapp.R;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

public class PicturePreviewActivity extends Activity implements View.OnClickListener {
    private static final String TAG = "PicturePreviewActivity";

    public static final String TIME_COST = "TIME_COST";
    public static final String GENERATED_IMAGE = "GENERATED_IMAGE";

    private TextView mText;
    private ImageView mImage;
    private Button mSavePicButton;
    private static Bitmap mBitmap;

    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.activity_picturepreview);
        mText = findViewById(R.id.resultText);
        mImage = findViewById(R.id.imageView);
        mSavePicButton = findViewById(R.id.save_picButton);

        Intent intent = getIntent();
        mText.setText(intent.getStringExtra(TIME_COST));
        mBitmap = (Bitmap) intent.getParcelableExtra(GENERATED_IMAGE);
        mImage.setImageBitmap(mBitmap);

        mSavePicButton.setOnClickListener(this);
    }


    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.save_picButton:
                saveStyledPicture(mBitmap);
                finish();
                break;
        }
    }

    private void galleryAddPic(File pic) {
        Intent mediaScanIntent = new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
        Uri contentUri = Uri.fromFile(pic);
        mediaScanIntent.setData(contentUri);
        this.sendBroadcast(mediaScanIntent);
    }

    private void saveStyledPicture(Bitmap bitmap) {
        String dir = Environment.getExternalStorageDirectory() + "/" + Environment.DIRECTORY_DCIM + "/Camera/";
        Long tsLong = System.currentTimeMillis() / 1000;
        String filename = "armnn_".concat(tsLong.toString()).concat(".jpg");
        Log.e("app", filename);
        File imageFile = new File(dir, filename);

        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream(imageFile);
            bitmap.compress(Bitmap.CompressFormat.JPEG, 100, fos);
            fos.close();
            galleryAddPic(imageFile);
            Toast.makeText(PicturePreviewActivity.this, R.string.picture_saved, Toast.LENGTH_SHORT).show();
        } catch (IOException e) {
            Log.e("app", e.getMessage());
            if (fos != null) {
                try {
                    fos.close();
                } catch (IOException e1) {
                    e1.printStackTrace();
                }
            }
        }
    }


}



