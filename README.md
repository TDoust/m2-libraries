## Macchina M2 Libraries

Here is where we will be keeping all of our M2 code. We encourage you to make suggestions, make pull requests, and generally get involved! Thanks for visiting.

#### Install Libraries

To get up and rolling quickly, do this:

1. Click "Clone or download" and then "Download ZIP"
2. Move each folder to your Arduino Libraries
3. Start (or Resart) your Arduino IDE and you will see several examples for each library ready to try out.
4. Make sure to follow these instructions before uploading to M2: http://macchina.cc/m2/getting-started


#### Summary:

Each of these Libraries adds functionality to M2 as summarized here:

 * `M2_Due_SD_HSCMI` Gives access to HSMCI (High Speed MultiMedia Card Interface) on M2. `SD_HSMCI` is support code that will also need to be in place.

 * `LIN` LIN bus functionality for M2 on 2 channels

 * `SW_CAN` Single Wire CAN (GMLAN) give M2 signle-wire CAN functionality. Uses external MCP2515 transceiver connected to SAM3X via SPI. Library currently includes an example of sendind SW CAN messege. 


##### For CAN functionality, use the excellent library:

https://github.com/collin80/due_can

##### Misc examples and test code that needs some organizing found on in GIST:

https://gist.github.com/macchina
