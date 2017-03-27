# Zoe-Lamp
Zoe-trope lamp with spinning shade and synchronized light

Currently running on an unmodified Arduino. 

##PINS

Pin|Direction|Connection
-|-|-
2,3|Out|Each connected to one of the LED transistors. Driving HIGH will light the associated 3 LED modules,
8|Out|Ground for the US1881. Set LOW.
9|In|Connected to the OUT of the US1881 hall effect latch. Switches between high and low on each half turn.
10|Out|Vcc for the US1881. Set HIGH.


 