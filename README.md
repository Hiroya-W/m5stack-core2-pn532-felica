# あくあたんFelica SwitchBot Lock Controller

```
Felica  ))))  PN532 - M5Stack ))))  apiserver (authentication + invoke switchbot api)
```



## M5Stack Core2 + PN532でFelicaを読む

`lib`内に利用するPN532ライブラリをsubmoduleとして追加しているので、`--recursive`を付けてCloneする。

```bash
git clone https://github.com/Hiroya-W/m5stack-core2-pn532-felica.git --recursive
```

## PN532 library

[Seeed-Studio/PN532](https://github.com/Seeed-Studio/PN532)をベースに、PlatformIOのライブラリとして読める形に修正した[Hiroya-W/PN532](https://github.com/Hiroya-W/PN532)を利用している。

## SwitchBot Lock BLE解析

custom service
CBA20D00-224d-11e6-9fb8-0002a5d5c51b

adv data
length = 12, bytes = 6909
6909 
c3 a7 27 60 88 6d 

c3 a7 27 60 88 6d dc 8b 08 20 lock
c3 a7 27 60 88 6d dd 9b 08 20 unlock
c3 a7 27 60 88 6d de 8b 08 20 lock
c3 a7 27 60 88 6d e4 9b 08 20 unlock
                     --
bit 7: calibration status 
 1:calibrated
 0:not calibrated
bit 6-4: lock status
 000 locked
 001 unlocked
bit 3: door status update source 0
bit 2: door status 0 closed
bit 1-0: reserve

