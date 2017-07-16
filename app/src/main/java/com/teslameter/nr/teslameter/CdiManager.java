package com.teslameter.nr.teslameter;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.NoSuchElementException;

/**
 * Created by nenad on 7.6.17..
 */

final class CdiManager {
    public double VREF;
    public double VQUANT;

    public class Informations {
        public Informations(String source, String message) {
            this.source = source;
            this.message = message;
        }
        public String getSource() {
            return source;
        }
        public String getMessage() {
            return message;
        }
        private String source;
        private String message;
    }
    public class RawData {
        /*
         * Regular constructor
         */
        public RawData(int[] xArray, int[] yArray, int[] zArray, int x, int y, int z, int aux1,
                       int aux2, int etemp, String stats, String infos) {
            this.xArray = xArray;
            this.yArray = yArray;
            this.zArray = zArray;
            this.x = x;
            this.y = y;
            this.z = z;
            this.aux1 = aux1;
            this.aux2 = aux2;
            this.etemp = etemp;
            this.stats = stats;
            this.infos = infos;
        }
        /*
         * Copy constructor
         */
        public RawData(RawData aRawData) {
            this(aRawData.xArray, aRawData.yArray, aRawData.zArray, aRawData.x,
                    aRawData.y, aRawData.z, aRawData.aux1, aRawData.aux2,
                    aRawData.etemp, aRawData.stats, aRawData.infos);
        }

        public int[] xArray;
        public int[] yArray;
        public int[] zArray;
        public int x;
        public int y;
        public int z;
        public int aux1;
        public int aux2;
        public int etemp;
        public String stats;
        public String infos;
    }

    public RawData fetchRawData() {
        //return new RawData(rawData);
        return rawData;
    }

    private RawData rawData;

    public Informations informations;

    public CdiManager() throws IOException {
        commonInitialization();
    }

    public CdiManager(int[] configArray) throws IOException {
        if (configArray.length != this.configKeys.length) {
            throw new IllegalArgumentException(String.format(Locale.getDefault(),
                    "configArray argument must have %d elements, but has %d elements",
                    this.configKeys.length, configArray.length));
        }

        for (int i = 0; i < this.configKeys.length; i++) {
            this.configKeys[i].setValue(configArray[i]);
        }
        commonInitialization();
    }

    public void setConfigKey(String name, int value) throws NoSuchElementException {
        for (ParamKey configKey : this.configKeys) {
            if (configKey.getName().equals(name)) {
                configKey.setValue(value);
                return;
            }
        }
        throw new NoSuchElementException();
    }

    public void startSampling() {
        shouldExit = false;

        Runnable producerTask = new Runnable() {
            @Override
            public void run() {
                int error;

                rtcommInit(new int[] {0});

                error = samplingOpen();

                if (error != 0) {
                    shouldExit = true;
                    informations = new Informations("producer.samplingOpen()", Integer.toString(error));
                    executeInformers();
                    return;
                }

                while(!shouldExit) {
                    samplingRefresh();
                }
                error = samplingClose();

                if (error != 0) {
                    shouldExit = true;
                    informations = new Informations("producer.samplingClose()", Integer.toString(error));
                    executeInformers();
                    return;
                }
            }
        };

        Runnable consumerTask = new Runnable() {
            @Override
            public void run() {

                while (!shouldExit) {
                    int val;
                    try {
                        val = adt7410.readRawValue();
                    } catch (IOException e) {
                        val = 0;
                    } catch (IllegalStateException e) {
                        val = 0;
                    }
                    dataAcquire();
                    rawData = new RawData(
                            dataProbeXRawArray(), dataProbeYRawArray(), dataProbeZRawArray(),
                            dataProbeXRaw(), dataProbeYRaw(), dataProbeZRaw(), dataAuxRaw(0),
                            dataAuxRaw(1), val, dataGetStats(), dataGetInfos());
                    dataRelease();

                    executeListeners();
                }
            }
        };
        producerThread = new Thread(producerTask);
        producerThread.setPriority(Thread.MAX_PRIORITY);
        producerThread.start();
        consumerThread = new Thread(consumerTask);
        consumerThread.setPriority(Thread.MAX_PRIORITY);
        consumerThread.start();
    }

    public void stopSampling() throws InterruptedException {
        shouldExit = true;
        producerThread.join(300);
        consumerThread.join(300);
    }

    public void attachListener(Runnable runnable) {
         listeners.add(runnable);
    }

    public void attachInformer(Runnable runnable) {
        informers.add(runnable);
    }

    public double calculateETemp(int input) {
        return adt7410.calculateTemp(input);
    }

    private class ParamKey {
        ParamKey(String type, String name, int defaultValue) {
            this.type = type;
            this.name = name;
            this.value = defaultValue;
        }
        public String getType() {
            return type;
        }

        public String getName() {
            return name;
        }

        public int getValue() {
            return value;
        }

