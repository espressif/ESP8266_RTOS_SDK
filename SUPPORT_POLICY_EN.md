The latest support policy for ESP8266 RTOS SDK can be found at [Support Policy](./SUPPORT_POLICY_EN.md).

Support Period Policy
=================

* [中文版](./SUPPORT_POLICY_CN.md)

Each ESP8266 RTOS SDK major and minor release (V3.0, V3.1, etc) is supported for 18 months after the initial stable release date.

Supported means that the ESP8266 RTOS SDK team will continue to apply bug fixes, security fixes, etc to the release branch on GitHub, and periodically make new bugfix releases as needed.

Users are encouraged to upgrade to a newer ESP8266 RTOS SDK release before the support period finishes and the release becomes End of Life (EOL). It is our policy to not continue fixing bugs in End of Life releases.

Pre-release versions (betas, previews, -rc and -dev versions, etc) are not covered by any support period. Sometimes a particular feature is marked as "Preview" in a release, which means it is also not covered by the support period.


Long Term Support releases
------------

Some releases (for example, ESP8266 RTOS SDK V3.1) are designated Long Term Support (LTS). LTS releases are supported for 30 months (2.5 years) after the initial stable release date.

We will add a `(LTS)` tag when we release a long term support version on GitHub at the first time. For example:

```
ESP8266 RTOS SDK Release v3.4 (LTS)
```

But we will not add `(LTS)` tag to the following bug fix versions. For example:

```
ESP8266 RTOS SDK Release v3.4.1
```

Example
-----

ESP8266 RTOS SDK V3.1 was released in January 2019 and is a Long Term Support (LTS) release, meaning it will be supported for 30 months until July 2021.

- The first V3.1 release was `v3.1` in January 2019.
- The ESP8266 RTOS SDK team continues to backport bug fixes, security fixes, etc to the release branch `release/v3.1`。
- Periodically stable bugfix releases are created from the release branch. For example `v3.1.1`、`v3.1.2`, etc. Users are encouraged to always update to the latest bugfix release.
- V3.1 bugfix releases continue until July 2021, when all V3.1.x releases become End of Life.

Existing Releases
--------

ESP8266 RTOS SDK V3.3 and all newer releases will follow this support period policy. The support period for each release will be announced when the release is made.

For releases made before the support period policy was announced, the following support periods apply:

- ESP8266 RTOS SDK V3.2.x will be supported until December 2021.
- ESP8266 RTOS SDK V3.2.x will be supported until December 2020.
- ESP8266 RTOS SDK V3.1.x is Long Term Support (LTS) release, will be supported until July 2021.
- ESP8266 RTOS SDK V3.0.x will be supported until October 2020.
- ESP8266 RTOS SDK V2.1.x is Long Term Support (LTS) release, will be supported until April 2021.
- ESP8266 RTOS SDK V2.0.x and earlier versions are already End of Life.
