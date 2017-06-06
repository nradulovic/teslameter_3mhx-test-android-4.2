package com.teslameter.nr.teslameter;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.widget.TextView;

import java.util.Locale;

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
    private AlertDialog alertDialog;

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

        refrestTask = new Runnable() {
            @Override
            public void run() {
                while (dataAcquire()) {
                    int xRaw = dataProbeXRaw();
                    int yRaw = dataProbeYRaw();
                    int zRaw = dataProbeZRaw();
                    int aux1Raw = dataAuxRaw(0);
                    int aux2Raw = dataAuxRaw(1);
                    float xVoltage = dataProbeXVoltage();
                    float yVoltage = dataProbeYVoltage();
                    float zVoltage = dataProbeZVoltage();
                    float aux1Voltage = dataAuxVoltage(0);
                    float aux2Voltage = dataAuxVoltage(1);
                    String stats = dataGetStats();
                    String infos = dataGetInfos();
                    dataRelease();

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

                    tvStats.setText(stats);
                    tvInfos.setText(infos);
                }
            }
        };
        consumerTask = new Runnable() {
            @Override
            public void run() {
                int error;

                rtcommInit(0);

                error = samplingOpen();

                if (error == 0) {
                    runOnUiThread(refrestTask);
                } else {
                    gracefulExit(0, "Failed to open sampling", error);;
                    return;
                }
                error = samplingRefresh();

                if (error != 0) {
                    samplingClose();
                    gracefulExit(0, "Failed to refresh sampling", error);
                    return;
                }
                error = samplingClose();

                if (error != 0) {
                    gracefulExit(0, "Failed to close sampling", error);
                    return;
                }
            }
        };

        alertDialog = new AlertDialog.Builder(MainActivity.this).create();

        consumerThread = new Thread(consumerTask);
        consumerThread.setPriority(Thread.MAX_PRIORITY);
        consumerThread.start();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        samplingClose();
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

    public native boolean dataAcquire();
    public native void dataRelease();

    public native int samplingOpen();
    public native int samplingRefresh();
    public native int samplingClose();
    public native int rtcommInit(int mode);
}
