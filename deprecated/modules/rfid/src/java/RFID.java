import com.rabbitmq.client.ConnectionFactory;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.Channel;

import java.util.Date;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;

public class RFID {
	private final String HOST_NAME = "mekosoft.vn";
	private final String USERNAME = "admin";
	private final String PASSWORD = "admin";
	private final String C_BINARY_DIR = "./rfid";
    private final String EXCHANGE_NAME = "ex_sensors";
    private final String ROUTING_KEY_PREFIX = "event.rfid.";
    private final String RFID_NUMBER;
    private Process runningProcess = null;
	
	// RFID 01 pins
	private final String D0_PIN; // wiringPi pin
	private final String D1_PIN; // wiringPi pin

    public RFID() {
		//default value - to the 01 module
		RFID_NUMBER = "1";
		D0_PIN = "3"; // wiringPi pin
		D1_PIN = "2"; // wiringPi pin
    }

    public RFID(String rfidNumber) {
        RFID_NUMBER = rfidNumber;
		
		if (rfidNumber.equals("1")) {
			D0_PIN = "3"; // wiringPi pin
			D1_PIN = "2"; // wiringPi pin
		} else if (rfidNumber.equals("2")) {
			D0_PIN = "3"; // wiringPi pin
			D1_PIN = "2"; // wiringPi pin
		} else {
			//default value - to the 01 module
			D0_PIN = "3"; // wiringPi pin
			D1_PIN = "2"; // wiringPi pin
		}
    }

    /**
     * Get raw data and return message in format [timestamp, sensor_id, data, options]
     * @param rawData The data to be formatted
     * @return String: formatted message
     */
    public String formatMessage(String rawData) {
        long timestamp = new Date().getTime() / 1000L;
        
        // return "[" + timestamp + "," + this.SENSOR_ID + "," + rawData + "," + "null" + "]";
        return "{" +
                    "\"timestamp\": + timestamp +"," +
                    "\"event_type\":\"" + "rfid" + "\"," +
                    "\"source\":\"" + "rfid." + this.RFID_NUMBER + "\"," +
                    "\"data\":\"" + "tag_id:" + rawData + "\"" +
                "}";
    }

    /**
     * Run rfid_main to read RFID tags, output the tag and send to RabbitMQ server
     */
    public void runRfid() {
        try{
            // The command has the following format
            ProcessBuilder pBuilder = new ProcessBuilder(this.C_BINARY_DIR,
														 this.D0_PIN,
														 this.D1_PIN);
            
            pBuilder.redirectErrorStream(true);
            
            // Get the reference to running process for later termination
            this.runningProcess = pBuilder.start();

            // Get process output (process output is input in program's point of view)
            BufferedReader reader = new BufferedReader(new InputStreamReader(this.runningProcess.getInputStream()));

            // Read process output until it terminates
            while (true) {
                // Capture the process output (RFID tag in this case)
                final String line = reader.readLine();

                // line will be null if the process is terminated
                if (line == null) break;

                System.out.println(line);

                // Create new thread to send out the message
                new Thread(new Runnable(){
                    public void run() {
                        sendMessage(formatMessage(line));
                    }
                }).start();
            }
        } catch (IOException ex) {
            System.out.println("Problem running rfid: " + ex.toString());
        }
    }

    /**
     * Send message to RabbitMQ server
     * @param message The message to send
     */
    public void sendMessage(String message) {
        ConnectionFactory factory = new ConnectionFactory();
        factory.setHost(this.HOST_NAME);
        factory.setUsername(this.USERNAME);
        factory.setPassword(this.PASSWORD);

        try (Connection connection = factory.newConnection();
            Channel channel = connection.createChannel()) {

            // channel.exchangeDeclare(this.EXCHANGE_NAME, "direct");

            channel.basicPublish(this.EXCHANGE_NAME, this.ROUTING_KEY_PREFIX + this.RFID_NUMBER, null, message.getBytes());

            System.out.println(" [x] Sent to exchange '" + this.EXCHANGE_NAME + "' with routing key '" + this.ROUTING_KEY_PREFIX + this.RFID_NUMBER + "' message '" + message + "'");
        } catch (Exception ex) {
            System.out.println("Problem sending to exchange '" + this.EXCHANGE_NAME + "' with routing key '"+ this.ROUTING_KEY_PREFIX + this.RFID_NUMBER + "' message '" + message + "'\n" + ex.toString());
            ex.printStackTrace();
        }
    }

    public static void main(String[] args) {
        System.out.println("Running...");
        if (args.length == 0)
            new RFID().runRfid();
        if (args.length == 1)
            new RFID(args[0]).runRfid();
        else {
            String message = "Usage:\n";
            message += "java -cp <jars> RFID\t\tRun test with SENSOR_NUMBER = 0\n";
            message += "java -cp <jars> RFID <SENSOR_NUMBER>\n";
            System.out.println(message);
        }
        
        // System.out.println(new RFID().formatMessage("TESTING ONLY"));
    }
} 
