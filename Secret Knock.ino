/* Detects patterns of knocks and triggers a motor to unlock
   it if the pattern is correct.
   
   By Steve Hoefer http://grathio.com
   Version 0.1.10.20.10
   Licensed under Creative Commons Attribution-Noncommercial-Share Alike 3.0
   http://creativecommons.org/licenses/by-nc-sa/3.0/us/
   (In short: Do what you want, just be sure to include this line and the four above it, and don't sell it or use it in anything you sell without contacting me.)
   
   Analog Pin 0: Piezo speaker (connected to ground with 1M pulldown resistor)
   Digital Pin 2: Switch to enter a new code.  Short this to enter programming mode.
   Digital Pin 3: DC gear reduction motor attached to the lock. (Or a motor controller or a solenoid or other unlocking mechanisim.)
   Digital Pin 4: Red LED. 
   Digital Pin 5: Green LED. 
   
   Update: Nov 09 09: Fixed red/green LED error in the comments. Code is unchanged. 
   Update: Nov 20 09: Updated handling of programming button to make it more intuitive, give better feedback.
   Update: Jan 20 10: Removed the "pinMode(knockSensor, OUTPUT);" line since it makes no sense and doesn't do anything.
 */

 #include <LiquidCrystal.h> //Dołączenie bilbioteki
LiquidCrystal lcd(6, 7, 8, 9, 10, 11); //Informacja o podłączeniu nowego wyświetlacza

 
// Pin definitions
const int knockSensor = 0;         // Piezo sensor on pin 0.
const int programSwitch = 2;       // If this is high we program a new code.
const int lockMotor = 3;           // Gear motor used to turn the lock.
const int redLED = 4;              // Status LED
const int greenLED = 5;            // Status LED
 
// Tuning constants.  Could be made vars and hoooked to potentiometers for soft configuration, etc.
const int threshold = 2;           // Minimum signal from the piezo to register as a knock
const int rejectValue = 25;        // If an individual knock is off by this percentage of a knock we don't unlock..
const int averageRejectValue = 15; // If the average timing of the knocks is off by this percent we don't unlock.
const int knockFadeTime = 150;     // milliseconds we allow a knock to fade before we listen for another one. (Debounce timer.)
const int lockTurnTime = 1000;      // milliseconds that we run the motor to get it to go a half turn.

const int maximumKnocks = 20;       // Maximum number of knocks to listen for.
const int knockComplete = 1200;     // Longest time to wait for a knock before we assume that it's finished.


