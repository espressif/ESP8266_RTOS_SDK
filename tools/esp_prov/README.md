# ESP Provisioning Tool

# NAME
`esp_prov` - A python based utility for testing the provisioning examples over a host using Httpd on a soft AP interface.

# SYNOPSIS

```
python esp_prov.py --ssid < AP SSID > --passphrase < AP Password > --sec_ver < Security version 0 / 1 > [ Optional parameters... ]
```

# DESCRIPTION

For SoftAP + HTTPD based provisioning. This assumes that the device is running in Wi-Fi SoftAP mode and hosts an HTTP server supporting specific endpoint URIs. Also client needs to connect to the device softAP network before running `esp_prov`. 

Usage of `esp-prov` assumes that the provisioning app has specific protocomm endpoints active. These endpoints are active in the provisioning examples and accept specific protobuf data structures:

| Endpoint Name | URI (HTTP server on ip:port) | Description                                               |
|---------------|------------------------------|-----------------------------------------------------------|
| prov-session  | http://ip:port/prov-session  | Security endpoint used for session establishment          |
| prov-config   | http://ip:port/prov-config   | Endpoint used for configuring Wi-Fi credentials on device |
| proto-ver     | http://ip:port/proto-ver     | Version endpoint for checking protocol compatibility      |
| custom-config | http://ip:port/custom-config | Optional endpoint for configuring custom credentials      |

# PARAMETERS

* `--help`
    Print the list of options along with brief descriptions

* `--verbose`, `-v`
    Sets the verbosity level of output log

* `--ssid <AP SSID>`
    For specifying the SSID of the Wi-Fi AP to which the device is to connect after provisioning

* `--passphrase <AP Password>`
    For specifying the password of the Wi-Fi AP to which the device is to connect after provisioning

* `--sec_ver <Security version number>`
    For specifying version of protocomm endpoint security to use. For now two versions are supported:
    * `0` for `protocomm_security0`
    * `1` for `protocomm_security1`

* `--pop <Proof of possession string>` (Optional)
    For specifying optional Proof of Possession string to use for protocomm endpoint security version 1. This option is ignored when security version 0 is in use

* `--proto_ver <Provisioning application version string>` (Optional) (Default `V0.1`)
    For specifying version string for checking compatibility with provisioning app prior to starting provisioning process

* `--softap_endpoint <softap_ip:port>` (Optional) (Default `192.168.4.1:80`)
    For specifying the IP and port of the HTTP server on which provisioning app is running. The client must connect to the device SoftAP prior to running `esp_prov`

* `--custom_config` (Optional)
    This flag assumes the provisioning app has an endpoint called `custom-config`. Use `--custom_info` and `--custom_ver` options to specify the fields accepted by this endpoint

* `--custom_info <some string>` (Optional) (Only use along with `--custom_config`)
    For specifying an information string to be sent to the `custom-config` endpoint during provisioning

* `--custom_ver <some integer>` (Optional) (Only use along with `--custom_config`)
    For specifying a version number (int) to be sent to the `custom-config` endpoint during provisioning

# AVAILABILITY

`esp_prov` is intended as a cross-platform tool.

For android, a provisioning tool along with source code is available [here](https://github.com/espressif/esp-idf-provisioning-android)

## Dependencies

This requires the following python libraries to run (included in requirements.txt):
* `future`
* `protobuf`
* `cryptography`

Run `pip install -r $IDF_PATH/tools/esp_prov/requirements.txt`

Note :
* The packages listed in requirements.txt are limited only to the ones needed AFTER fully satisfying the requirements of ESP8266_RTOS_SDK

# EXAMPLE USAGE

Please refer to the README.md files with the examples present under `$IDF_PATH/examples/provisioning/`, namely:

* `softap_prov`
* `custom_config`

Each of these examples use specific options of the `esp_prov` tool and give an overview to simple as well as advanced usage scenarios.
