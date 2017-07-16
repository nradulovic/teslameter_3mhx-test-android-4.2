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
    private Runnable listenerTask;
    private Runnable informerTask;
    private AlertDialog alertDialog;
    private CdiManager cdiManager;
    private Protocol protocol;
    private CdiManager.RawData rawData;

    TextView hdr_channel;

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
        tvEtempRaw = (TextView) findViewById(R.id.etemp_raw);
        tvEtempFinal = (TextView) findViewById(R.id.etemp_final);

        hdr_channel = (TextView) findViewById(R.id.hdr_channel);

        hdr_channel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, HelloActivity.class);
                startActivity(intent);
            }
        });

        refrestTask = new Runnable() {
            @Override
            public void run() {
                tvXraw.setText(Integer.toString(rawData.x));
                tvYraw.setText(Integer.toString(rawData.y));
                tvZraw.setText(Integer.toString(rawData.z));
                tvAux1raw.setText(Integer.toString(rawData.aux1));
                tvAux2raw.setText(Integer.toString(rawData.aux2));

                tvXvoltage.setText(String.format(Locale.getDefault(), "%.3f", rawData.x * cdiManager.VQUANT));
                tvYvoltage.setText(String.format(Locale.getDefault(), "%.3f", rawData.y * cdiManager.VQUANT));
                tvZvoltage.setText(String.format(Locale.getDefault(), "%.3f", rawData.z * cdiManager.VQUANT));
                tvAux1Voltage.setText(String.format(Locale.getDefault(), "%.3f", rawData.aux1 * cdiManager.VQUANT));
                tvAux2Voltage.setText(String.format(Locale.getDefault(), "%.3f", rawData.aux2 * cdiManager.VQUANT));

                tvEtempRaw.setText(Integer.toString(rawData.etemp));
                tvEtempFinal.setText(String.format(Locale.getDefault(), "%.3f", cdiManager.calculateETemp(rawData.etemp)));

                tvStats.setText(rawData.stats);
                tvInfos.setText(rawData.infos);
            }
        };
        listenerTask = new Runnable() {
            @Override
            public void run() {
                rawData = cdiManager.fetchRawData();
                runOnUiThread(refrestTask);
            }
        };
        informerTask = new Runnable() {
            @Override
            public void run() {
                gracefulExit(0, cdiManager.informations.getMessage(), cdiManager.informations.getSource());
            }
        };
        alertDialog = new AlertDialog.Builder(MainActivity.this).create();

        try {
            cdiManager = new CdiManager(new int[]{1, 1, 1, 1, 1, 0, 0, 1, 3, 2, 5, 4, 20, 1, 1, 1, 1,
                    1, 1, 1, 1, 1, 1, 1, 1, 1000, 1000, 1000, 1000, 3,
                    3, 2});
        } catch (IOException e) {
            e.printStackTrace();
        }
        cdiManager.attachListener(listenerTask);
        cdiManager.attachInformer(informerTask);
        cdiManager.startSampling();

        protocol = new Protocol(cdiManager);
        try {
            protocol.open();
        } catch (IOException e) {
            e.printStackTrace();
        }
        protocol.process_events();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        try {
            cdiManager.stopSampling();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    void gracefulExit(final int msg_type, final String text, final String status) {
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
                alertDialog.setMessage(String.format(Locale.getDefault(), "%s : %s", text, status));

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
}
