// ---------------------------------------------------------------------------
// Calculate a ping median using the ping_timer() method.
// ---------------------------------------------------------------------------

// include the library code:
#include <LiquidCrystal.h>
#include <NewPing.h>  //Ultrasonic ranging module HC - SR04 provides 2cm - 400cm non-contact measurement function
#include <SimpleDHT.h> //for use with DHT11 humidity and temp sensor

int dhtpin = 2; // Defines pin number to which the sensor is connected
SimpleDHT11 dht11; // Creates a dht11 object

const int ITERATIONS = 3; // Number of iterations to evaluate in main loop.
const int TRIGGER_PIN = 5; // Arduino pin tied to trigger pin on ping sensor.
const int ECHO_PIN = 6; // Arduino pin tied to echo pin on ping sensor.
const int MAX_DISTANCE = 500; // Maximum distance (in cm) to ping.
const int PING_INTERVAL = 33; // Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo).

unsigned long pingTimer[ITERATIONS]; // Holds the times when the next ping should happen for each iteration.
unsigned int cm[ITERATIONS];         // Where the ping distances are stored.
uint8_t currentIteration = 0;        // Keeps track of iteration step.
volatile int median; //Create a volatile RAM variable "median" for lcd display

//initialize pump LED output
const int PUMP_PIN = 3;
const int ALARM_PIN = 4;
int pump_on;
int alarm_on;

// initialize the library with the numbers of the interface pins
//LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.


