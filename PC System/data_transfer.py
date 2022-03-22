import serial
import time

def table_transmission(ser, mode,
                       level_1=[], level_2={}, level_3={}, problem_lane=0,  # for Tables mode
                       vehicles_number_in_lane=0, vehicles_number_enter=0,  # for Check mode
                       time_in=[], cars_in=[],  # for Lane map mode

                       ):

    if mode == 'Tables':
        pc_transmission = '$@PL{num_lane}$'.format(num_lane=problem_lane)
        print(pc_transmission)
        pc_transmission = pc_transmission.encode()
        ser.write(pc_transmission)
        time.sleep(2)
        for level_i in range(3):
            if level_i == 0:  # level_1
                data_transmission = '$@1'
                for str_num in level_1:
                    if int(str_num) > 99: str_num = '99'
                    data_transmission += str_num.zfill(2)
                data_transmission += '$'
                print('level 1:')
                print(data_transmission)
                pc_transmission = data_transmission
                pc_transmission = pc_transmission.encode()
                ser.write(pc_transmission)
                time.sleep(1)
            elif level_i == 1:  # level_2
                data_transmission = '$@2'
                for dict_tuple in level_2.values():
                    data_transmission += dict_tuple[0] + dict_tuple[2]
                data_transmission += '$'
                print('level 2:')
                print(data_transmission)
                print('level 2 packages:')
                packages = len(data_transmission) // 64
                for package_i in range(packages + 1):
                    print('level 2 packages number ', package_i + 1)
                    pc_transmission = data_transmission[:64]
                    pc_transmission = pc_transmission.encode()
                    ser.write(pc_transmission)
                    print(pc_transmission)
                    data_transmission = data_transmission[64:]
                time.sleep(1)
            else:  # level_3
                data_transmission = '$@3'
                for dict_tuple in level_3.values():
                    data_transmission += dict_tuple[2] + dict_tuple[3]
                data_transmission += '$'
                print('level 3:')
                print(data_transmission)
                print('level 3 packages:')
                packages = len(data_transmission) // 64
                for package_i in range(packages + 1):
                    print('level 3 packages number ', package_i + 1)
                    pc_transmission = data_transmission[:64]
                    pc_transmission = pc_transmission.encode()
                    ser.write(pc_transmission)
                    print(pc_transmission)
                    data_transmission = data_transmission[64:]
                time.sleep(0.5)

    if mode == 'Check':
        if vehicles_number_in_lane > 99: vehicles_number_in_lane = 99
        if vehicles_number_enter > 99: vehicles_number_enter = 99
        data_transmission = '$@C'
        data_transmission += str(vehicles_number_enter).zfill(2)
        data_transmission += str(vehicles_number_in_lane).zfill(2)
        data_transmission += '$'
        pc_transmission = data_transmission.encode()
        ser.write(pc_transmission)

    if mode == 'Lane map':
        data_transmission = '$@LM'
        for i in range(len(time_in)):
            data_transmission += str(time_in[i]).zfill(3)
            data_transmission += str(cars_in[i]).zfill(2)
        data_transmission += '$'
        pc_transmission = data_transmission.encode()
        ser.write(pc_transmission)

def read_command(ser):
    data_read = ''
    data_input = ''
    bytes_to_read = ser.in_waiting
    if bytes_to_read != 0:
        data_read = ser.read(1)
        data_read = data_read.decode()
        if data_read == '$':
            time.sleep(0.2)
            counter_bytes = 0
            while True:
                data_read = ser.read(1)
                counter_bytes += 1
                data_read = data_read.decode()
                if data_read == '$':
                    break
                else:
                    data_input += data_read
        else:
            counter_bytes = 0
            while True:
                data_read = ser.read(1)
                counter_bytes += 1
                data_read = data_read.decode()
                if data_read == '$':
                    data_input = ''
                    break
    return data_input