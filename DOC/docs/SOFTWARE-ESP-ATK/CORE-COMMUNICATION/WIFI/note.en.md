# NOTE

!!! note
    WiFi is a builtin function for almost all ESP32 dev boards. It works in either "AP (Access Point)" or "STA (Station)" mode or simultaneously in both modes. Herein, we mainly use the ESP32 as a station to connect to the router and access the Internet.

!!! tip
    Herein, we make the node work in AP mode with two access methods: one is to connect to a regular personal WiFi network; the other is to operate in an enterprise WiFi network mode, specifically, we use WPA2 Enterprise mode with PEAP-MSCHAPV2 authentication.