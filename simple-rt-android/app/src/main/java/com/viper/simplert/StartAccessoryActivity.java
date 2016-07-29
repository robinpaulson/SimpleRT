package com.viper.simplert;

import android.app.Activity;
import android.content.Intent;
import android.net.VpnService;
import android.os.Bundle;
import android.widget.Toast;

public class StartAccessoryActivity extends Activity {
    private static final String TAG = "StartAccessoryActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (getIntent() != null) {
            Toast.makeText(this, "SimpleRT Connected!", Toast.LENGTH_SHORT).show();

            Intent intent = VpnService.prepare(this);
            if (intent != null) {
                startActivityForResult(intent, 0);
            } else {
                onActivityResult(0, RESULT_OK, null);
            }
        }
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request == 0 && result == RESULT_OK) {
            Intent intent = new Intent(this, TetherService.class);
            intent.fillIn(getIntent(), 0);
            startService(intent);
        }
        finish();
    }
}
