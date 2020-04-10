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

    public int process(short[] samples, short[] outArray) {
        return process(mNativeHandle, samples, outArray);
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        destroy(mNativeHandle);
        mNativeHandle = 0;
    }

    private native long init();
    private native void destroy(long handle);
    private native int process(long handle, short[] samples, short[] outArray);

    private Wagc() {
        mNativeHandle = init();
    }
}
