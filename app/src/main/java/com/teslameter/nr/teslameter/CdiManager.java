package com.teslameter.nr.teslameter;

import java.util.NoSuchElementException;

/**
 * Created by nenad on 7.6.17..
 */

final class CdiManager {
    class ParamKey {
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
            this.value = value;
        }
        private String type;
        private String name;


        private int value;
    }
    private ParamKey[] configKey = {
            new ParamKey("bool",  "en_x",             1),
            new ParamKey("bool",  "en_y",             1),
            new ParamKey("bool",  "en_z",             1),
            new ParamKey("bool",  "en_aux1",          1),
            new ParamKey("bool",  "en_aux2",          1),
            new ParamKey("bool",  "en_probe_buffer",  0),
            new ParamKey("bool",  "en_aux_buffer",    0),
            new ParamKey("int",   "probe_pga",        1),
            new ParamKey("int",   "aux1_mux_hi",      3),
            new ParamKey("int",   "aux1_mux_lo",      2),
            new ParamKey("int",   "aux2_mux_hi",      5),
            new ParamKey("int",   "aux2_mux_lo",      4),
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
            new ParamKey("int",   "displayfrequency", 3),
            new ParamKey("int",   "probe_mux_hi",     3),
            new ParamKey("int",   "probe_mux_lo",     2),
    };
    private ParamKey [] paramKeys = {
            new ParamKey("bool",    "en_autorange",     0),
            new ParamKey("int",     "vspeed",           14),
            new ParamKey("int",     "workmode",         0)
    };

    CdiManager() {

    }
    void setConfigKey(String name, int value) throws NoSuchElementException {
        for (ParamKey configKey : this.configKey) {
            if (configKey.getName().equals(name)) {
                configKey.setValue(value);
                break;
            }
        }
        throw new NoSuchElementException();
    }
}