void setup() {

//define and initialize state of pump and alarm pins.
pinMode(PUMP_PIN, OUTPUT);
pinMode(ALARM_PIN, OUTPUT);
digitalWrite(PUMP_PIN, LOW); // turn the PUMP_PIN off by making the voltage LOW
digitalWrite(ALARM_PIN, LOW); // turn the PUMP_PIN off by making the voltage LOW
pump_on = 0;
alarm_on = 0;

// set up the LCD's number of columns and rows:
  lcd.begin(16, 2); // Initializes the interface to the LCD screen
    // Print an initial message to the LCD.
    lcd.setCursor(0,0); // Sets the location at which subsequent text written to the LCD will be displayed
    lcd.print("Distance =    cm"); // set the initial Display on the first line
    lcd.setCursor(0,1);            // set the cursor to column 0, line 1
    lcd.print("T/H = ");           // set the initial display on the second line
     
 // Serial.begin(9600); // Only needed if serial monitor is used to display results.
 
  pingTimer[0] = millis() + 75;            // First ping starts at 75ms, gives time for the Arduino to chill before starting.
  for (uint8_t i = 1; i < ITERATIONS; i++) // Set the starting time for each iteration.
    pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;
}

  
void loop() {
 
  for (uint8_t i = 0; i < ITERATIONS; i++) { // Loop through all the iterations.
    if (millis() >= pingTimer[i]) {          // Is it this iteration's time to ping?
      pingTimer[i] += PING_INTERVAL * ITERATIONS; // Set next time this sensor will be pinged.
      if (i == 0 && currentIteration == ITERATIONS - 1) oneSensorCycle(); // Sensor ping cycle complete, do something with the results.
      sonar.timer_stop();          // Make sure previous timer is canceled before starting a new ping (insurance).
      currentIteration = i;        // Sensor being accessed.
      cm[currentIteration] = 0;    // Make distance zero in case there's no ping echo for this iteration.
      sonar.ping_timer(echoCheck); // Do the ping (processing continues, interrupt will call echoCheck to look for echo).
    }
  }
  // Other code that *DOESN'T* analyze ping results can go here.

// Ultrasonic measurement actions to display distance and turn on or off sump pump LED.
  int dist = median * 0.394; //Convert cm to inches  
   // (note: lcd line 1 is the second row, since counting begins with 0):
   int min_dist = 3;  //minimum dist to display which helps steady displayed distance
   if (dist >= min_dist){ 
  //clear the last number
  lcd.setCursor(11, 0);
  lcd.print("   in");
    
  // print the distance
  //Right justify the distance measurement for readability.
  if (dist > 99) lcd.setCursor(11, 0);
  if (dist >9 && dist < 100) {
    lcd.setCursor(11, 0);
    lcd.print(' ');  //use single quote for one character, double quotes for more than one
    lcd.setCursor(12, 0);
     }
    if (dist < 10 && dist >= min_dist) {
    lcd.setCursor(11, 0);
    lcd.print("  ");
    lcd.setCursor(13, 0);
     }
  lcd.print(dist); //finally print the distance after evaluating the number of digits to justify dist right, adjacent to units displayed. 
  
  // Place other code here to actuate sump pump on, off and alarm levels and delay 100 when pump or alarm state changes to allow LED to display visible change in state.
  // If dist read jumps around too much, turning pump on off repeatedly, evaluate action based on previous two reads within a range of about 3 inches. 
  // For hysteresis action, set pump_on variable to 1 when turned on and 0 when turned off. 
  // Test with LED. Level 1, 2, 3, 4. 1=pump off, 2=pump on, 3=alarm off, 4=alarm on.
  int pump_on_level = 50 - 20; //Assume crock is 50 inches deep. The lower the distance the higher the water level in the crock. 
  int pump_off_level = 50 - 15;
  int alarm_on_level = 50 - 35;
  int alarm_off_level = 50 - 30; 
  
// Determine if pump should be turned on.
//If pump was previously off, then turn on if level is high enough.
  if (dist < pump_on_level) {
     digitalWrite(PUMP_PIN, HIGH); 
     pump_on = 1;
  }
  if (dist > pump_off_level) {
    pump_on = 0;
    digitalWrite(PUMP_PIN, LOW);    // turn the LED off by making the voltage LOW
  }
  if (dist < alarm_on_level) {
    alarm_on = 1;
    digitalWrite(ALARM_PIN, HIGH); 
  }
  if (dist > alarm_off_level) {
    alarm_on = 0;
    digitalWrite(ALARM_PIN, LOW);    // turn the ALARM off by making the voltage LOW
  }
     
  //delay(50); 
    } //End of dist > min_dist loop
  
  delay(1); //Allow LED to display intermittent triggers

//Read and Display temperature and humidity
// read without samples.
  byte temperature = 0;
  byte humidity = 0;
  if (dht11.read(dhtpin, &temperature, &humidity, NULL)) {
    //Serial.print("Read DHT11 failed.");
    return;
  }
   
   lcd.setCursor(6, 1);
   lcd.print((int)(temperature * 9/5+32)); // Convert to deg F
   lcd.print('F');
   lcd.print(" / "); //use single quote for one character, double quotes for more than one
   lcd.print((int)humidity);
   lcd.print('%');
   // DHT11 sampling rate is 1HZ.
  //delay(250);

} //END OF MAIN LOOP


void echoCheck() { // If ping received, set the sensor distance to array.
  if (sonar.check_timer())
    cm[currentIteration] = sonar.ping_result / US_ROUNDTRIP_CM;
}

void oneSensorCycle() { // All iterations complete, calculate the median.
  unsigned int uS[ITERATIONS];
  uint8_t j, it = ITERATIONS;
  uS[0] = NO_ECHO;
  for (uint8_t i = 0; i < it; i++) { // Loop through iteration results.
    if (cm[i] != NO_ECHO) { // Ping in range, include as part of median.
      if (i > 0) {          // Don't start sort till second ping.
        for (j = i; j > 0 && uS[j - 1] < cm[i]; j--) // Insertion sort loop.
          uS[j] = uS[j - 1];                         // Shift ping array to correct position for sort insertion.
      } else j = 0;         // First ping is sort starting point.
      uS[j] = cm[i];        // Add last ping to array in sorted position.
    } else it--;            // Ping out of range, skip and don't include as part of median.
  } //End of loop thru iteration results.
  median = uS[it >> 1]; //store median result for use in main loop.
}  // End of calculate the median loop.
 
