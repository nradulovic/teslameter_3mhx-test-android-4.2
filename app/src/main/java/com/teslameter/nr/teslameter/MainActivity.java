package com.teslameter.nr.teslameter;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.widget.TextView;

import java.io.IOException;
import java.util.Locale;
import java.util.concurrent.Semaphore;

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
    private TextView tvEtempRaw;
    private TextView tvEtempFinal;
    private Runnable refrestTask;
    private Runnable producerTask;
    private Runnable consumerTask;
    private Thread producerThread;
    private Thread consumerThread;
    private Semaphore available;
    private boolean shouldExit;
    private AlertDialog alertDialog;
    private ADT7410 adt7410;

    int xRaw;
    int yRaw;
    int zRaw;
    int aux1Raw;
    int aux2Raw;
    int etempRaw = 0;
    float xVoltage;
    float yVoltage;
    float zVoltage;
    float aux1Voltage;
    float aux2Voltage;
    float etempFinal;
    String stats;
    String infos;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        tvXraw =        (TextView) findViewById(R.id.x_raw);
        tvYraw =        (TextView) findViewById(R.id.y_raw);
        tvZraw =        (TextView) findViewById(R.id.z_raw);
        tvAux1raw =     (TextView) findViewById(R.id.aux1_raw);
        tvAux2raw =     (TextView) findViewById(R.id.aux2_raw);
        tvAux1Voltage = (TextView) findViewById(R.id.aux1_voltage);
        tvAux2Voltage = (TextView) findViewById(R.id.aux2_voltage);
        tvXvoltage =    (TextView) findViewById(R.id.x_voltage);
        tvYvoltage =    (TextView) findViewById(R.id.y_voltage);
        tvZvoltage =    (TextView) findViewById(R.id.z_voltage);
        tvStats =       (TextView) findViewById(R.id.stats);
        tvInfos =       (TextView) findViewById(R.id.infos);
        tvEtempRaw =    (TextView) findViewById(R.id.etemp_raw);
        tvEtempFinal =  (TextView) findViewById(R.id.etemp_final);
        available = new Semaphore(0);

        refrestTask = new Runnable() {
            @Override
            public void run() {
                tvXraw.setText(String.format(Locale.getDefault(), "%d",xRaw));
                tvYraw.setText(String.format(Locale.getDefault(), "%d",yRaw));
                tvZraw.setText(String.format(Locale.getDefault(), "%d",zRaw));
                tvAux1raw.setText(String.format(Locale.getDefault(), "%d", aux1Raw));
                tvAux2raw.setText(String.format(Locale.getDefault(), "%d", aux2Raw));

                tvXvoltage.setText(String.format(Locale.getDefault(), "%.3f", xVoltage));
                tvYvoltage.setText(String.format(Locale.getDefault(), "%.3f", yVoltage));
                tvZvoltage.setText(String.format(Locale.getDefault(), "%.3f", zVoltage));
                tvAux1Voltage.setText(String.format(Locale.getDefault(), "%.3f", aux1Voltage));
                tvAux2Voltage.setText(String.format(Locale.getDefault(), "%.3f", aux2Voltage));

                tvEtempRaw.setText(String.format(Locale.getDefault(), "%d", etempRaw));
                tvEtempFinal.setText(String.format(Locale.getDefault(), "%.3f", etempFinal));

                tvStats.setText(stats);
                tvInfos.setText(infos);
            }
        };
        consumerTask = new Runnable() {
            @Override
            public void run() {
                while (true) {
                    int val;

                    try {
                        available.acquire();
                    } catch (InterruptedException e) {
                        break;
                    }

                    dataAcquire();
                    xRaw = dataProbeXRaw();
                    yRaw = dataProbeYRaw();
                    zRaw = dataProbeZRaw();
                    aux1Raw = dataAuxRaw(0);
                    aux2Raw = dataAuxRaw(1);
                    xVoltage = dataProbeXVoltage();
                    yVoltage = dataProbeYVoltage();
                    zVoltage = dataProbeZVoltage();
                    aux1Voltage = dataAuxVoltage(0);
                    aux2Voltage = dataAuxVoltage(1);
                    stats = dataGetStats();
                    infos = dataGetInfos();
                    dataRelease();

                    try {
                        val = adt7410.readRawValue();
                    } catch (IOException e) {
                        val = 0;
                    }
                    etempRaw = val;
                    etempFinal = adt7410.getTemp(etempRaw);

                    runOnUiThread(refrestTask);
                }
            }
        };
        producerTask = new Runnable() {
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

                    if (error != 0) {
                        shouldExit = true;
                        gracefulExit(0, "Failed to refresh sampling", error);
                        return;
                    }
                    available.release();
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

        rtcommInit(0);

        try {
            this.adt7410 = new ADT7410(2, 0);
        } catch (IOException e) {
            gracefulExit(0, "Failed to initialize ADT7410", 0);
        }
        consumerThread = new Thread(consumerTask);
        consumerThread.setPriority(Thread.MAX_PRIORITY);
        consumerThread.start();
        producerThread = new Thread(producerTask);
        producerThread.setPriority(Thread.MAX_PRIORITY);
        producerThread.start();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        shouldExit = true;
        try {
            producerThread.join(300);
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
    public native int dataProbeXRaw();
    public native int dataProbeYRaw();
    public native int dataProbeZRaw();
    public native int dataAuxRaw(int mchannel);

    public native float dataProbeXVoltage();
    public native float dataProbeYVoltage();
    public native float dataProbeZVoltage();
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
