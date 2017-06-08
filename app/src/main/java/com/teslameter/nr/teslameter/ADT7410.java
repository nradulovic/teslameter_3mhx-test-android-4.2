package com.teslameter.nr.teslameter;

import java.io.IOException;
import java.util.Locale;

/**
 * Created by nenad on 6/7/17.
 */

final class ADT7410 {
    private static final int CHIP_I2C_ID                = 0x48;
    private static final int ADT7410_REG_TEMP_MSB       = 0x00;
    private static final int ADT7410_REG_TEMP_LSB       = 0x01;
    private static final int ADT7410_REG_STATUS         = 0x02;
    private static final int ADT7410_REG_CONFIGURATION  = 0x03;
    private static final int ADT7410_REG_ID             = 0x0b;
    private static final int ADT7410_REG_RESET          = 0x2f;
    private static final int REG_STATUS_NRDY                = (0x1 << 7);
    private static final int REG_CONFIGURATION_RESOLUTION   = (0x1 << 7);
    private I2cSlave i2c;

    ADT7410(int bus_id, int chip_id) throws IOException {

        if ((chip_id != 0) && (chip_id != 1)) {
            throw new IllegalArgumentException(String.format(Locale.getDefault(),
                    "Argument chip_id = %d is out of range: [0, 1]", chip_id));
        }
        this.i2c = new I2cSlave(bus_id, ADT7410.CHIP_I2C_ID);

        /* Reset the chip */
        this.i2c.writeReg(ADT7410.ADT7410_REG_RESET, 0x11);
        /* Enable 16-bit mode */
        this.i2c.writeReg(ADT7410.ADT7410_REG_CONFIGURATION, ADT7410.REG_CONFIGURATION_RESOLUTION);
    }

    int readRawValue() throws IOException, IllegalStateException {
        /* Check if data is available */
        int status = this.i2c.readReg(ADT7410.ADT7410_REG_STATUS);

        if ((status & ADT7410.REG_STATUS_NRDY) != 0) {
            throw new IllegalStateException("Not ready");
        }

        int [] buf = this.i2c.readBuf(ADT7410.ADT7410_REG_TEMP_MSB, 2);

        return (buf[0] * 256 + buf[1]);
    }

    float calculateTemp(int rawValue) {
        return ((float)(rawValue / 128.0));
    }
}
