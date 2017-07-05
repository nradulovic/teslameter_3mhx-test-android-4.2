package com.teslameter.nr.teslameter;

import java.util.Locale;
import java.util.NoSuchElementException;

/**
 * Created by nenad on 7.6.17..
 */

final class CdiManager {
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

    public CdiManager() {

    }

    public CdiManager(int [] configArray) {
        if (configArray.length != this.configKeys.length) {
            throw new IllegalArgumentException(String.format(Locale.getDefault(),
                    "configArray argument must have %d elements, but has %d elements",
                    this.configKeys.length, configArray.length));
        }

        for (int i = 0; i < this.configKeys.length; i++) {
            this.configKeys[i].setValue(configArray[i]);
        }
    }

    public void setConfigKey(String name, int value) throws NoSuchElementException {
        for (ParamKey configKey : this.configKeys) {
            if (configKey.getName().equals(name)) {
                configKey.setValue(value);
                break;
            }
        }
        throw new NoSuchElementException();
    }

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

    public native int dataAcquire();
    public native int dataRelease();

    public native int samplingOpen();
    public native int samplingRefresh();
    public native int samplingClose();
    public native int rtcommInit(int [] config);

    private void configureFirmware() {

    }
    // Used to load the 'cdi_manager' library on application startup.
    static {
        System.loadLibrary("cdi_manager");
    }
}
