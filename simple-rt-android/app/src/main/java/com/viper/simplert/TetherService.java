/*
 * SimpleRT: Reverse tethering utility for Android
 * Copyright (C) 2016 Konstantin Menyaev
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.viper.simplert;

import android.annotation.TargetApi;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.net.ConnectivityManager;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.net.VpnService;
import android.os.Build;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.widget.Toast;

import java.util.List;

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
            showErrorDialog(getString(R.string.accessory_error));
            stopSelf();
            return START_NOT_STICKY;
        }

        /* default values for compatibility with old simple_rt version */
        int prefixLength = 30;
        String ipAddr = "10.10.10.2";
        String dnsServer = "8.8.8.8";

        /* expected format: address,dns_server */
        String[] tokens = accessory.getSerial().split(",");
        if (tokens.length == 2) {
            ipAddr = tokens[0];
            dnsServer = tokens[1];
            prefixLength = 24;
        }

        Log.d(TAG, "Got accessory: " + accessory.getModel());

        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        filter.addAction(UsbManager.ACTION_USB_ACCESSORY_DETACHED);
        registerReceiver(mUsbReceiver, filter);

        Builder builder = new Builder();
        builder.setMtu(1500);
        builder.setSession(getString(R.string.app_name));
        builder.addAddress(ipAddr, prefixLength);
        builder.addRoute("0.0.0.0", 0);
        builder.addDnsServer(dnsServer);

        final ParcelFileDescriptor accessoryFd = ((UsbManager) getSystemService(Context.USB_SERVICE)).openAccessory(accessory);
        if (accessoryFd == null) {
            showErrorDialog(getString(R.string.accessory_error));
            stopSelf();
            return START_NOT_STICKY;
        }

        final ParcelFileDescriptor tunFd = builder.establish();
        if (tunFd == null) {
            showErrorDialog(getString(R.string.tun_error));
            stopSelf();
            return START_NOT_STICKY;
        }

        Toast.makeText(this, "SimpleRT Connected!", Toast.LENGTH_SHORT).show();
        Native.start(tunFd.detachFd(), accessoryFd.detachFd());

        setAsUndernlyingNetwork(ipAddr);
        return START_NOT_STICKY;
    }

    private void setAsUndernlyingNetwork(String ipAddress) {
        if (Build.VERSION.SDK_INT >= 22) {
            Network vpnNetwork = findVpnNetwork(ipAddress);
            if (vpnNetwork != null) {
                // so that applications knows that network is available
                setUnderlyingNetworks(new Network[]{vpnNetwork});
                Log.w(TAG, "VPN set as underlying network");
            }
        } else {
            Log.w(TAG, "Cannot set underlying network, API version " + Build.VERSION.SDK_INT + " < 22");
        }
    }

    @TargetApi(22)
    private Network findVpnNetwork(String ipAddress) {
        ConnectivityManager cm = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        Network[] networks = cm.getAllNetworks();
        for (Network network : networks) {
            LinkProperties linkProperties = cm.getLinkProperties(network);
            List<LinkAddress> addresses = linkProperties.getLinkAddresses();
            for (LinkAddress addr : addresses) {
                if (addr.toString().equals(ipAddress + "/24")) {
                    return network;
                }
            }
        }
        return null;
    }

    private void showErrorDialog(String err) {
        Intent activityIntent = new Intent(getApplicationContext(), InfoActivity.class);
        activityIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        activityIntent.putExtra("text", err);
        startActivity(activityIntent);
    }
}
