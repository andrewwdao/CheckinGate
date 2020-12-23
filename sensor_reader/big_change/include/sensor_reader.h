
#define en_rabbitmq   1
#define en_pir		  1
#define en_rfid	 	  0
#define en_uhf_w26    0
#define en_uhf_rs232  1
#define en_camera	  1

// ------------------------- Constants -----------------------------------
// --- RabitMQ server infos
#define EXCHANGE_NAME 		"ex_sensors"
#define ROUTING_KEY_PREFIX	"event"
#define HOST				"demo1.gate.mekosoft.vn"
#define USERNAME			"admin"
#define PASSWORD			"admin"
#define PORT 				5672

// --- PIR parameters
#define PIR_DEBOUNCE 	   200000 //us
#define PIR_STATE_DEBOUNCE 100000 //us
#define PIR_CNT	    	   3
#define PIR_1_PIN	       4  //wiringpi pin
#define PIR_2_PIN	       1  //wiringpi pin
#define PIR_3_PIN	       3  //wiringpi pin
#define PIR_STATE_ID	   3
#define OTHER_SENSOR_ID    0

// --- RFID parameters
#define MAIN_RFID_1 1 //index for rfid module 1
#define MAIN_RFID_2 2 //index for rfid module 2
#define RFID_1_D0_PIN 7 //wiringpi pin
#define RFID_1_D1_PIN 0 //wiringpi pin
#define RFID_2_D0_PIN 2 //wiringpi pin
#define RFID_2_D1_PIN 3 //wiringpi pin
#define OE_PIN 		  11 //wiringpi pin

// --- UHF parameters
#define UHF_PORT 	  "/dev/serial0"
#define UHF_BAUDRATE  115200
#define MAIN_UHF   3  //index for uhf
#define UHF_D0_PIN 10 //wiringpi pin
#define UHF_D1_PIN 11 //wiringpi pin
#define UHF_DELAY  5000 //read interval (ms)

// --- Camera parameter
#define IMAGE_LIMIT	  10000
#define IMAGE_DIR 	  "./web/public/images"
