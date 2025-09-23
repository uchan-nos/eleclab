#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/drivers/gpio.h>

#define DRV8830_ADDR (0xC8 >> 1)
#define DRV8830_CONTROL 0x00
#define DRV8830_FAULT   0x01

static const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(green_led_2), gpios);

enum Direction {
    DIR_STANDBY,
    DIR_FORWARD,
    DIR_REVERSE,
    DIR_BRAKE
};

int drv8830_set_control(uint8_t vset, enum Direction dir) {
    uint8_t msg[2] = {DRV8830_CONTROL, (vset << 2) | dir};
    // i2c_write/read に指定する addr は 7 ビット右詰め。
    return i2c_write(i2c_dev, msg, 2, DRV8830_ADDR);
}

int drv8830_clear_fault(void) {
    uint8_t msg[2] = {DRV8830_FAULT, 0x80};
    return i2c_write(i2c_dev, msg, 2, DRV8830_ADDR);
}

int drv8830_get_fault(void) {
    struct i2c_msg msgs[2];
    uint8_t send_buf[1] = {DRV8830_FAULT};
    uint8_t recv_buf[1];

    msgs[0].buf = send_buf;
    msgs[0].len = 1;
    msgs[0].flags = I2C_MSG_WRITE;

    msgs[1].buf = recv_buf;
    msgs[1].len = 1;
    msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

    int res = i2c_transfer(i2c_dev, msgs, 2, DRV8830_ADDR);
    if (res < 0) {
        return res;
    }

    return recv_buf[0];
}

int main(void) {
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    drv8830_clear_fault();

    for (int val = 0x06; val <= 0x3f; ++val) {
        drv8830_set_control(val, DIR_FORWARD);
        printk("set control. fault = %02x\n", drv8830_get_fault());
        k_msleep(300);

        gpio_pin_set_dt(&led, (val >> 1) & 1);
    }

    return 0;
}
