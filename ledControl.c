/*
 * Color RGB to/from HSV
 */

#include "Arduino.h"

#include "color.h"
const rgb dark = {0x00, 0x00, 0x00};
const rgb wakeColor = {0xAA, 0x55, 0x00};
volatile rgb outColor = {0x00, 0x00, 0xFF};

enum LEDMODE {
	stillMode,
	fadeMode,
	blinkMode,
	pulseMode
};

enum LEDMODE ledMode = stillMode;

struct Fading {
	hsv fromHSV;
	hsv currHSV;
	hsv toHSV;
	uint16_t fadeCntr;	// Counter used to end the fade transitions
	uint8_t inc;	// Hue increment per transition
	bool positiveIncrement;
	int16_t error;	// Bressenham algorithm error to enable up to 32768ms delays
	int16_t dh;	// hue differential and time differential
	int16_t dt;	// time differential is the amound of discrete time steps per fade transition
					// or the number of times that the LED will be refreshed for this transition
} fading;

struct Blinking {
	bool status; // blink status OFF or ON
	uint16_t period;
	uint32_t next;	// Next time to switch blink status. current timer + period/2
} blinking;

struct Pulsing {
	uint8_t min;
	uint8_t max;
	hsv currHSV;
	uint8_t increment;	// Brightness increments between LED refreshing cycles
	bool rampUp;
} pulsing;



void setColorRGB(const uint8_t r, const uint8_t g, const uint8_t b){
	outColor.r = r;
	outColor.g = g;
	outColor.b = b;
}

void setColor(const uint8_t color[3]){
	setColorRGB( color[0], color[1] , color[2] );
}

/*
 * Fade from current RGB color to RGB parameter, ms is the duration for the fade transition
 *
 */
void fadeTo(const uint8_t r, const uint8_t g, const uint8_t b, const uint16_t ms){
	ledMode = fadeMode;

	rgb toRGB = {r, g, b};
	// Transform current and next color to HSV
	fading.fromHSV = rgb2hsv(outColor);
	fading.currHSV = fading.fromHSV;
	fading.toHSV = rgb2hsv(toRGB);

	//printf("Fade from h = %d, s=%d, v= %d\n", wheelTo360(fading.currHSV.h), fading.currHSV.s, fading.currHSV.v);
	//printf("Fade to h = %d, s=%d, v= %d\n", wheelTo360(fading.toHSV.h), fading.toHSV.s, fading.toHSV.v);

	fading.dt = ms/LED_REFRESHING_PERIOD;
	fading.fadeCntr = fading.dt;
	//printf("Led Updates Per Period = %d\n", fading.fadeCntr);
	fading.error = 0;

	fading.dh = abs(fading.fromHSV.h - fading.toHSV.h);

	// Looking for the fastest route to reach the next Color
	// if the increment is smaller than 180, that meeans that the shortest route is within a full circle (0 to 360 hue degrees)
	// although if the increment is bigger than 180, that means that the shorter route will be over 360 or under 0 degrees
	if (fading.dh < WHEEL_180) {
		if (fading.fromHSV.h < fading.toHSV.h) {
			fading.positiveIncrement = true;
		} else {
			fading.positiveIncrement = false;
		}
	} else { // Hue increment per update period is bigger than 180
		if (fading.fromHSV.h < fading.toHSV.h) {
			fading.positiveIncrement = false;
		} else {
			fading.positiveIncrement = true;
		}
		// adjust increment for transitions that overflow the wheel
		fading.dh = WHEEL_360 - fading.dh;
	}
	fading.inc = fading.dh / fading.fadeCntr;

	//printf("Hue Diff = %d\n", wheelTo360(fading.dh));
	//printf("Hue Increment = %d\n", wheelTo360(fading.inc));
	//printf("Positive increment? %d\n", fading.positiveIncrement);
}

/*void fadeToColor(const Color c, uint8_t ms){}
void fadeToColorAndReturn(const Color c, uint8_t ms){}*/

void fadeUpdate(void) {
	// Output current color
	outColor = hsv2rgb(fading.currHSV);
	sendRgbColor( outColor);
	// Terminal bar
	/*for (int i = 0; i < fading.currHSV.h; ++i) {
		printf("#");
	}
	printf("\n");*/

	// Only fade if the number of led fade refreshes is bigger than 0!
	if (fading.fadeCntr--) {
		if (fading.positiveIncrement) {  // Positive increment moving clockwise along the Hue wheel
			// Detect and correct an color overflow hue value situation
			if ((fading.currHSV.h + fading.inc) >= WHEEL_360) {
				fading.currHSV.h = fading.currHSV.h + fading.inc - WHEEL_360;
			} else {
				fading.currHSV.h += fading.inc;  // Increment current Hue value
				// Discretization double to int correction
				// This solves casting and rounding issues using uint8_t.Based on Bressenham's algorithm
				if (!fading.inc) { // Increment equal to 0
					// Bressenham's algorithm, incremental scan conversion algorithm
				 	fading.error += 2*fading.dh;
				 	if (fading.error > fading.dt) {
				 		fading.currHSV.h++;
				 		fading.error-= 2* fading.dt;
				 	}
				// We look at the current value using rounded increment, and compared to the same equivalent increment from our
				// target hue value, if smaller increment by 1.
				 } else if (fading.currHSV.h < (fading.toHSV.h - fading.fadeCntr * fading.inc)){
					fading.currHSV.h++;
				}
			}
		} else { // Negative increment moving counterclockwise along the Hue wheel
			// Detect and correct negative overflows for hue values
			if ((fading.currHSV.h - fading.inc) <= 0) {
				fading.currHSV.h = fading.currHSV.h - fading.inc + WHEEL_360;
			} else {
				fading.currHSV.h -= fading.inc;
				// This solves casting and rounding issues using uint8_t.Based on Bressenham's algorithm
				if (!fading.inc) { // Increment equal to 0
					// Bressenham's algorithm
				 	fading.error += 2*fading.dh;
				 	if (fading.error > fading.dt)	{
				 		fading.currHSV.h--;
				 		fading.error-= 2* fading.dt;
				 	}
				} else if(fading.currHSV.h > ( (fading.toHSV.h + fading.fadeCntr * fading.inc) % WHEEL_360 ) ){
					fading.currHSV.h--;
				}
			}
		}
		//printf("Hue = %d, Saturation = %d, Value = %d \n", wheelTo360(fading.currHSV.h), fading.currHSV.s, fading.currHSV.v);
	} else {  // End of the fade to transition, return to send colors
		ledMode = stillMode;
		//printf("Fade ending :)\n");
	}
}

