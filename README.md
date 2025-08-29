# esp32-picture-frame

picture frame connected to an esp32

## hardware

* ESP32. I used a dev kit board from Amazon, but eventually I should probably make my own PCB for lower power.
* WaveShare 7.3 inch (F) EPD. This displays 7 colours: https://www.waveshare.com/7.3inch-e-paper-hat-f.htm
* I use a Ikea RODALM 5x7 frame, matted to 4x6.

## photos-server

this is mostly written by chatgpt.

## esp32-picture-frame

code running on the esp32. most of it is copied from waveshare's guide.

## what's next?

i'm thinking about a PCB design right now.

Also there is now e-ink Spectra 6 displays (e.g. [this one](https://www.waveshare.com/13.3inch-e-paper-hat-plus-e.htm))
and so of course I want to try it out. Unfortunately the driver PCB is different (60-pin FPC instead of 50-pin)
so I'd probably have to redesign some stuff :(