        public void setValue(int value) {
            String type_name = "unknown";
            boolean should_throw = true;

            if (this.type.equals("bool")) {
                type_name = "bool";
                should_throw = !((value == 0) || (value == 1));
            } else if (this.type.equals("uint2")) {
                type_name = "unsigned integer (2-bit)";
                should_throw = !((value >= 0) || (value < 4));
            } else if (this.type.equals("uint3")) {
                type_name = "unsigned integer (3-bit)";
                should_throw = !((value >= 0) || (value < 8));
            } else if (this.type.equals("uint4")) {
                type_name = "unsigned integer (4-bit)";
                should_throw = !((value >= 0) || (value < 16));
            } else if (this.type.equals("int")) {
                type_name = "signed integer (32-bit)";
                should_throw = !((value >= 0) || (value < Integer.MAX_VALUE));
            }

            if (should_throw) {
                throw new IllegalArgumentException(String.format(Locale.getDefault(),
                        "type %s does not accept value %d", type_name, value));
            }
            this.value = value;
        }
        private String type;
        private String name;
        private int value;
    }

    private ParamKey[] configKeys = {
            new ParamKey("bool",  "en_x",             1),
            new ParamKey("bool",  "en_y",             1),
            new ParamKey("bool",  "en_z",             1),
            new ParamKey("bool",  "en_aux1",          1),
            new ParamKey("bool",  "en_aux2",          1),
            new ParamKey("bool",  "en_probe_buffer",  0),
            new ParamKey("bool",  "en_aux_buffer",    0),
            new ParamKey("uint2", "probe_pga",        1),
            new ParamKey("uint3", "aux1_mux_hi",      3),
            new ParamKey("uint3", "aux1_mux_lo",      2),
            new ParamKey("uint3", "aux2_mux_hi",      5),
            new ParamKey("uint3", "aux2_mux_lo",      4),
            new ParamKey("int",   "tcgain",           20),
            new ParamKey("int",   "tgain1hi",         1),
            new ParamKey("int",   "vgain1hi",         1),
            new ParamKey("int",   "tgain2hi",         1),
            new ParamKey("int",   "tgain2lo",         1),
            new ParamKey("int",   "vgain2hi",         1),
            new ParamKey("int",   "vgain2lo",         1),
            new ParamKey("int",   "tgain3hi",         1),
            new ParamKey("int",   "tgain3lo",         1),
            new ParamKey("int",   "vgain3hi",         1),
            new ParamKey("int",   "vgain3lo",         1),
            new ParamKey("int",   "tgain4lo",         1),
            new ParamKey("int",   "vgain4lo",         1),
            new ParamKey("int",   "gainlvl1",         1000),
            new ParamKey("int",   "gainlvl2",         1000),
            new ParamKey("int",   "gainlvl3",         1000),
            new ParamKey("int",   "gainlvl4",         1000),
            new ParamKey("uint4", "displayfrequency", 3),
            new ParamKey("uint3", "probe_mux_hi",     3),
            new ParamKey("uint3", "probe_mux_lo",     2),
    };
    private ParamKey [] paramKeys = {
            new ParamKey("bool",    "en_autorange",     0),
            new ParamKey("int",     "vspeed",           14),
            new ParamKey("int",     "workmode",         0)
    };
    private volatile boolean shouldExit;
    private Thread producerThread;
    private Thread consumerThread;
    private ADT7410 adt7410;
    private List<Runnable> listeners;
    private List<Runnable> informers;


    private void commonInitialization() throws IOException {
        adt7410 = new ADT7410(2, 0);
        listeners = new ArrayList<Runnable>();
        informers = new ArrayList<Runnable>();
        informations = new Informations("none", "none");
        VREF = 2.5;
        VQUANT = ((VREF * 2.0) / (8388608 - 1)) * 1000.0;
    }
    private void executeListeners() {
        for (Runnable element : listeners) {
            element.run();
        }
    }
    private void executeInformers() {
        for (Runnable element : informers) {
            element.run();
        }
    }
    private void configureFirmware() {

    }

    private native int dataProbeXRaw();
    private native int dataProbeYRaw();
    private native int dataProbeZRaw();
    private native int dataAuxRaw(int mchannel);
    private native float dataProbeXVoltage();
    private native float dataProbeYVoltage();
    private native float dataProbeZVoltage();
    private native float dataAuxVoltage(int mchannel);
    private native int[] dataProbeXRawArray();
    private native int[] dataProbeYRawArray();
    private native int[] dataProbeZRawArray();
    private native String dataGetStats();
    private native String dataGetInfos();
    private native int dataAcquire();
    private native int dataRelease();
    private native int samplingOpen();
    private native int samplingRefresh();
    private native int samplingClose();
    private native int rtcommInit(int[] config);

    // Used to load the 'cdi_manager' library on application startup.
    static {
        System.loadLibrary("cdi_manager");
    }
}
