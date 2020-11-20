// Bluetooth initialisation
#include <SoftwareSerial.h>   //Software Serial Port

#define RxD 7
#define TxD 6
#define ConnStatus A1

#define DEBUG_ENABLED  1

int shieldPairNumber = 17;
String slaveNameCmd = "\r\n+STNA=Slave";\
SoftwareSerial blueToothSerial(RxD,TxD);

// Servo initialisation
#include <Servo.h>                            // Include servo library
 
Servo servoLeft;                              // Declare left and right servos
Servo servoRight;

String path = "";

void setup()                                  // Built-in initialization block
{ 
  pinMode(10, INPUT);  pinMode(9, OUTPUT);    // Left IR LED & Receiver
  pinMode(3, INPUT);  pinMode(2, OUTPUT);     // Right IR LED & Receiver
  pinMode(5, INPUT); pinMode(4, OUTPUT);     // Front IR LED & Receiver

  servoLeft.attach(13);                       // Attach left signal to pin 13
  servoRight.attach(12);                      // Attach right signal to pin 12
  Serial.begin(9600);

  blueToothSerial.begin(38400);                    // Set Bluetooth module to default baud rate 38400
    
  pinMode(RxD, INPUT);
  pinMode(TxD, OUTPUT);
  pinMode(ConnStatus, INPUT);

  Serial.println("Checking Slave-Master connection status.");

  if (digitalRead(ConnStatus)==1) {
        Serial.println("Already connected to Master - remove USB cable if reboot of Master Bluetooth required.");
  }
  
  else {
    Serial.println("Not connected to Master.");
    
    setupBlueToothConnection();   // Set up the local (slave) Bluetooth module

    delay(1000);                  // Wait one second and flush the serial buffers
    Serial.flush();
    blueToothSerial.flush();
  }
}  
 
void loop() {
  
  int irLeft = irDetect(9, 10, 38000);        // Check for object on left
  int irRight = irDetect(2, 3, 40000);        // Check for object on right
  int irFront = irDetect(4, 5, 27000);        // Check for object in front

//  // print (Left, right, front)
//  Serial.print("(");  Serial.print(irLeft);   Serial.print(", ");
//  Serial.print(irRight);   Serial.print(", ");
//  Serial.print(irFront);   Serial.println(")");




  // *************** Bluetooth *******************
  
  // Check if there's any data sent from the remote Bluetooth shield
  if (blueToothSerial.available()) {
      char recvChar = blueToothSerial.read();
      Serial.println(recvChar);
      if (recvChar == 'f') {
        moveTowards("front");
      }
      else if (recvChar == 'l') {
        moveTowards("left");
      }
      else if (recvChar == 'r') {
        moveTowards("right");
      }
      else if (recvChar == 'b') {
        moveTowards("back");
      } 
  }

  





  

  // both left and right sides see the maze wall -> move foward
  if ((irLeft == 0) && (irRight == 0) && (irFront == 1)) {  
    moveTowards("front");
  }
  
  // dead end -> pick up ball and trace back to start
  else if ((irLeft == 0) && (irRight == 0) && (irFront == 0)) {  
    // sent data from the local serial terminal
    String str = "find the ball";
    for (int i = 0; i < sizeof(str) - 1; i++) {
      blueToothSerial.print(str[i]);
    }
    blueToothSerial.print('\n');

    
    // Wait for 500 ms -> for picking up ball?
    toMove(0, 0, 500);
    
    // move backwards towards start
    int pathLength = sizeof(path) - 2;
  
    for (int i = pathLength; i >= 0; i--) {
      String direction = reverseOf(path[i]);
      moveTowards(direction);
    }

    // stop servo signal
    servoLeft.detach();
    servoRight.detach();

  }

  // front and left side sees the maze wall -> turn right
  else if ((irLeft == 0) && (irRight == 1) && (irFront == 0)) {
    moveTowards("right");
    moveTowards("front");
  }

  // front and right side sees the maze wall -> turn left
  else if ((irLeft == 1) && (irRight == 0) && (irFront == 0)) {
    moveTowards("left");
    moveTowards("front");
  }


  // only left side sees the maze wall -> choose right or front
  else if ((irLeft == 0) && (irRight == 1) && (irFront == 1)) {
    int choice = random(2);     // choose from 0 or 1

    // turn right if 0
    if (choice == 0) {
      moveTowards("right");
      moveTowards("front");
    }

    // move forward if 1
    if (choice == 1) {
      // move forward for 3000 ms (a block's distance)
      for (int i = 0; i < 6; i++) {
        moveTowards("front");
      }
    }
  }

  // only right side sees the maze wall -> choose right or front
  else if ((irLeft == 1) && (irRight == 0) && (irFront == 1)) {
    int choice = random(2);     // choose from 0 or 1

    // turn left if 0
    if (choice == 0) {
      moveTowards("left");
      moveTowards("front");
    }

    // move forward if 1
    else if (choice == 1) {
      // move forward for 3000 ms (a block's distance)
      for (int i = 0; i < 6; i++) {
        moveTowards("front");
      }
    }
  }

  // only front side sees the maze wall -> choose right or front
  else if ((irLeft == 1) && (irRight == 0) && (irFront == 1)){
    int choice = random(2);     // choose from 0 or 1

    if (choice == 0) {          // turn left if 0
      moveTowards("left");
      moveTowards("front");
    }

    else if (choice == 1) {     // turn right if 1
      moveTowards("right");
      moveTowards("front");
    }
  }

  // no obstacle at any direction -> choose front, left or right
  else {
    int choice = random(3);     // choose from 0 to 2
    
    if (choice == 0) {          // turn left if 0
      moveTowards("left");
      moveTowards("front");
    }

    else if (choice == 1) {     // turn right if 1
      moveTowards("right");
      moveTowards("front");
    }

    else if (choice == 2) {     // move forward if 2
      for (int i = 0; i < 6; i++) {
        moveTowards("front");
      }
    }
  }
  delay(100);
 }

