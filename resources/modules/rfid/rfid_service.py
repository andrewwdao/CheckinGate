import subprocess as sp
import pika
import sys
import time

HOST_NAME = 'mekosoft.vn'
USERNAME = 'admin'
PASSWORD = 'admin'

C_BINARY_DIR = "./rfid"

class RFID:
    def __init__(self, rfid_number="1", d0_pin="3", d1_pin="2"):
        self.RFID_NUMBER = rfid_number
        self.D0_PIN = d0_pin
        self.D1_PIN = d1_pin

        self.EXCHANGE_NAME = 'ex_sensors'
        self.ROUTING_KEY_PREFIX = "event.rfid."

        self.connection = pika.BlockingConnection(
            pika.ConnectionParameters(
                host=HOST_NAME,
                credentials=pika.PlainCredentials(USERNAME, PASSWORD)
            )
        )
        self.channel = self.connection.channel()

        self.rfid_process = self.rfid = sp.Popen(
            [C_BINARY_DIR, self.RFID_NUMBER],
            # ['./test'],
            shell=False,
            stdout=sp.PIPE,
            stderr=sp.STDOUT
        )
        self.stream = self.rfid_process.stdout

    def __del__(self):
        self.connection.close()
        self.rfid_process.terminate()
        self.rfid_process.kill()

    def run(self):
        print("Running...")
        while True:
            rawData = self.stream.readline()
            if rawData:
                data = rawData.strip().decode("utf-8")
                print(data)
                self.send_message(self.format_message(data))
            else:
                print('End of Stream')
                return

    def format_message(self, raw_data):
        timestamp = int(time.time())

        return ("{" +
                    "\"timestamp\":" + str(timestamp) + "\"," +
                    "\"event_type\":\"" + "rfid" + "\"," +
                    "\"source\":\"" + "rfid." + self.RFID_NUMBER + "\"," +
                    "\"data\":\"" + "tag_id:" + raw_data + "\"" +
                "}")

    def send_message(self, message):
        self.channel.basic_publish(
            exchange=self.EXCHANGE_NAME,
            routing_key=self.ROUTING_KEY_PREFIX + self.RFID_NUMBER,
            body=message
        )

        print(
            " [x] Sent to exchange '" + self.EXCHANGE_NAME +
            "' with routing key '" + self.ROUTING_KEY_PREFIX + self.RFID_NUMBER +
            "' message '" + message + "'"
        )

if __name__=='__main__':
    if len(sys.argv) > 2:
        print('Usage:')
        print('\tpython3 rfid.py\tRun with RFID_ID = 1')
        print('\tpython3 rfid.py <RFID_ID>')
        sys.exit(1)
    
    if (len(sys.argv) == 1):
        RFID().run()
    else:
        RFID(sys.argv[1]).run()