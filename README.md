[![CI test](https://github.com/zevlg/tgs2png/workflows/CI/badge.svg)](https://github.com/zevlg/tgs2png/actions)
[![AUR version](https://img.shields.io/aur/version/tgs2png-git)](https://aur.archlinux.org/packages/tgs2png-git)

# tgs2png

Convert Telegram's animated stickers in TGS format into series of PNG
images.

Requires:
* librlottie `$ apt install librlottie-dev`
* libpng `$ apt install libpng-dev`

# Building

```console
$ mkdir build
$ cd build
$ cmake ..
$ make
# copy tgs2png somewhere into $PATH
```

# Running

NOTE: Telegram's TGS format is a gzipped rlottie file.

Get info about TGS file:
```console
$ gunzip -c sample.tgs | tgs2png -i -
```

Extract 10's frame:
```console
$ gunzip -c sample.tgs | tgs2png -o 10 -n 1 - > frame10.png
```
