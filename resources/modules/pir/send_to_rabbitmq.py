import subprocess as sp
import pika
import sys
import time

HOST_NAME = 'mekosoft.vn'
USERNAME = 'admin'
PASSWORD = 'admin'

class Sensor:
    def __init__(self, sensor, sensor_number):
        self.sensor = sensor
        self.SENSOR_NUMBER = sensor_number

        self.EXCHANGE_NAME = 'ex_sensors'
        self.ROUTING_KEY_PREFIX = "event.{}.".format(sensor)

        self.connection = pika.BlockingConnection(
            pika.ConnectionParameters(
                host=HOST_NAME,
                credentials=pika.PlainCredentials(USERNAME, PASSWORD)
            )
        )
        self.channel = self.connection.channel()

    def __del__(self):
        self.connection.close()

    def format_message(self, raw_data):
        timestamp = int(time.time())

        # return ("{" +
        #             "\"timestamp\":" + str(timestamp) + "\"," +
        #             "\"event_type\":\"" + sensor + "\"," +
        #             "\"source\":\"" + sensor + "." + self.SENSOR_NUMBER + "\"," +
        #             "\"data\":\"" + "tag_id:" + raw_data + "\"" +
        #         "}")

        return ('{' +
                '"timestamp":{},'.format(str(timestamp))+
                '"event_type":"{}",'.format(self.sensor)+
                '"source":"{}.{}",'.format(self.sensor, self.SENSOR_NUMBER)+
                '"data":"{}"'.format(raw_data)+
                '}')

    def send_message(self, message):
        self.channel.basic_publish(
            exchange=self.EXCHANGE_NAME,
            routing_key=self.ROUTING_KEY_PREFIX + self.SENSOR_NUMBER,
            body=message
        )


if len(sys.argv) < 4:
    print("Usage:\n\tpython3 send_rabbitmq.py <sensor_name> <sensor_number> <data>")
    sys.exit(1)

sensor = Sensor(sys.argv[1], sys.argv[2])
sensor.send_message(sensor.format_message(sys.argv[3]))
