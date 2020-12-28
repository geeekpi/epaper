import time
from Epaper import *
from PIL import Image #Pillow

# Demo Configuration
X_PIXEL = 128
Y_PIXEL = 250
RED_CH = True # If the module has only two colors(B&W), please set it to False.

if RED_CH is True:
    print("Flash Red")
    e = Epaper(X_PIXEL,Y_PIXEL)
    e.flash_red(on=True)
    e.flash_black(on=False)
    e.update()

    time.sleep(1)

print("Flash Black")
e = Epaper(X_PIXEL,Y_PIXEL)
if RED_CH is True:
    e.flash_red(on=False)
e.flash_black(on=True)
e.update()

time.sleep(1)

print("Flash White")
e = Epaper(X_PIXEL,Y_PIXEL)
if RED_CH is True:
    e.flash_red(on=False)
e.flash_black(on=False)
e.update()

time.sleep(1)

print("Flash Image")
f = Image.open('demo.png')
f = f.convert('RGB') # conversion to RGB
data = f.load()

rBuf = [0] * 4000
bBuf = [0] * 4000

for y in range(250):
    for x in range(128):
       # Red CH
       if data[x,y] == (237,28,36) and RED_CH is True:
           # This algorithm has bugs if ported according to C, the solution is referred to:https://www.taterli.com/7450/
           index = int(16 * y + (15 - (x - 7) / 8))
           tmp = rBuf[index]
           rBuf[index] = tmp | (1 << (x % 8))
       # Black CH
       elif data[x,y] == (255,255,255):
            index = int(16 * y + (15 - (x - 7) / 8))
            tmp = bBuf[index]
            bBuf[index] = tmp | (1 << (x % 8))

e = Epaper(X_PIXEL,Y_PIXEL)
if RED_CH is True:
    e.flash_red(buf=rBuf)
e.flash_black(buf=bBuf)
e.update()