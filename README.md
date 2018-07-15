THIS REPO IS OBSOLITE! 

PLEASE GOTO...

https://github.com/bigjosh/Move38-Arduino-Platform


.. FOR THE REPO THAT WORKS WITH THE KICKSTARTER VERSIONS OF THE BLINKS TILES!



# Move32 blinks lib

Arduino library for writing games to run on the Move32 Automatiles platform.

##Overview 
This library provides a high level API designed to handle many of the command tasks needed to write games. It is built on top of the firmware interface, which has more flexibility but requires working at a lower level.     

## API



**isAlone**
```c
boolean isAlone();
// returns true when the tile has no neighbors
// same as checking all 6 sides and seeing that they return 0
```

###Blink display manager methods###
These functions should not be used in `loop` since they will handle animation on their own.
Use them in a callback, for example, when the button is pressed, `fadeToAndReturn(255, 0, 0, 500);` will turn red over the course of half a second and then return to its previous color.

**fadeTo**
```c
void fadeTo(int r, int g, int b, int ms);  // timed change to color

void fadeToAndReturn(int r, int g, int b, int ms);  // timed change to color and back
```

**blink**
```c
void blink(int ms); // defaults to on/off of current color

void blink(int ms, int min, int max); // TODO: low and high levels for blinking and the time between them

void blink(int ms, int[n][3] c); // TODO: (low priority) send array of colors to blink between
```

**pulse**
```c
void pulse(int ms); // phase

void pulse(int ms, int min, int max); // TODO: phase w/ low and high brightness

void pulse(int ms, int[n][3] c); // TODO: phased pulse between colors (depends on fadeTo)

```

###Color Management



###Button
```c
void setButtonClickThreshold(int ms); // defaulted to 200ms, but function available to make slower or faster clicking part of the game

void buttonClicked();
void buttonDoubleClicked();
void buttonTripleClicked();

void buttonLongPressed();
```

##Examples###

The following examples are written using the AutomaTiles API for the Arduino IDE.

**blink**
An example to show how to blink with a duration `blink(duration)`

**pulse**
An example to show how to pulse with a duration `pulse(duration)`

**fadeTo**
An example to show how colors can transition smoothly between two colors `fadeTo(r, g, b, duration)` **-WIP-**

**getNeighbor**

**isAlone**

**setColor**

**setColorRGB**
