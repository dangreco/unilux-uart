# Changelog

## [0.1.0](https://github.com/dangreco/unilux-uart/compare/v0.0.1...v0.1.0) (2026-06-25)


### Features

* add frame debugging ([#13](https://github.com/dangreco/unilux-uart/issues/13)) ([8313c16](https://github.com/dangreco/unilux-uart/commit/8313c165099606c1afb6e1a739b0edcb5945c7a9))
* add initial ESPHome implementation ([#7](https://github.com/dangreco/unilux-uart/issues/7)) ([26107ef](https://github.com/dangreco/unilux-uart/commit/26107ef8aa36816b7f5b24eb9a2ccf2926a2577f))
* add settings ([ae8e033](https://github.com/dangreco/unilux-uart/commit/ae8e03305fa5509ead7538d81b0976a09f7dc65f))
* fan speed message (0x22) + climate fan modes ([00d94c0](https://github.com/dangreco/unilux-uart/commit/00d94c0abc97a84090c8c50fc50940e69bcffcc3))
* HVAC mode message (0x5C) + two-way climate mode control ([d460eee](https://github.com/dangreco/unilux-uart/commit/d460eee6eb4b62bfc9b1ae7725186aab17d0c2bd))
* power message (0x21) + climate OFF mode ([8c67d51](https://github.com/dangreco/unilux-uart/commit/8c67d51b79357105ae29e673ff540007b948b35f))
* refactor AUP implementation to encoder + decoder, add tests ([#4](https://github.com/dangreco/unilux-uart/issues/4)) ([7252b7c](https://github.com/dangreco/unilux-uart/commit/7252b7cf06887f34ac5941ffc04c7f67afeb34c8))
* sync ([c8f3b27](https://github.com/dangreco/unilux-uart/commit/c8f3b277e7ff77010154bc5e8df21d1621a0970f))
* target temperature message + two-way climate ([fb81eae](https://github.com/dangreco/unilux-uart/commit/fb81eaeb5545beba655fd847c490e7b6fd67e556))
* temperature parsing + logging ([#14](https://github.com/dangreco/unilux-uart/issues/14)) ([65f0e54](https://github.com/dangreco/unilux-uart/commit/65f0e549723a3f2e6f9792cbabd749474b03f141))
* temperature sensors ([#15](https://github.com/dangreco/unilux-uart/issues/15)) ([0d18c8c](https://github.com/dangreco/unilux-uart/commit/0d18c8ccbc3b84e8588b16d2f8aadb525533eb16))
* widen climate temperature range to 5-40 °C ([9cd1c91](https://github.com/dangreco/unilux-uart/commit/9cd1c912e4fe1ce46bdfdf8dc84822137cd960a5))
* wire library into esphome component ([#11](https://github.com/dangreco/unilux-uart/issues/11)) ([3f0ccc4](https://github.com/dangreco/unilux-uart/commit/3f0ccc46dc828c780177623c57c5b0ccbba396ed))
* WMMM encoder + decoder, tests ([#6](https://github.com/dangreco/unilux-uart/issues/6)) ([d482b67](https://github.com/dangreco/unilux-uart/commit/d482b67eaf582e2cdd6893bac39781444353911e))


### Bug Fixes

* fix esphome config, log ([#8](https://github.com/dangreco/unilux-uart/issues/8)) ([7b3434d](https://github.com/dangreco/unilux-uart/commit/7b3434d26b21fb5cf77ecb98fa2be22ae3e4d169))
* make target-temperature climate adjustable and on by default ([215ffb2](https://github.com/dangreco/unilux-uart/commit/215ffb2db1f8fc1b2d88d305e67cff8716af15d1))
* move logging to dump_config ([#10](https://github.com/dangreco/unilux-uart/issues/10)) ([99337cf](https://github.com/dangreco/unilux-uart/commit/99337cf64147f0f173bd2fe8f7246143bb74747f))
* remove unused sensor inheritance ([#9](https://github.com/dangreco/unilux-uart/issues/9)) ([9c1a813](https://github.com/dangreco/unilux-uart/commit/9c1a813701de3a5d8a12edd339f133d8e30fe48c))

## 0.0.1 (2026-06-20)


### Miscellaneous Chores

* cut 0.0.1 to test the release pipeline ([#2](https://github.com/dangreco/unilux-uart/issues/2)) ([af5edd9](https://github.com/dangreco/unilux-uart/commit/af5edd96759208bb00e2657eea2e5fc264032f5e))
