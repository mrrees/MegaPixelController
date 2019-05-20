# MegaPixelController
Code for the Megapixel Controller using E1.31

Code: This code will allow a Teensy 3.2 Based chip function as a e1.31 controller. The base functionalitly will control up to 32 universes at speeds up to 40 FPS.  

Requirements: The Controller should use the octows2811 methods as explained on the PJRC webiste.  The controller also uses the Wiznet 5200  chip module for ethernet connectivity. The code requires the use of the one socket library located here:

https://github.com/mrrees/MegaPixel-One-Socket-Ethernet

Note w5500 users will need to modify the w5100 files in order for it to work correctly and utilize the full 16k buffer. 


