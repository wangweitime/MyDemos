package com.wei.collections;

import android.app.Activity;
import android.os.Bundle;

/**
 * Created by wangwei on 9/19/14.
 */
public class EGLImageActivity extends Activity {
    EGLImageView mView;

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        mView = new EGLImageView(getApplication());
        setContentView(mView);
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

