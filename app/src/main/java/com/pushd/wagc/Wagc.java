package com.pushd.wagc;

public class Wagc {
    static long mNativeHandle;
    static Wagc mInstance;

    native long init();
    native void destroy(long handle);
    native int process(long handle, short[] samples, int micLevel, short[] outArray);

    static {
        System.loadLibrary("wagc");
    }

    static Wagc getInstance() {
        if (mInstance == null) {
            mInstance = new Wagc();
        }
        return mInstance;
    }

    private Wagc() {
        mNativeHandle = init();
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        destroy(mNativeHandle);
        mNativeHandle = 0;
    }
}
