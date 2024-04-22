APA102 to P9813 LED Conversion
==============================

This project translates APA102 clock and data signals to work for the P9813 chip.  The hardware set up for this uses the PixelBlaze LED driver and a Raspberry Pi Pico W to perform the translation.  The protocols are similar enough to make the translation relatively simple.  In order for the Pico to process at about 1MHz we use PIO programs to handle all of the I/O.  Additionally the program is divided to use both cores on the Pico.  One to handle the input and one to handle the output. The code borrows heavily from the PICO-SDK-EXAMPLES project.


- APA102 Datasheet - https://www.luxalight.eu/sites/default/files/downloads/2020-03/Datasheet_APA102.pdf
- P9813 Datasheet - https://www.alldatasheet.com/datasheet-pdf/pdf/1221780/ETC1/P9813.html


![Alt text](logic_screenshot.jpg?raw=true "Conversion")