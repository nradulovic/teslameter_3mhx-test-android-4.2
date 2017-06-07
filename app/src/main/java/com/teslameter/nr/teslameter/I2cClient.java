package com.teslameter.nr.teslameter;

import java.io.IOException;
import java.util.Locale;

/**
 * Created by nenad on 6/7/17.
 */

public class I2cClient {
    /* JNI stuff */
    private native int i2cWrReg(int bus_id, int chip_address, int reg, int data);
    private native int i2cRdReg(int bus_id, int chip_address, int reg);
    private native int [] i2cRdBuf(int bus_id, int chip_address, int reg, int bufsize);

    /* Data */
    private int bus_id;
    private int chip_id;

    /* Private methods */
    private void assert_arg_reg(int reg) {
        if ((reg < 0) || (reg > 255))  {
            throw new IllegalArgumentException(String.format(Locale.getDefault(),
                    "Argument reg = %d is out of range: [0, 255]", reg));
        }
    }

    /* Public methods */
    I2cClient(int bus_id, int chip_id) {
        this.bus_id = bus_id;
        this.chip_id = chip_id;
    }

    public void i2cWriteReg(int reg, int data) throws IOException {
        this.assert_arg_reg(reg);

        if ((data < 0) || (data > 255)) {
            throw new IllegalArgumentException(String.format(Locale.getDefault(),
                    "Argument data = %d is out of range: [0, 255]", data));
        }

        int retval = this.i2cWrReg(this.bus_id, this.chip_id, reg, data);

        if (retval < 0) {
            throw new IOException(String.format(Locale.getDefault(),
                    "Failed to write to register %d: error %d", reg, retval));
        }
    }

    public int i2cReadReg(int reg) throws IOException {
        this.assert_arg_reg(reg);

        int retval = this.i2cRdReg(this.bus_id, this.chip_id, reg);

        if (retval < 0) {
            throw new IOException(String.format(Locale.getDefault(),
                    "Failed to write to register %d: error %d", reg, retval));
        }

        return retval;
    }

    public int [] i2cReadBuf(int reg, int bufsize) throws IOException {
        this.assert_arg_reg(reg);

        int [] retval = this.i2cRdBuf(this.bus_id, this.chip_id, reg, bufsize);

        if (retval == null) {
            throw new IOException(String.format(Locale.getDefault(),
                    "Failed to write to register %d: error: no memory"));
        } else if (retval[0] < 0) {
            throw new IOException(String.format(Locale.getDefault(),
                    "Failed to write to register %d: error %d", retval[0]));
        }
        return retval;
    }
}
