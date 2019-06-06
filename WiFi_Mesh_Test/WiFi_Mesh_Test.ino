#include "painlessMesh.h"
#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;
// the array to store the id of others
int arrSize = 10;
uint32_t nodeArr[10] = {0};
String Instructions[4] = {"_FORWARD_", "_BACK_", "_LEFT_", "_RIGHT_"};

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

void sendMessage() {
  String msg = Instructions[int(random(0, 4))];
  Serial.println(msg);
  uint32_t target;
  int randomNum;
  for (int i = 0; i < arrSize; i ++){
    if (nodeArr[i] != 0){
      target = nodeArr[i];
      if (mesh.sendSingle( target, msg )){
        // if the message could be sent
        // print out the system message successful
        Serial.println("Successfully sent");
        taskSendMessage.setInterval(TASK_SECOND * 5);
      } else {
        // if transfer not succesul
        // remove the target from the array (user log out of the mesh system)
        Serial.println("Unsuccessful");
        nodeArr[randomNum] = 0;
      }
      break;
    } else {
      Serial.println("Stand by");
    }
  }
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
    // add the id of the new connection to the array established above
    for (int i = 0; i < sizeof(nodeArr); i ++ ){
      if (nodeArr[i] == 0){
        nodeArr[i] = nodeId;
        break;
      }
    }
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void setup() {
  Serial.begin(115200);

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
}

void loop() {

  userScheduler.execute(); // it will run mesh scheduler as well
  mesh.update();
}