int irDetect(int irLedPin, int irReceiverPin, long frequency)
{
  tone(irLedPin, frequency, 8);               // turn on IR LED for 1 ms
  delay(1);
  int ir = digitalRead(irReceiverPin);        // 1 if no detect, 0 detect
  delay(1);                                   // Down time before recheck
  return ir; 
}


// Backward  Linear  Stop  Linear   Forward
//   -200     -100    0     100       200
void toMove(int speedLeft, int speedRight, int duration) {
  // left wheel clockwise, right wheel anti-clockwise
  servoLeft.writeMicroseconds(1500 + speedLeft);   // Set Left servo speed
  servoRight.writeMicroseconds(1500 - speedRight); // Set right servo speed
  
  delay(duration);              // keep moving according to duration
}


void moveTowards(String direction) {
  if (direction.equals("front")) {
    toMove(-200, -200, 500);
    path += "f";
  }
  
  else if (direction.equals("left")) {
    toMove(-200, 200, 775);
    path += "l";
  }
  
  else if (direction.equals("right")) {
    toMove(200, -200, 775);
    path += "r";
  }

  else if (direction.equals("back")) {
    toMove(200, 200, 20);
    path += "b";
  }
}

String reverseOf(char direction) {
  if (direction == 'f') {
    return "back";
  }

  if (direction == 'l') {
    return "right";
  }

  if (direction == 'r') {
    return "left";
  }

  if (direction == 'b') {
    return "front";
  }
}


void setupBlueToothConnection()
{
    Serial.println("Setting up the local (slave) Bluetooth module.");

    slaveNameCmd += shieldPairNumber;
    slaveNameCmd += "\r\n";

    blueToothSerial.print("\r\n+STWMOD=0\r\n");      // Set the Bluetooth to work in slave mode
    blueToothSerial.print(slaveNameCmd);             // Set the Bluetooth name using slaveNameCmd
    blueToothSerial.print("\r\n+STAUTO=0\r\n");      // Auto-connection should be forbidden here
    blueToothSerial.print("\r\n+STOAUT=1\r\n");      // Permit paired device to connect me
    

    blueToothSerial.flush();
    delay(2000);                                     // This delay is required

    blueToothSerial.print("\r\n+INQ=1\r\n");         // Make the slave Bluetooth inquirable
    
    blueToothSerial.flush();
    delay(2000);                                     // This delay is required
    
    Serial.println("The slave bluetooth is inquirable!");
}
