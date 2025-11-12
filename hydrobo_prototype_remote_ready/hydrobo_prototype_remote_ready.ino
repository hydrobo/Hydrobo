/*
  Arduino FS-I6X Demo
  fsi6x-arduino-uno.ino
  Read output ports from FS-IA6B receiver module
  Display values on Serial Monitor
  
  Channel functions by Ricardo Paiva - https://gist.github.com/werneckpaiva/
  
  DroneBot Workshop 2021
  https://dronebotworkshop.com
*/

// Define Input Connections
#define CH1 A0
#define CH2 A1
#define CH3 A2
#define CH4 A3
#define CH5 A4
#define CH6 A5

int enA = 9;   // PWM pin for left motor 
int in1 = 7;    
int in2 = 6; 
int enB = 10;  // PWM pin for right motor 
int in3 = 5; 
int in4 = 4;

// Integers to represent values from sticks and pots
int ch1Value;
int ch2Value;
int ch3Value;
int ch4Value; 
int ch5Value;

// Boolean to represent switch value
bool ch6Value;

// Motor speed variables 
int leftSpeed = 0; 
int rightSpeed = 0; 
int targetLeftSpeed = 0; 
int targetRightSpeed = 0; 
// Speed limits 
int maxSpeed = 255; 
int minSpeed = 0; 
// Smoothing factor (smaller = slower acceleration) 
int accelerationStep = 5;

// Moves 'current' toward 'target' by at most 'step' and returns new value.
int approachSpeed(int current, int target, int step) {
  if (current < target) {
    current += step;
    if (current > target) current = target;
  } else if (current > target) {
    current -= step;
    if (current < target) current = target;
  }
  return current;
}

// Read the number of a specified channel and convert to the range provided.
// If the channel is off, return the default value
int readChannel(int channelInput, int minLimit, int maxLimit, int defaultValue){
  int ch = pulseIn(channelInput, HIGH, 30000);
  if (ch < 100) return defaultValue;
  return map(ch, 1000, 2000, minLimit, maxLimit);
}

// Read the switch channel and return a boolean value
bool readSwitch(byte channelInput, bool defaultValue){
  int intDefaultValue = (defaultValue)? 100: 0;
  int ch = readChannel(channelInput, 0, 100, intDefaultValue);
  return (ch > 50);
}

void setup(){
  // Set up serial monitor
  Serial.begin(115200);
  
  // Set all pins as inputs
  pinMode(CH1, INPUT);
  pinMode(CH2, INPUT);
  pinMode(CH3, INPUT);
  pinMode(CH4, INPUT);
  pinMode(CH5, INPUT);
  pinMode(CH6, INPUT);

  pinMode(enA, OUTPUT); 
  pinMode(in1, OUTPUT); 
  pinMode(in2, OUTPUT); 
  pinMode(enB, OUTPUT); 
  pinMode(in3, OUTPUT); 
  pinMode(in4, OUTPUT);

}


