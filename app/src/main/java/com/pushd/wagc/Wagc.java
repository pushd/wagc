package com.pushd.wagc;

public class Wagc {
    private static long mNativeHandle;
    private static Wagc mInstance;

    static {
        System.loadLibrary("wagc");
    }

    public static Wagc getInstance() {
        if (mInstance == null) {
            mInstance = new Wagc();
        }
        return mInstance;
    }

    public native int process(long handle, short[] samples, int micLevel, short[] outArray);


    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        destroy(mNativeHandle);
        mNativeHandle = 0;
    }

    private native long init();
    private native void destroy(long handle);
    private Wagc() {
        mNativeHandle = init();
    }
}
