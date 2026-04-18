# CORE FUNCTION MODULES / COMPONENTS LIST
!!! note 
    Core function development focuses on developing general node functions. As for functions that are specific to certain applications or scenarios, we will introduce them in the development of extended functions. The following is a list of core function modules/components, presented in the recommended development order:

| Category      | Function          | Component/Module     | Remarks          |
| ------------- | ----------------- | -------------------- | ---------------- |
| Actuation     | LED Indicator Control | node_led            |  Used to feedback node status     |
| Actuation     | RGB Indicator Control | node_rgb            |  Used to feedback node status     |
| Control       | External Interrupt | node_exti           |  Used for external interrupt handling and response |
| Control       | Timer             | node_timer          |  Used for periodic background task processing |
| Control       | Real-Time Clock   | node_rtc            |  Used for time management and timed wake-up |
| Control       | Random Number Generator | node_rng            |  Used to generate random numbers       |
| Control       | I2C Bus          | node_i2c            |  Used to connect I2C devices    |
| Control       | SPI Bus          | node_spi            |  Used to connect SPI devices    |
| Actuation     | SD Card Storage         | node_sdcard        |  Used for data storage        |
| Communication | WIFI Wireless Communication     | node_wifi          |  Used for wireless network connection     |
| Communication | IoT Communication        | node_mqtt           |  Used for IoT data interaction    |
<!-- | Actuation     | LCD Display       | node_lcd            |  Used to display information        | -->


The version that includes the above functions is named AIoTNode-CORE, and we will develop the AIoTNode-MORE version based on this for extended function development.