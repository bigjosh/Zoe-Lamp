# Zoe-Lamp
Zoe-trope lamp with spinning shade and synchronized light

Currently running on an unmodified Arduino. 

##PINS

Pin|Direction|Connection
- | - | -
2 | Out | Connected to gate of IRLZ34N transistor (thanks [Robb!](http://robb.cc)) that turns LEDs on. Driving HIGH will light the associated LED strips,
10 | Out |Vcc for the A3141 hall effect sensor. Set HIGH.
9 | Out | Ground for A3141. Set LOW. 
8 | In | Connected to the OUT of the A3141 hall effect sensor. Switches high when magnet passes over. 


 