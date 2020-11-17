package com.arm.visualrecognizer;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageDecoder;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;
import android.view.View;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.TextView;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Vector;

public class MainActivity extends AppCompatActivity implements Recognizer.RecognitionListener {

    final static String TAG = "MainActivity";
    final static float  ACCEPTANCE_SCORE = 0.4f;
    final int SELECT_PICTURE = 1;
    Recognizer recognizer;

    ImageView imgSelectedImage;
    TextView txtClassification, txtExecutionTime, txtConfidence;
    CheckBox chkUseNNAPI;
    Vector<String> labels;



    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        recognizer = new Recognizer(this);
        recognizer.setRecognitionListener(this);

        imgSelectedImage = findViewById(R.id.imgSelectedImage);
        txtClassification = findViewById(R.id.txtClassification);
        txtConfidence = findViewById(R.id.txtConfidence);
        txtExecutionTime = findViewById(R.id.txtExecutionTime);
        chkUseNNAPI = findViewById(R.id.chkUseNNAPI);
    }

    public void onLoadImageClicked(View view) {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType("image/*");
        Intent chooser = Intent.createChooser(intent, "Choose a Picture");
        startActivityForResult(chooser, SELECT_PICTURE);
    }

    public void onActivityResult (int reqCode, int resultCode, Intent data) {
        super.onActivityResult(reqCode, resultCode, data);
        if (resultCode == RESULT_OK) {
            if (reqCode == SELECT_PICTURE) {
                Uri selectedUri = data.getData();
                String fileString = selectedUri.getPath();
                imgSelectedImage.setImageURI(selectedUri);

                ImageDecoder.Source source = ImageDecoder.createSource(this.getContentResolver(), selectedUri);
                Bitmap image = null;
                try {
                    image = ImageDecoder.decodeBitmap(source);
                    boolean useNNAPI = chkUseNNAPI.isChecked();
                    recognizer.setUseNNAPI(useNNAPI);
                    recognizer.recognize(image);
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }


    @Override
    public void OnRecognized(Recognizer.Result result) {
        runOnUiThread(()-> {
            Log.d(TAG, result.toString());
            txtExecutionTime.setText( String.format("%.2f", (float)result.getExecutionTime() / 1000.0f));
            txtConfidence.setText(String.format("%d",result.getPropability()));
            if(result.getPropability() < ACCEPTANCE_SCORE) {
                txtClassification.setText(getResources().getString(R.string.label_uncertainresult));
            } else {
                txtClassification.setText(result.getLabel());
            }
        });
    }
}