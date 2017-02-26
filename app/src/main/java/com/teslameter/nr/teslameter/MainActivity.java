package com.teslameter.nr.teslameter;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.SystemClock;
import android.widget.TextView;

public class MainActivity extends Activity {
    private TextView tvXraw;
    private TextView tvYraw;
    private TextView tvZraw;
    private TextView tvXvoltage;
    private TextView tvYvoltage;
    private TextView tvZvoltage;
    private Runnable refrestTask;
    private Runnable consumerTask;
    private Thread consumerThread;
    private boolean shouldExit;
    private AlertDialog alertDialog;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        tvXraw = (TextView) findViewById(R.id.x_raw);
        tvYraw = (TextView) findViewById(R.id.y_raw);
        tvZraw = (TextView) findViewById(R.id.z_raw);
        tvXvoltage = (TextView) findViewById(R.id.x_voltage);
        tvYvoltage = (TextView) findViewById(R.id.y_voltage);
        tvZvoltage = (TextView) findViewById(R.id.z_voltage);

        refrestTask = new Runnable() {
            @Override
            public void run() {
                dataAcquire();
                tvXraw.setText(dataGetXraw());
                tvYraw.setText(dataGetYraw());
                tvZraw.setText(dataGetZraw());
                tvXvoltage.setText(dataGetXvoltage());
                tvYvoltage.setText(dataGetYvoltage());
                tvZvoltage.setText(dataGetZvoltage());
                dataRelease();
            }
        };
        consumerTask = new Runnable() {
            @Override
            public void run() {
            int error;

            error = samplingOpen();

            if (error != 0) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        alertDialog.setTitle("Error");
                        alertDialog.setMessage("Failed to open sampling");
                        alertDialog.setButton(AlertDialog.BUTTON_NEUTRAL, "Exit",
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    dialog.dismiss();
                                    android.os.Process.killProcess(android.os.Process.myPid());
                                }
                            });
                        alertDialog.show();
                    }
                });
                shouldExit = true;
                return;
            }
            while(!shouldExit) {
                SystemClock.sleep(100);
                error = samplingRefresh();

                if (error == 0) {
                    runOnUiThread(refrestTask);
                } else {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            alertDialog.setTitle("Error");
                            alertDialog.setMessage("Failed to refresh samples");
                            alertDialog.setButton(AlertDialog.BUTTON_NEUTRAL, "Exit",
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                        dialog.dismiss();
                                        android.os.Process.killProcess(android.os.Process.myPid());
                                    }
                                });
                            alertDialog.show();
                        }
                    });
                    shouldExit = true;
                    return;
                }
            }
            error = samplingClose();

            if (error != 0) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        alertDialog.setTitle("Error");
                        alertDialog.setMessage("Failed to close sampling");
                        alertDialog.setButton(AlertDialog.BUTTON_NEUTRAL, "Exit",
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    dialog.dismiss();
                                    android.os.Process.killProcess(android.os.Process.myPid());
                                }
                            });
                        alertDialog.show();
                    }
                });
                shouldExit = true;
                return;
            }
            }
        };
        shouldExit = false;

        alertDialog = new AlertDialog.Builder(MainActivity.this).create();

        consumerThread = new Thread(consumerTask);
        consumerThread.setPriority(Thread.MAX_PRIORITY);
        consumerThread.start();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        shouldExit = true;
        try {
            consumerThread.join(300);
        } catch (InterruptedException e) {

        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String dataGetXraw();
    public native String dataGetYraw();
    public native String dataGetZraw();
    public native String dataGetXvoltage();
    public native String dataGetYvoltage();
    public native String dataGetZvoltage();
    public native void dataAcquire();
    public native void dataRelease();
    public native int samplingOpen();
    public native int samplingRefresh();
    public native int samplingClose();
}
