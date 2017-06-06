package com.teslameter.nr.teslameter;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends Activity {
    private TextView tvXraw;
    private TextView tvYraw;
    private TextView tvZraw;
    private TextView tvAux1raw;
    private TextView tvAux2raw;
    private TextView tvAux1Voltage;
    private TextView tvAux2Voltage;
    private TextView tvXvoltage;
    private TextView tvYvoltage;
    private TextView tvZvoltage;
    private TextView tvStats;
    private TextView tvInfos;
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
        tvAux1raw = (TextView) findViewById(R.id.aux1_raw);
        tvAux2raw = (TextView) findViewById(R.id.aux2_raw);
        tvAux1Voltage = (TextView) findViewById(R.id.aux1_voltage);
        tvAux2Voltage = (TextView) findViewById(R.id.aux2_voltage);
        tvXvoltage = (TextView) findViewById(R.id.x_voltage);
        tvYvoltage = (TextView) findViewById(R.id.y_voltage);
        tvZvoltage = (TextView) findViewById(R.id.z_voltage);
        tvStats = (TextView) findViewById(R.id.stats);
        tvInfos = (TextView) findViewById(R.id.infos);

        refrestTask = new Runnable() {
            @Override
            public void run() {
                dataAcquire();
                tvXraw.setText(dataGetXraw());
                tvYraw.setText(dataGetYraw());
                tvZraw.setText(dataGetZraw());
                tvAux1raw.setText(String.format("%d", dataAuxRaw(0)));
                tvAux1Voltage.setText(String.format("%.3f", dataAuxVoltage(0)));
                tvAux2raw.setText(String.format("%d", dataAuxRaw(1)));
                tvAux2Voltage.setText(String.format("%.3f", dataAuxVoltage(1)));
                tvXvoltage.setText(dataGetXvoltage());
                tvYvoltage.setText(dataGetYvoltage());
                tvZvoltage.setText(dataGetZvoltage());
                tvStats.setText(dataGetStats());
                tvInfos.setText(dataGetInfos());
                dataRelease();
            }
        };
        consumerTask = new Runnable() {
            @Override
            public void run() {
                int error;

                rtcommInit(0);

                error = samplingOpen();

                if (error != 0) {
                    shouldExit = true;
                    gracefulExit(0, "Failed to open sampling", error);;
                    return;
                }

                while(!shouldExit) {
                    error = samplingRefresh();

                    if (error == 0) {
                        runOnUiThread(refrestTask);
                    } else {
                        shouldExit = true;
                        gracefulExit(0, "Failed to refresh sampling", error);
                        return;
                    }
                }
                error = samplingClose();

                if (error != 0) {
                    shouldExit = true;
                    gracefulExit(0, "Failed to close sampling", error);
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

    void gracefulExit(final int msg_type, final String text, final int status) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                String title;
                boolean shouldExit;

                switch (msg_type) {
                    case 0:
                        title = "Error";
                        shouldExit = true;
                        break;
                    case 1:
                        title = "Warning";
                        shouldExit = true;
                        break;
                    default:
                        title = "Unknown";
                        shouldExit = true;
                        break;
                }
                alertDialog.setTitle(title);
                alertDialog.setMessage(String.format("%s : %d", text, status));

                if (shouldExit) {
                    alertDialog.setButton(AlertDialog.BUTTON_NEUTRAL, "Exit",
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    dialog.dismiss();
                                    android.os.Process.killProcess(android.os.Process.myPid());
                                }
                            });
                } else {
                    alertDialog.setButton(AlertDialog.BUTTON_NEUTRAL, "Continue",
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    dialog.dismiss();
                                }
                            });
                }

                alertDialog.show();
            }
        });
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
    public native int dataAuxRaw(int mchannel);
    public native float dataAuxVoltage(int mchannel);
    public native String dataGetStats();
    public native String dataGetInfos();
    public native void dataAcquire();
    public native void dataRelease();
    public native int samplingOpen();
    public native int samplingRefresh();
    public native int samplingClose();
    public native int rtcommInit(int mode);
}
