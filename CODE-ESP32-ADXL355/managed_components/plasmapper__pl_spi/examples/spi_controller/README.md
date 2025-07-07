# SPI Controller Example

1. SPI interface is initialized (host: HSPI_HOST, MOSI pin: 5, MISO pin: 18, SCLK pin: 19).
2. SPI controller is initialized (SPI mode: 0, frequency: 10 MHz, CS pin: 21, number of address bits: 8, max SCLK-MISO delay: 30 ns).
3. 2 bytes are written to the target.
4. 2 bytes are read from the target.