void loop() {
  
  // Get values for each channel
  ch1Value = readChannel(CH1, -100, 100, 0);
  ch2Value = readChannel(CH2, -100, 100, 0);
  ch3Value = readChannel(CH3, -100, 100, -100);
  ch4Value = readChannel(CH4, -100, 100, 0);
  ch5Value = readChannel(CH5, -100, 100, 0);
  ch6Value = readSwitch(CH6, false);

  // Settings
  const int deadband = 5;        // joystick deadzone
  const int baseMaxSpeed = 255;  // hardware max PWM
  const int minDriveSpeed = 0;   // minimum applied speed

  // If you don't want this behaviour, set speedLimiterPercent = 100.
  int speedLimiterPercent = 100;
  if (speedLimiterPercent < 10) speedLimiterPercent = 10; // avoid zero unless you want
  int effectiveMaxSpeed = (baseMaxSpeed * speedLimiterPercent) / 100;

  // Determine throttle magnitude from ch2Value
  int throttle = ch2Value; // -100..100
  if (abs(throttle) <= deadband) {
    // no movement
    targetLeftSpeed = 0;
    targetRightSpeed = 0;
    stopMotors();
  } else {
    // magnitude 0..100 (absolute joystick deflection)
    int magnitude = abs(throttle);
    // Map magnitude to 0..effectiveMaxSpeed
    int speedFromStick = map(magnitude, 0, 100, 0, effectiveMaxSpeed);
    // base left/right before steering
    int baseLeft = speedFromStick;
    int baseRight = speedFromStick;

    // Steering adjustment from ch1Value (-100..100)
    int steer = ch4Value;
    // steeringFactor is 0..100 representing how strongly we turn
    int steeringFactor = constrain(abs(steer), 0, 100);

    // Apply steering by reducing the inner wheel speed.
    // If steer > 0 => turning right => reduce right wheel
    // If steer < 0 => turning left  => reduce left wheel
    // reduction scales linearly with steeringFactor
    int reduction = (baseRight * steeringFactor) / 100; // max reduction
    if (steer > deadband) {
      // turning right
      baseRight = baseRight - reduction;
      if (baseRight < 0) baseRight = 0;
    } else if (steer < -deadband) {
      // turning left
      baseLeft = baseLeft - reduction;
      if (baseLeft < 0) baseLeft = 0;
    }

    // Now apply forward/backward direction
    if (throttle > 0) {
      // forward: motors set for forward
      forward();
      targetLeftSpeed  = constrain(baseLeft, minDriveSpeed, effectiveMaxSpeed);
      targetRightSpeed = constrain(baseRight, minDriveSpeed, effectiveMaxSpeed);
    } else {
      // backward: motors set for backward
      backward();
      targetLeftSpeed  = constrain(baseLeft, minDriveSpeed, effectiveMaxSpeed);
      targetRightSpeed = constrain(baseRight, minDriveSpeed, effectiveMaxSpeed);
    }
  }

  // Smooth speed ramping
  leftSpeed  = approachSpeed(leftSpeed, targetLeftSpeed, accelerationStep);
  rightSpeed = approachSpeed(rightSpeed, targetRightSpeed, accelerationStep);

  // Apply speeds to motors
  analogWrite(enA, leftSpeed);
  analogWrite(enB, rightSpeed);

  // Debug
  Serial.print("Left PWM: "); Serial.print(leftSpeed);
  Serial.print(" | Right PWM: "); Serial.println(rightSpeed);

  Serial.print("Ch1(steer): "); Serial.print(ch1Value);
  Serial.print(" | Ch2(throttle): "); Serial.print(ch2Value);
  Serial.print(" | Ch3(limit%): "); Serial.print(speedLimiterPercent);
  Serial.print(" | Ch6(switch): "); Serial.println(ch6Value);
/*
  // Print to Serial Monitor
  Serial.print("Ch1: ");
  Serial.print(ch1Value);
  Serial.print(" | Ch2: ");
  Serial.print(ch2Value);
  Serial.print(" | Ch3: ");
  Serial.print(ch3Value);
  Serial.print(" | Ch4: ");
  Serial.print(ch4Value);
  Serial.print(" | Ch5: ");
  Serial.print(ch5Value);
  Serial.print(" | Ch6: ");
  Serial.println(ch6Value);
*/
  delay(500);
}

void forward() { 
  digitalWrite(in1, HIGH); 
  digitalWrite(in2, LOW); 
  digitalWrite(in3, HIGH); 
  digitalWrite(in4, LOW); 
} 
void backward() { 
  digitalWrite(in1, LOW); 
  digitalWrite(in2, HIGH); 
  digitalWrite(in3, LOW); 
  digitalWrite(in4, HIGH); 
} 
void stopMotors() { 
  digitalWrite(in1, LOW); 
  digitalWrite(in2, LOW); 
  digitalWrite(in3, LOW); 
  digitalWrite(in4, LOW); 
}