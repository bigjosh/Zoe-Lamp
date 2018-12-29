# Zoe-Lamp
Zoe-trope lamp with spinning shade and synchronized light

Currently running on an unmodified Arduino. 

## PINS

| Pin | Direction | Connection |
| - | - | - |
| 2 | Out | Connected to gate of IRLZ34N transistor (thanks [Robb!](http://robb.cc)) that turns LEDs on. Driving HIGH will light the associated LED strips. |
| 10 | Out |Vcc for the A3141 hall effect sensor. Set HIGH. |
| 9 | Out | Ground for A3141. Set LOW.  |
| 8 | In | Connected to the OUT of the A3141 hall effect sensor. Switches high when magnet passes over. |


 ## TODO
 
 
1)	Content. People who have seen the lamp at my house really like the top dance clip. Small clips look much better since higher flash rates at low spin speeds. Maybe a stack of really nice dance clips? It would also be great to loop them so no jarring jump at the beginning/end.  
2)	Transitions. It actually looks really cool to transition from one FPS to the next live while spinning because one starts to spin faster and faster and blurs away while the other slows down until it locks in.  I think this would look especially cool with dance clips, maybe switching every 5 seconds or so. 
3)	Top plate. I’d like to add a top plate to hold the sensor and also be a connection point for the LEDs and probably even the PCB for the electronics. This would mean bring the stem all the way up to almost the bottom of the shade. This will also protect the sensor from getting bonked and limit shade wobble. We could even drop 3 bearings in there, we’d have to try to see how that felt. 
4)	Wire channels. We need a way to get power from the top plate down and out the bottom. Can the stem be hallow? Maybe a square extrusion?
5)	Pin. We need to make that pin protrude as little as possible to avoid impaling people. Are the dart tips special? If not then maybe something with a flat bottom so we can attach it to the top plate? If yes, then maybe we can mount it from below though a hole in the top plate?
6)	UI. How do people turn the lamp on and off for normal operation?  I kinda like having an old school lamp switch on the cord. Turn it on and it just…. Well, turns on. The magic happens when you spin and it seamlessly transitions from steady on to animation at the same brightness level until the speed drops, and then it seamlessly goes back to normal lamp. 
