# m5stack-core2-pn532-felica

M5Stack Core2 + PN532でFelicaを読む

`lib`内に利用するPN532ライブラリをsubmoduleとして追加しているので、`--recursive`を付けてCloneする。

```bash
git clone https://github.com/Hiroya-W/m5stack-core2-pn532-felica.git --recursive
```

## PN532 library

[Seeed-Studio/PN532](https://github.com/Seeed-Studio/PN532)をベースに、PlatformIOのライブラリとして読める形に修正した[Hiroya-W/PN532](https://github.com/Hiroya-W/PN532)を利用している。
