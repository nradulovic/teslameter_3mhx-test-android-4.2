package com.teslameter.nr.teslameter;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
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
    private volatile boolean shouldExit;
    private AlertDialog alertDialog;
    private ADT7410 adt7410;
    private CdiManager cdiManager;
    private Protocol protocol;

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
    int[] xRawArray;
    int[] yRawArray;
    int[] zRawArray;
    String stats;
    String infos;
    TextView hdr_channel;

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

        hdr_channel =  (TextView) findViewById(R.id.hdr_channel);

        hdr_channel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this,HelloActivity.class);
                startActivity(intent);
            }
        });
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

                    cdiManager.dataAcquire();
                    xRaw = cdiManager.dataProbeXRaw();
                    yRaw = cdiManager.dataProbeYRaw();
                    zRaw = cdiManager.dataProbeZRaw();
                    aux1Raw = cdiManager.dataAuxRaw(0);
                    aux2Raw = cdiManager.dataAuxRaw(1);
                    xVoltage = cdiManager.dataProbeXVoltage();
                    yVoltage = cdiManager.dataProbeYVoltage();
                    zVoltage = cdiManager.dataProbeZVoltage();
                    aux1Voltage = cdiManager.dataAuxVoltage(0);
                    aux2Voltage = cdiManager.dataAuxVoltage(1);
                    xRawArray = cdiManager.dataProbeXRawArray();
                    yRawArray = cdiManager.dataProbeYRawArray();
                    zRawArray = cdiManager.dataProbeZRawArray();
                    stats = cdiManager.dataGetStats();
                    infos = cdiManager.dataGetInfos();
                    cdiManager.dataRelease();

                    try {
                        val = adt7410.readRawValue();
                    } catch (IOException e) {
                        val = 0;
                    } catch (IllegalStateException e) {
                        val = 0;
                    }
                    etempRaw = val;
                    etempFinal = adt7410.calculateTemp(etempRaw);

                    runOnUiThread(refrestTask);
                }
            }
        };
        producerTask = new Runnable() {
            @Override
            public void run() {
                int error;

                cdiManager.rtcommInit(new int[] {0});

                error = cdiManager.samplingOpen();

                if (error != 0) {
                    shouldExit = true;
                    gracefulExit(0, "Failed to open sampling", error);
                    return;
                }

                while(!shouldExit) {
                    error = cdiManager.samplingRefresh();

                    if (error != 0) {
                        shouldExit = true;
                        gracefulExit(0, "Failed to refresh sampling", error);
                        return;
                    }
                    available.release();
                }
                error = cdiManager.samplingClose();

                if (error != 0) {
                    shouldExit = true;
                    gracefulExit(0, "Failed to close sampling", error);
                    return;
                }
            }
        };
        shouldExit = false;

        alertDialog = new AlertDialog.Builder(MainActivity.this).create();
        cdiManager = new CdiManager( new int[] {1, 1, 1, 1, 1, 0, 0, 1, 3, 2, 5, 4, 20, 1, 1, 1, 1,
                                                1, 1, 1, 1, 1, 1, 1, 1, 1000, 1000, 1000, 1000, 3,
                                                3, 2});

        try {
            this.adt7410 = new ADT7410(2, 0);
        } catch (IOException e) {
            gracefulExit(0, "Failed to initialize ADT7410", 0);
        }
        this.protocol = new Protocol();
        this.protocol.open();
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
                        shouldExit = false;
                        break;
                    default:
                        title = "Unknown";
                        shouldExit = true;
                        break;
                }
                alertDialog.setTitle(title);
                alertDialog.setMessage(String.format(Locale.getDefault(), "%s : %d", text, status));

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

}
