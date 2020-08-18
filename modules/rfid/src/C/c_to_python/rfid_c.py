import subprocess as sp
import pika
import sys
import time

HOST_NAME = 'mekosoft.vn'
USERNAME = 'admin'
PASSWORD = 'admin'

C_BINARY_DIR = "./rfid"

class RFID:
    def __init__(self, rfid_number="1"):
        self.RFID_NUMBER = rfid_number

        self.EXCHANGE_NAME = 'ex_sensors'
        self.ROUTING_KEY_PREFIX = "event.rfid."

        self.isConnected = False

        try:
            self.connection = pika.BlockingConnection(
                pika.ConnectionParameters(
                    host=HOST_NAME,
                    credentials=pika.PlainCredentials(USERNAME, PASSWORD)
                )
            )
        except pika.exceptions.AMQPConnectionError:
            print("Can't connect to RabbitMQ server (no internet connection or behind a proxy?)")
        except Exception as ex:
            print(repr(ex))
            sys.exit(1)
        else:
            self.isConnected = True

        self.channel = self.connection.channel()

    def __del__(self):
        if self.isConnected:
            self.connection.close()

    def format_message(self, raw_data):
        timestamp = int(time.time())

        # to_dec = int(raw_data, 16)
        return ("{" +
                    "\"timestamp\":" + str(timestamp) + "," +
                    "\"event_type\":\"" + "rfid" + "\"," +
                    "\"source\":\"" + "rfid." + self.RFID_NUMBER + "\"," +
                    "\"data\":\"" + "tag_id:" + raw_data + "\"" +
                    # "\"data\":\"" + "tag_id:" + str(to_dec) + "\"" +
                "}")

    def send_message(self, message):
        self.channel.basic_publish(
            exchange=self.EXCHANGE_NAME,
            routing_key=self.ROUTING_KEY_PREFIX + self.RFID_NUMBER,
            body=message
        )
        print("Python: message sent: " + message)


if (len(sys.argv) < 2):
    print("Usage:")
    print("\tpython3 rfid_c.py <RFID tag> [<RFID ID> = 1]")
    sys.exit(1)

# do a if b else c
rfid = RFID(sys.argv[2] if len(sys.argv) == 3 else "1")

if rfid.isConnected:
    rfid.send_message(rfid.format_message(sys.argv[1]))
else:
    print('Problem connecting to RabbitMQ server, discarding tag ID "' + sys.argv[1] + '"')