import json

def read_can_database(file_path):
    with open(file_path, 'r') as file:
        can_database = json.load(file)
    return can_database

def extract_bits(data, start_bit, length):
    value = 0
    for i in range(length):
        byte_index = (start_bit + i) // 8
        bit_index = (start_bit + i) % 8
        value |= ((data[byte_index] >> bit_index) & 0x01) << i
    return value

def decode_bit_fields(value, bit_fields):
    for bit_field in bit_fields:
        if 'bit_range' in bit_field:
            start_bit, end_bit = map(int, bit_field['bit_range'].split('-'))
            bit_length = end_bit - start_bit + 1
            bit_value = (value >> start_bit) & ((1 << bit_length) - 1)
            print(f"{bit_field['description']}: {bit_value}")
        else:
            bit = bit_field['bit']
            description = bit_field['description']
            bit_value = (value >> bit) & 0x01
            print(f"{description}: {bit_value}")

def decode_can_message(can_database, msg_id, data):
    for message in can_database['messages']:
        if int(message['id'], 16) == msg_id:
            for signal in message['signals']:
                start_bit = signal['start_bit']
                length = signal['length']
                factor = signal['factor']
                offset = signal['offset']
                value = extract_bits(data, start_bit, length) * factor + offset
                print(f"{signal['name']}: {value} {signal['unit']}")
                if 'bit_fields' in signal:
                    raw_value = extract_bits(data, start_bit, length)
                    decode_bit_fields(raw_value, signal['bit_fields'])

can_database = read_can_database('can_database.json')

# Example CAN message (ID: 0x0CF11F05, Data: [0x00, 0x00, 0x00, 0x00, 0x00])
example_message_id = 0x0CF11F05
example_data = [0x00, 0x00, 0x00, 0x00, 0x00]

decode_can_message(can_database, example_message_id, example_data)
