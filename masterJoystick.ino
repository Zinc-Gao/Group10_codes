#include <SoftwareSerial.h>   // Software Serial Port

#define RxD 7
#define TxD 6
#define ConnStatus A1         // Connection status on the SeeedStudio v1 shield is available on pin A1

#define DEBUG_ENABLED  1

int shieldPairNumber = 17;
int JoyStick_X = 0; //x
int JoyStick_Y = 3; //y
int JoyStick_Z = 2; //key


// MUST NOT use pin A1
// MUST set the PIO[1]

// The following four string variable are used to simplify adaptation of code to different shield pairs

String slaveName = "Slave";                  // This is concatenated with shieldPairNumber later
String masterNameCmd = "\r\n+STNA=Master";   // This is concatenated with shieldPairNumber later
String connectCmd = "\r\n+CONN=";            // This is concatenated with slaveAddr later

int nameIndex = 0;
int addrIndex = 0;

String recvBuf;
String slaveAddr;
String retSymb = "+RTINQ=";   // Start symble when there's any return

SoftwareSerial blueToothSerial(RxD,TxD);


void setup()
{
    Serial.begin(9600);
    blueToothSerial.begin(38400);                    // Set Bluetooth module to default baud rate 38400

    pinMode(RxD, INPUT);
    pinMode(TxD, OUTPUT);
    pinMode(ConnStatus, INPUT);

    //  Check whether Master and Slave are already connected by polling the ConnStatus pin (A1 on SeeedStudio v1 shield)
    //  This prevents running the full connection setup routine if not necessary.

    Serial.println("Checking Master-Slave connection status.");

    if (digitalRead(ConnStatus)==1) {
        Serial.println("Already connected to Slave - remove USB cable if reboot of Master Bluetooth required.");
    }

    else {
        Serial.println("Not connected to Slave.");

        setupBlueToothConnection();     // Set up the local (master) Bluetooth module
        getSlaveAddress();              // Search for (MAC) address of slave
        makeBlueToothConnection();      // Execute the connection to the slave

        delay(1000);                    // Wait one second and flush the serial buffers
        Serial.flush();
        blueToothSerial.flush();
    }
}


void loop() {
  if (blueToothSerial.available()) {
    char recvChar = blueToothSerial.read();
    Serial.print(recvChar);
  }

  int x,y,z;
  x = analogRead(JoyStick_X);
  y = analogRead(JoyStick_Y);
  z = digitalRead(JoyStick_Z);
  delay(100);

  if ( x >= 0 and x <= 510) {
    blueToothSerial.print('f');
  }
  if ( x >= 517 and x <= 1023) {
    blueToothSerial.print('b');
  }
  if ( y >= 517 and y <= 1023) {
    blueToothSerial.print('r');
  }
  if ( y >= 0 and x <= 510) {
    blueToothSerial.print('l');
  }
}


void setupBlueToothConnection()
{
    Serial.println("Setting up the local (master) Bluetooth module.");

    masterNameCmd += shieldPairNumber;
    masterNameCmd += "\r\n";

    blueToothSerial.print("\r\n+STWMOD=1\r\n");      // Set the Bluetooth to work in master mode
    blueToothSerial.print(masterNameCmd);            // Set the bluetooth name using masterNameCmd
    blueToothSerial.print("\r\n+STAUTO=0\r\n");      // Auto-connection is forbidden here

    blueToothSerial.flush();                         // Pauses the program until all Serial.print()ing is done.
    delay(2000);                                     // This delay is required

    blueToothSerial.print("\r\n+INQ=1\r\n");         // Make the master Bluetooth inquire

    blueToothSerial.flush();
    delay(2000);                                     // This delay is required

    Serial.println("Master is inquiring!");
}


void getSlaveAddress()
{
    slaveName += shieldPairNumber;

    Serial.print("Searching for address of slave: ");
    Serial.println(slaveName);

    slaveName = ";" + slaveName;   // The ';' must be included for the search that follows

    char recvChar;

    // Initially, if(blueToothSerial.available()) will loop and, character-by-character, fill recvBuf to be:
    //    +STWMOD=1 followed by a blank line
    //    +STNA=MasterTest (followed by a blank line)
    //    +S
    //    OK (followed by a blank line)
    //    OK (followed by a blank line)
    //    OK (followed by a blank line)
    //    WORK:
    //
    // It will then, character-by-character, add the result of the first device that responds to the +INQ request:

    while(1)
    {
        if(blueToothSerial.available())
        {
            recvChar = blueToothSerial.read();
            recvBuf += recvChar;

            nameIndex = recvBuf.indexOf(slaveName);   // Get the position of slave name

            if ( nameIndex != -1 )   // ie. if slaveName was found
            {
                addrIndex = (recvBuf.indexOf(retSymb,(nameIndex - retSymb.length()- 18) ) + retSymb.length());   // Get the start position of slave address
                slaveAddr = recvBuf.substring(addrIndex, nameIndex);   // Get the string of slave address

                Serial.print("Slave address found: ");
                Serial.println(slaveAddr);

                break;  // Only breaks from while loop if slaveName is found
            }
        }
    }
}


void makeBlueToothConnection()
{
    Serial.println("Initiating connection with slave.");

    char recvChar;

    // Having found the target slave address, now form the full connection command

    connectCmd += slaveAddr;
    connectCmd += "\r\n";

    int connectOK = 0;       // Flag used to indicate succesful connection
    int connectAttempt = 0;  // Counter to track number of connect attempts

    // Keep trying to connect to the slave until it is connected (using a do-while loop)

    do
    {
        Serial.print("Connect attempt: ");
        Serial.println(++connectAttempt);

        blueToothSerial.print(connectCmd);   // Send connection command

        // Initially, if(blueToothSerial.available()) will loop and, character-by-character, fill recvBuf to be:
        //    OK
        //
        //    +BTSTATE:3
        //
        //    (BTSTATE:3 = Connecting)
        //
        // It will then, character-by-character, add the result of the connection request.
        // If that result is "CONNECT:OK", the while() loop will break and the do() loop will exit.
        // If that result is "CONNECT:FAIL", the while() loop will break with an appropriate "FAIL" message
        // and a new connection command will be issued for the same slave address.

        recvBuf = "";

        while(1)
        {
            if(blueToothSerial.available())
            {
                recvChar = blueToothSerial.read();
                recvBuf += recvChar;

                if(recvBuf.indexOf("CONNECT:OK") != -1)
                {
                    connectOK = 1;
                    Serial.println("Connected to slave!");
                    blueToothSerial.print("Master-Slave connection established!");
                    break;
                }
                else if(recvBuf.indexOf("CONNECT:FAIL") != -1)
                {
                    Serial.println("Connection FAIL, try again!");
                    break;
                }
            }
        }
    } while (0 == connectOK);

}
