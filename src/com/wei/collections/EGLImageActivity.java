package com.wei.collections;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;

import java.io.IOException;
import java.io.InputStream;

/**
 * Created by wangwei on 9/19/14.
 */
public class EGLImageActivity extends Activity {
    EGLImageView mView;

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        // Hide the window title.
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        mView = new EGLImageView(getApplication());
        setContentView(mView);

        InputStream is = this.getResources()
                .openRawResource(R.raw.test);
        Bitmap bitmap;
        try {
            bitmap = BitmapFactory.decodeStream(is);
            mView.setBitmap(bitmap);
        } finally {
            try {
                is.close();
            } catch(IOException e) {
                // Ignore.
            }
        }


    }

    @Override protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
        mView.onResume();
    }

}

