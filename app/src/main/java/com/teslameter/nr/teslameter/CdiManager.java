package com.teslameter.nr.teslameter;

import java.io.ObjectStreamClass;
import java.util.NoSuchElementException;

/**
 * Created by nenad on 7.6.17..
 */

final class CdiManager {
    class Config {
        Config(String type, String name, int defaultValue) {
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
    private Config [] configs = {
            new Config("bool",  "en_x",             1),
            new Config("bool",  "en_y",             1),
            new Config("bool",  "en_z",             1),
            new Config("bool",  "en_aux1",          1),
            new Config("bool",  "en_aux2",          1),
            new Config("bool",  "en_probe_buffer",  0),
            new Config("bool",  "en_aux_buffer",    0),
            new Config("int",   "probe_pga",        1),
            new Config("int",   "aux1_mux_hi",      3),
            new Config("int",   "aux1_mux_lo",      2),
            new Config("int",   "aux2_mux_hi",      5),
            new Config("int",   "aux2_mux_lo",      4),
            new Config("int",   "tcgain",           20),
            new Config("int",   "tgain1hi",         1),
            new Config("int",   "vgain1hi",         1),
            new Config("int",   "tgain2hi",         1),
            new Config("int",   "tgain2lo",         1),
            new Config("int",   "vgain2hi",         1),
            new Config("int",   "vgain2lo",         1),
            new Config("int",   "tgain3hi",         1),
            new Config("int",   "tgain3lo",         1),
            new Config("int",   "vgain3hi",         1),
            new Config("int",   "vgain3lo",         1),
            new Config("int",   "tgain4lo",         1),
            new Config("int",   "vgain4lo",         1),
            new Config("int",   "gainlvl1",         1),
            new Config("int",   "gainlvl1",         1000),
            new Config("int",   "gainlvl2",         1000),
            new Config("int",   "gainlvl3",         1000),
            new Config("int",   "gainlvl4",         1000),
            new Config("int",   "displayfrequency", 3),
            new Config("int",   "probe_mux_hi",     3),
            new Config("int",   "probe_mux_lo",     2),
    };

    CdiManager() {

    }
    void setConfig(String name, int value) throws NoSuchElementException {
        for (Config configKey : this.configs) {
            if (Objects.eq configKey.getName() == name) {
                configKey.setValue(value);
                break;
            }
        }
        throw new NoSuchElementException();
    }
}
