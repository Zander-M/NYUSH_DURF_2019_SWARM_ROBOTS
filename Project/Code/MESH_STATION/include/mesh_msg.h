#ifndef _MESH_MSG_H_
#define _MESH_MSG_H_

// buffer define size
#define RX_SIZE (1500)
#define TX_SIZE (1460)

// cmd define
#define IDLE_CMD  (0x0)
#define MOTOR_CMD (0x1)
#define LED_CMD   (0x2)
#define FIND_CMD  (0x3)

// instruction datatype
typedef struct{
    uint8_t cmd; // 
    uint8_t direction;
    uint8_t time;
} mesh_move_t;

// move direction
#define FORWARD   (0x0)
#define BACKWARD  (0x1)
#define LEFT      (0x2)
#define RIGHT     (0x3)

// LED datatype
typedef struct{
    uint8_t cmd;
    uint8_t R;
    uint8_t G;
    uint8_t B;
} mesh_led_t;

// response datatype

#define ROBOT_OK 0
#define ROBOT_ERROR 1

typedef struct{
    uint8_t rspd;
} rspd_t;


#endif 