/*
 * This sets up a basic blink animation, ms is the blink period in ms
 */
void blink(const uint16_t ms){
	ledMode = blinkMode;
	blinking.status = false;
	blinking.period = ms;
	blinking.next = ms + timer;
}

void blinkUpdate(void) {
	if ((blinking.next-timer) > blinking.period) {
		if (blinking.status) { // On to Off
			//printf("OFF\n" );
			sendRgbColor( dark);
			blinking.status = false;
		} else {  // Off to On
			//printf("ON\n");
			sendRgbColor( outColor);
			blinking.status = true;
		}
		blinking.next += blinking.period;  // Updating the next blinking time incrementing it by the blinking period in ms
	}
}

/*
 * This sets up a pulse animation, pulsation works incrementing and decrementing the Value (hsv)
 * @ms; is the blink period in ms
 */
void pulse(const uint16_t ms){
	ledMode = pulseMode;
	// Initializing pulsing variables
	pulsing.currHSV = rgb2hsv(outColor);
	//printf("Pulse h = %d, s=%d, v= %d\n", pulsing.currHSV.h, pulsing.currHSV.s, pulsing.currHSV.v);
	// Start pulsing from current value
	pulsing.rampUp = false;
	pulsing.min = 0;
	pulsing.max = 255;

	uint16_t ledUpdatesPerPeriod = ms/LED_REFRESHING_PERIOD;  // This is how many times the LED will be refreshed by pulsing period
	//printf("ledUpdatesPerPeriod = %d\n", ledUpdatesPerPeriod);
	// Lets round up the ledUpdatePerPeriod if its a odd number
	// We do this to make sure that there will be always a led output cycle to reach the pulsing.max value
	// Note, we are rounding division, this means that the period will be a +- 32ms difference.
	if(ledUpdatesPerPeriod%2) {
		ledUpdatesPerPeriod++;
	}
	//printf("ledUpdatesPerPeriod = %d\n", ledUpdatesPerPeriod);
	// A period contains a ramp up cycle and a ramp down cycle. That is why we dived by two
	ledUpdatesPerPeriod >>= 1;
	//printf("ledUpdatesPerPeriod = %d\n", ledUpdatesPerPeriod);
	// TODO: fine a better place to conver
	pulsing.increment = (uint8_t)(pulsing.currHSV.v) / ledUpdatesPerPeriod;
	//printf("Increment = %d\n", pulsing.increment);

	// Check max and mins of increment to avoid weird situation
	if (pulsing.increment > pulsing.max) {
		pulsing.increment = pulsing.max;
	} else if (pulsing.increment < pulsing.min) {
		pulsing.increment = pulsing.min;
	}
}

void pulsingUpdate(void) {
	outColor = hsv2rgb(pulsing.currHSV);
	sendRgbColor( outColor);

	if (pulsing.rampUp) { // pulsing is ramping up
		if ((pulsing.currHSV.v + pulsing.increment) >= pulsing.max) {  // If HSV value next cycle will be bigger than the max value time to ramp down
			pulsing.rampUp = false;
			pulsing.currHSV.v = pulsing.max;  // Make sure we never overflow
		} else {
			pulsing.currHSV.v += pulsing.increment;  // Increase HSV value for the next light update cycle (ledOutputControl () )
		}
	} else { // pulsing is ramping down
		if ((pulsing.currHSV.v - pulsing.increment) <= pulsing.min) {  // are we under our minimum value?
			pulsing.rampUp = true;
			pulsing.currHSV.v = pulsing.min;
		} else {
			pulsing.currHSV.v -= pulsing.increment;
		}
	}
	//printf("Is ramping up? = %d\n", pulsing.rampUp);
}

/*
 * This controls the led output mode and its logic
 */
void updateLed(void) {
	switch(ledMode){
		case stillMode:
			sendRgbColor( outColor);
			break;
		case fadeMode:
			fadeUpdate();
			break;
		case blinkMode:
			blinkUpdate();
			break;
		case pulseMode:
			pulsingUpdate();
			break;
	}
}

