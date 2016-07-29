package com.viper.simplert;

public class Native {
    static native void start(int tun_fd, int acc_fd);
    static native void stop();
    static native boolean is_running();

    static {
        System.loadLibrary("simplertjni");
    }
}
