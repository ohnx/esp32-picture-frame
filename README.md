# esp32-picture-frame

picture frame connected to an esp32

## hardware

* ESP32. I used a dev kit board from Amazon, but eventually I should probably make my own PCB for lower power.
* WaveShare 7.3 inch (F) EPD. This displays 7 colours: https://www.waveshare.com/7.3inch-e-paper-hat-f.htm
* I use a Ikea RODALM 5x7 frame, matted to 4x6.

## photos-server

this is mostly written by chatgpt.

## esp32-picture-frame

code running on the esp32. most of it is copied from waveshare's guide

TODO: make it work on the whole image. right now due to ram constraints i only
make it work on part of it. rip. it shouldn't be hard to stream, but also it's
late and i should sleep

## what's next?

Well there is now e-ink Spectra 6 displays (e.g. [this one](https://www.waveshare.com/13.3inch-e-paper-hat-plus-e.htm))
and so of course I want to try it out.
