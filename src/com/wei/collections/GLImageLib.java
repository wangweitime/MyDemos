package com.wei.collections;

import android.graphics.Bitmap;

/**
 * Created by wangwei on 9/19/14.
 */
public class GLImageLib {

    static {
        System.loadLibrary("eglimage");
    }

    /**
     * @param width the current view width
     * @param height the current view height
     */
    public static native void init(int width, int height);
    public static native void draw(Bitmap bmp);
}
