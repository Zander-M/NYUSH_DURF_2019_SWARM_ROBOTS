import processing.serial.*;

String myString = null;
Serial myPort;


int NUM_OF_VALUES = 4;   /** YOU MUST CHANGE THIS ACCORDING TO YOUR PROJECT **/
int[] sensorValues;      /** this array stores values from Arduino **/

// setup the size of the screen
void setup(){
  size(800, 800);
  setupSerial();

}

// draw the coordinate
void draw(){
  updateSerial();
  printArray(sensorValues);
  noFill();
  background(255);
  line(0, height / 2, width, height / 2);
  line(width / 2, 0, width / 2, height);
  fill(sensorValues[3]);
  arc(width / 2, height / 2, width / 2, height / 2, 0, PI / 2);
  fill(sensorValues[2]);
  arc(width / 2, height / 2, width / 2, height / 2, PI / 2, PI);
  fill(sensorValues[1]);
  arc(width / 2, height / 2, width / 2, height / 2, PI, PI * 3 / 2);
  fill(sensorValues[0]);
  arc(width / 2, height / 2, width / 2, height / 2, PI * 3 / 2, PI * 2);
}

// fill the arc with different degree of shade with regard to the distance of the mouse

void setupSerial() {
  printArray(Serial.list());
  myPort = new Serial(this, Serial.list()[ 2 ], 9600);
  // WARNING!
  // You will definitely get an error here.
  // Change the PORT_INDEX to 0 and try running it again.
  // And then, check the list of the ports,
  // find the port "/dev/cu.usbmodem----" or "/dev/tty.usbmodem----" 
  // and replace PORT_INDEX above with the index number of the port.

  myPort.clear();
  // Throw out the first reading,
  // in case we started reading in the middle of a string from the sender.
  myString = myPort.readStringUntil( 10 );  // 10 = '\n'  Linefeed in ASCII
  myString = null;

  sensorValues = new int[NUM_OF_VALUES];
}



void updateSerial() {
  while (myPort.available() > 0) {
    myString = myPort.readStringUntil( 10 ); // 10 = '\n'  Linefeed in ASCII
    if (myString != null) {
      String[] serialInArray = split(trim(myString), ",");
      if (serialInArray.length == NUM_OF_VALUES) {
        for (int i=0; i<serialInArray.length; i++) {
          sensorValues[i] = int(serialInArray[i]);
        }
      }
    }
  }
}