// Variables.
int secretCode[maximumKnocks] = {50, 25, 25, 50, 100, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // Initial setup: "Shave and a Hair Cut, two bits."
int knockReadings[maximumKnocks];   // When someone knocks this array fills with delays between knocks.
int knockSensorValue = 0;           // Last reading of the knock sensor.
int programButtonPressed = false;   // Flag so we remember the programming button setting at the end of the cycle.

void setup() 
{
  lcd.begin(16, 2); //Deklaracja typu
  lcd.setCursor(0,0); //Ustawienie kursora
  
  pinMode(lockMotor, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(programSwitch, INPUT);
  
  Serial.begin(9600);               			// Uncomment the Serial.bla lines for debugging.
  Serial.println("Program start.");  			// but feel free to comment them out after it's working right.
  
  digitalWrite(greenLED, HIGH);      // Green LED on, everything is go.
}

void loop() {
  // Listen for any knock at all.
  knockSensorValue = analogRead(knockSensor);
  
  if (digitalRead(programSwitch)==HIGH){  // is the program button pressed?
    programButtonPressed = true;          // Yes, so lets save that state
    digitalWrite(redLED, HIGH);           // and turn on the red light too so we know we're programming.
  } else {
    programButtonPressed = false;
    digitalWrite(redLED, LOW);
  }
  
  if (knockSensorValue >=threshold){
    listenToSecretKnock();
  }
} 



 
// Records the timing of knocks.
void listenToSecretKnock(){
  
  //Serial.println("knock starting"); 
  lcd.clear();
  lcd.begin(16, 2); //Deklaracja typu
  lcd.setCursor(1,0); //Ustawienie kursora
  
  lcd.print("Knock starting! ");
 
    
  int i = 0;
  // First lets reset the listening array.
  for (i=0;i<maximumKnocks;i++){
    knockReadings[i]=0;
    
  }
  
  

  
  int currentKnockNumber=0;         			// Incrementer for the array.
  int startTime=millis();           			// Reference for when this knock started.
  int now=millis();
  
  digitalWrite(greenLED, LOW);      			// we blink the LED for a bit as a visual indicator of the knock.
  if (programButtonPressed==true){
     digitalWrite(redLED, LOW);                         // and the red one too if we're programming a new knock.
  }
  delay(knockFadeTime);                       	        // wait for this peak to fade before we listen to the next one.
  digitalWrite(greenLED, HIGH);  
  if (programButtonPressed==true){
     digitalWrite(redLED, HIGH);                        
  }
  do {
    //listen for the next knock or wait for it to timeout. 
    knockSensorValue = analogRead(knockSensor);
    if (knockSensorValue >=threshold){                   //got another knock...
      //record the delay time.
     
      Serial.println("*knock*");
      lcd.clear();
      lcd.begin(16, 2); //Deklaracja typu
      lcd.setCursor(4,1); //Ustawienie kursora
      lcd.print("*knock!* ");
      

     
     
      now=millis();
      knockReadings[currentKnockNumber] = now-startTime;
      currentKnockNumber ++;                             //increment the counter
      startTime=now;          
      // and reset our timer for the next knock
      digitalWrite(greenLED, LOW);  
      if (programButtonPressed==true){
        digitalWrite(redLED, LOW);                       // and the red one too if we're programming a new knock.
      }
      delay(knockFadeTime);                              // again, a little delay to let the knock decay.
      digitalWrite(greenLED, HIGH);
      if (programButtonPressed==true){
        digitalWrite(redLED, HIGH);                         
      }
      lcd.clear();
    }

    now=millis();
    
    //did we timeout or run out of knocks?
  } while ((now-startTime < knockComplete) && (currentKnockNumber < maximumKnocks));
  
  //we've got our knock recorded, lets see if it's valid
  if (programButtonPressed==false){             // only if we're not in progrmaing mode.
    if (validateKnock() == true){
      triggerDoorUnlock(); 
    } else {
      
      Serial.println("Secret knock failed.");
      lcd.clear();
      locker();
      //lcd.begin(16, 2);
      lcd.setCursor(4, 0);
      lcd.print("Secret knock");
      lcd.setCursor(4, 1);
      lcd.print("failed!!");
      delay(2000);
      lcd.clear();

      
      digitalWrite(greenLED, LOW);  		// We didn't unlock, so blink the red LED as visual feedback.
      for (i=0;i<4;i++){					
        digitalWrite(redLED, HIGH);
        delay(100);
        digitalWrite(redLED, LOW);
        delay(100);
      }
      digitalWrite(greenLED, HIGH);
    }
  } else { // if we're in programming mode we still validate the lock, we just don't do anything with the lock
    validateKnock();
    // and we blink the green and red alternately to show that program is complete.
    Serial.println("New lock stored.");
    lcd.clear();
    lcd.begin(16, 2);
    lcd.setCursor(2, 0);
    lcd.print("New lock");
    lcd.setCursor(2, 1);
    lcd.print("stored :)");
   // delay(700);
    //lcd.clear();
          
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);
    for (i=0;i<3;i++){
      delay(100);
      digitalWrite(redLED, HIGH);
      digitalWrite(greenLED, LOW);
      delay(100);
      digitalWrite(redLED, LOW);
      digitalWrite(greenLED, HIGH);      
    }
  }
}


// Runs the motor (or whatever) to unlock the door.
void triggerDoorUnlock(){
  lcd.clear();
  thumbsup();
  
  lcd.setCursor(7, 0);
  lcd.print("Door");
  lcd.setCursor(5, 1);
  lcd.print("unlocked!!");

  Serial.println("Door unlocked!");
  
  int i=0;
  
  // turn the motor on for a bit.
  digitalWrite(lockMotor, HIGH);
  digitalWrite(greenLED, HIGH);            // And the green LED too.
  
  delay (lockTurnTime);                    // Wait a bit.
  
  digitalWrite(lockMotor, LOW);            // Turn the motor off.
  
  // Blink the green LED a few times for more visual feedback.
  for (i=0; i < 5; i++){   
      digitalWrite(greenLED, LOW);
      delay(100);
      digitalWrite(greenLED, HIGH);
      delay(100);
  }
   lcd.clear();
}

void thumbsup() {
 byte thumb1[8] = {B00100,B00011,B00100,B00011,B00100,B00011,B00010,B00001};
 byte thumb2[8] = {B00000,B00000,B00000,B00000,B00000,B00000,B00000,B00011};
 byte thumb3[8] = {B00000,B00000,B00000,B00000,B00000,B00000,B00001,B11110};
 byte thumb4[8] = {B00000,B01100,B10010,B10010,B10001,B01000,B11110,B00000};
 byte thumb5[8] = {B00010,B00010,B00010,B00010,B00010,B01110,B10000,B00000};
 byte thumb6[8] = {B00000,B00000,B00000,B00000,B00000,B10000,B01000,B00110};
 lcd.createChar(0, thumb1);
 lcd.createChar(1, thumb2);
 lcd.createChar(2, thumb3);
 lcd.createChar(3, thumb4);
 lcd.createChar(4, thumb5);
 lcd.createChar(5, thumb6);
 
 
 lcd.setCursor(0,1);
 lcd.write((byte)0);
 lcd.setCursor(0,0);
 lcd.write(1);
 lcd.setCursor(1,1);
 lcd.write(2);
 lcd.setCursor(1,0);
 lcd.write(3);
 lcd.setCursor(2,1);
 lcd.write(4);
 lcd.setCursor(2,0);
 lcd.write(5);
}

void locker() {
 byte thumb11[8] = {B00000,B00000,B00000,B00001,B00001,B00001,B00001,B00011};
 byte thumb22[8] = {B01110,B10001,B00000,B00000,B00000,B00000,B00000,B11111};
 byte thumb33[8] = {B00000,B00000,B00000,B10000,B10000,B10000,B10000,B11000};
 byte thumb44[8] = {B00011,B00011,B00011,B00011,B00011,B00011,B00011,B00000};
 byte thumb55[8] = {B11111,B11111,B11011,B11011,B11011,B11111,B11111,B00000};
 byte thumb66[8] = {B11000,B11000,B11000,B11000,B11000,B11000,B11000,B00000};
 lcd.createChar(0, thumb11);
 lcd.createChar(1, thumb22);
 lcd.createChar(2, thumb33);
 lcd.createChar(3, thumb44);
 lcd.createChar(4, thumb55);
 lcd.createChar(5, thumb66);
 
 
 lcd.setCursor(0,0);
 lcd.write((byte)0);
 lcd.setCursor(1,0);
 lcd.write(1);
 lcd.setCursor(2,0);
 lcd.write(2);
 lcd.setCursor(0,1);
 lcd.write(3);
 lcd.setCursor(1,1);//ok
 lcd.write(4);
 lcd.setCursor(2,1);
 lcd.write(5);
}

// Sees if our knock matches the secret.
// returns true if it's a good knock, false if it's not.
// todo: break it into smaller functions for readability.
boolean validateKnock(){
  int i=0;
 
  // simplest check first: Did we get the right number of knocks?
  int currentKnockCount = 0;
  int secretKnockCount = 0;
  int maxKnockInterval = 0;          			// We use this later to normalize the times.
  
  for (i=0;i<maximumKnocks;i++){
    if (knockReadings[i] > 0){
      currentKnockCount++;
    }
    if (secretCode[i] > 0){  					//todo: precalculate this.
      secretKnockCount++;
    }
    
    if (knockReadings[i] > maxKnockInterval){ 	// collect normalization data while we're looping.
      maxKnockInterval = knockReadings[i];
    }
  }
  
  // If we're recording a new knock, save the info and get out of here.
  if (programButtonPressed==true){
      for (i=0;i<maximumKnocks;i++){ // normalize the times
        secretCode[i]= map(knockReadings[i],0, maxKnockInterval, 0, 100); 
      }
      // And flash the lights in the recorded pattern to let us know it's been programmed.
      digitalWrite(greenLED, LOW);
      digitalWrite(redLED, LOW);
      delay(1000);
      digitalWrite(greenLED, HIGH);
      digitalWrite(redLED, HIGH);
      delay(50);
      for (i = 0; i < maximumKnocks ; i++){
        digitalWrite(greenLED, LOW);
        digitalWrite(redLED, LOW);  
        // only turn it on if there's a delay
        if (secretCode[i] > 0){                                   
          delay( map(secretCode[i],0, 100, 0, maxKnockInterval)); // Expand the time back out to what it was.  Roughly. 
          digitalWrite(greenLED, HIGH);
          digitalWrite(redLED, HIGH);
        }
        delay(50);
      }
	  return false; 	// We don't unlock the door when we are recording a new knock.
  }
  
  if (currentKnockCount != secretKnockCount){
    return false; 
  }
  
  /*  Now we compare the relative intervals of our knocks, not the absolute time between them.
      (ie: if you do the same pattern slow or fast it should still open the door.)
      This makes it less picky, which while making it less secure can also make it
      less of a pain to use if you're tempo is a little slow or fast. 
  */
  int totaltimeDifferences=0;
  int timeDiff=0;
  for (i=0;i<maximumKnocks;i++){ // Normalize the times
    knockReadings[i]= map(knockReadings[i],0, maxKnockInterval, 0, 100);      
    timeDiff = abs(knockReadings[i]-secretCode[i]);
    if (timeDiff > rejectValue){ // Individual value too far out of whack
      return false;
    }
    totaltimeDifferences += timeDiff;
  }
  // It can also fail if the whole thing is too inaccurate.
  if (totaltimeDifferences/secretKnockCount>averageRejectValue){
    return false; 
  }
  
  return true;
  
}
