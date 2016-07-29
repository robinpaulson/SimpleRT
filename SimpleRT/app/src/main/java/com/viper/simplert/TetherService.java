package com.viper.simplert;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.net.VpnService;
import android.os.ParcelFileDescriptor;
import android.util.Log;

public class TetherService extends VpnService {
    private static final String TAG = "TetherService";
    private static final String ACTION_USB_PERMISSION = "com.viper.simplert.TetherService.action.USB_PERMISSION";

    private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            if (UsbManager.ACTION_USB_ACCESSORY_DETACHED.equals(action)) {
                Log.d(TAG,"Accessory detached");

                UsbAccessory accessory = intent.getParcelableExtra(UsbManager.EXTRA_ACCESSORY);
                Native.stop();
                unregisterReceiver(mUsbReceiver);
            }
        }
    };

    @Override
    public int onStartCommand(final Intent intent, int flags, final int startId) {
        Log.w(TAG, "onStartCommand");

        if (intent == null) {
            Log.i(TAG, "Intent is null");
            return START_NOT_STICKY;
        }

        if (Native.is_running()) {
            Log.e(TAG, "already running!");
            return START_NOT_STICKY;
        }

        final UsbAccessory accessory = intent.getParcelableExtra(UsbManager.EXTRA_ACCESSORY);

        if (accessory == null) {
            Log.e(TAG, "accessory is null");
            return START_NOT_STICKY;
        }

        Log.d(TAG, "Got accessory: " + accessory.getModel());

        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        filter.addAction(UsbManager.ACTION_USB_ACCESSORY_DETACHED);
        registerReceiver(mUsbReceiver, filter);

        Builder builder = new Builder();
        builder.setMtu(1500);
        builder.setSession("SimpleRT");
        builder.addAddress("10.10.10.2", 30);
        builder.addRoute("0.0.0.0", 0);
        builder.addDnsServer("8.8.8.8");

        final ParcelFileDescriptor accessoryFd = ((UsbManager) getSystemService(Context.USB_SERVICE)).openAccessory(accessory);
        final ParcelFileDescriptor tunFd = builder.establish();

        Native.start(tunFd.detachFd(), accessoryFd.detachFd());

        return START_NOT_STICKY;
    }
}
