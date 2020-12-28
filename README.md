# epaper
This is a library driven by 52Pi E-ink, which can drive 2.13 inch electronic paper and other sizes of electronic paper.
* Currently, only 2.13 inch screen support is provided.
## How to use it
* Snap the 2.13 inch epaper hat board into the Raspberry Pi GPIO
* OS Requirement: Raspberry Pi OS 
* Enable `SPI` interface via `sudo raspi-config` tool.
* Install `Pillow`, `spidev`, `RPi.GPIO` libraries.
* Download demo code by:
```bash
cd ~
git clone https://github.com/geeekpi/epaper.git
cd epaper/
python3 eink2.13_demo.py
```
And the display will flash `red`, `black`, `white` and finally a picture.
