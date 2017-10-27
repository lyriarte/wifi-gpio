## WiFi GPIO

Setup a web server to control all 4 GPIOs on a ESP-01 device. 

### Operation

The arduino code first goes through a predefined list of WiFi access points, if none is available it raises its own AP called **ESP_XXXXXX** where XXXXXX are the ESP-01 MAC address 6 last hex digits. In AP mode, default address is **192.168.64.1**

The code then raises a web server that turns the GPIOs on or off according to the URI.

### Examples

  * http://192.168.64.1/1001 turns on GPIO 0 and GPIO 3

```
1001
```

  * http://192.168.64.1/1100 turns on GPIO 0 and GPIO 1

```
1100
```

  * http://192.168.64.1/ shows the GPIOs status

```
1100
```

  * http://192.168.64.1/0000 turns off all four GPIOs

```
0000
```


