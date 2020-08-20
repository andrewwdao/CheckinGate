import serial

# Reader commands
RESET_CMD         = 0x70
READ_CMD          = 0x81
RT_INVENTORY_CMD  = 0x89 # Real time inventory
GET_READER_ID_CMD = 0x68 # Get reader identifier
BUZZER_CMD        = 0x7A

# Buzzer modes
BUZZER_QUIET     = 0x00
BUZZER_ROUND     = 0x01
BUZZER_TAG       = 0x02

# Membank
RESERVED_MEMBANK = 0x00
EPC_MEMBANK      = 0x01
TID_MEMBANK      = 0x02
USER_MEMBANK     = 0x03

# Needed to format package
HEADER           = 0xA0
READER_ADDRESS   = 0x01
PUBLIC_ADDRESS   = 0xFF

class UHFReader():
    # Read datasheet for checksum function (written in C)
    def get_checksum(self, data):
        cs = 0x00
        for i in data:
            cs += i
            cs %= 256
        
        cs = ~cs + 1
        cs %= 256
        return cs
    
    def check_packet_checksum(self, bytes_arr):
        return (self.get_checksum(bytes_arr[:-1]) == bytes_arr[-1])

    def format_command(self, data):
        # Message length count from third byte, exculuding header and len byte (address -> check)
        message_length = len(data) + 2
        data = [HEADER, len(data) + 2, READER_ADDRESS] + data
        data.append(self.get_checksum(data))
        return bytearray(data)

    def get_hex_string(self, bytes_str):
        hex_str = ''
        ugly_hex_str = bytes_str.hex()
        for i in range(len(ugly_hex_str)):
            hex_str += ugly_hex_str[i]
            if i%2==1:
                hex_str += ' '
        return hex_str.upper()
    
    def get_int_arr(self, bytes_str):
        hex_arr = []
        hex_str = bytes_str.hex()
        for i in range(0, len(hex_str), 2):
            hex_arr.append(int(hex_str[i] + hex_str[i+1], 16))
        return hex_arr

    def open_connection(self, port, baudrate=115200, timeout=1):
        self.ser = serial.Serial(port=port, baudrate=baudrate, timeout=timeout)
        self.ser.close()
        self.ser.open()
    
    def close_connection(self):
        self.ser.close()

    def reset_reader(self):
        self.ser.write(self.format_command([RESET_CMD]))

    def read_tag(self, membank=0x02, word_address=0x00, word_cnt=0x01):
        self.ser.write(self.format_command([READ_CMD, membank, word_address, word_cnt]))

    def realtime_inventory(self):
        repeat = 0x01
        self.ser.write(self.format_command([RT_INVENTORY_CMD, repeat]))

    def set_beeper_mode(self, mode):
        self.ser.write(self.format_command([BUZZER_CMD, mode]))

    def read_output(self, n_bytes=1):
        return self.ser.read(n_bytes)


'''
===============================
          Driver code
===============================
'''

uhf = UHFReader()
uhf.open_connection("COM6")
# uhf.reset_reader()
# uhf.set_beeper_mode(BUZZER_QUIET)

while True: 
    # uhf.read_tag(membank=TID_MEMBANK, word_address=0x01, word_cnt=0x06)
    # read_byte = uhf.read_output() # Read header
    # if read_byte != b'\xA0':
    #     continue

    # packet_length = uhf.read_output() # Read package length
    # packet_length = int.from_bytes(packet_length, "big")

    # line = uhf.read_output(packet_length)
    line = uhf.read_output(200)
    if line == b'':
        continue
    print(uhf.get_hex_string(line))