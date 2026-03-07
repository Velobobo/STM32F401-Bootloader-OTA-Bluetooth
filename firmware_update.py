import socket
import struct
import os
import binascii

target_address = "00:22:03:01:65:00" # mac addr
port = 1  # Standard RFCOMM port for HC-05/HC-06 modules
START_BYTE = b'\xFC'
ACK_BYTE = b'\xFA'
NACK_BYTE = b'\xFB'
SOF_BYTE = b'\xAA'
PACKET_SIZE = 512
slot=0
fw_data=''
def calculate_crc32(data):
    return binascii.crc32(data) & 0xFFFFFFFF

def send_firmware_packets(sock):
    global fw_data
    total_len = len(fw_data)
    seq_num = 0
    bytes_sent = 0

    while bytes_sent < total_len:
        # 1. Slice the data chunk
        chunk = fw_data[bytes_sent : bytes_sent + PACKET_SIZE]
        actual_chunk_len = len(chunk)
        
        # 2. Calculate Packet CRC
        packet_crc = calculate_crc32(chunk)
        
        # 3. Construct Header: SOF(1) | SEQ(2) | LEN(2)
        # '<BHH' -> Little-endian: Unsigned Char, Unsigned Short, Unsigned Short
        header = struct.pack('<BHH', 
                             ord(SOF_BYTE), 
                             seq_num, 
                             actual_chunk_len)
        
        # 4. Construct Footer: CRC(4)
        # '<I' -> Little-endian: Unsigned Int
        footer = struct.pack('<I', packet_crc)
        
        # 5. Assemble Full Packet
        full_packet = header + chunk + footer
        
        # 6. Send and Wait for ACK/NACK
        retries = 3
        success = False
        
        while retries > 0:
            print(f"Sending Packet {seq_num} ({actual_chunk_len} bytes)...", end=' ')
            sock.sendall(full_packet)
            
            response = sock.recv(1)
            
            if response == ACK_BYTE:
                print("ACK")
                success = True
                break
            elif response == NACK_BYTE:
                print("NACK! Resending...")
                retries -= 1
            else:
                print(f"Unknown Response {response.hex()}. Retrying...")
                retries -= 1
        
        if not success:
            print("Failed to send packet after retries. Aborting.")
            return False
            
        # 7. Update offsets
        bytes_sent += actual_chunk_len
        print(f"Packet {seq_num} verified")
        seq_num += 1
        

    print("Firmware transfer complete!")
    response = sock.recv(1)
    if(response == ACK_BYTE):
        print("Firmware CRC Verified")
    elif(response==NACK_BYTE):
        print("Firmware CRC WRONG")
    else:
        print("Unknown respose after transfer")
    return True


def get_firmware_data(slot):
    global fw_data
    slot_name='A' if slot==b'\x00' else 'B'
    filename = fr"C:\Users\parth\STM32CubeIDE\workspace_1.19.0\Bootloader_Application\firmware_{slot_name}.bin"
    if not os.path.exists(filename):
        print(f"Error: {filename} not found!")
        return None, 0, 0
    
    with open(filename, 'rb') as f:
        fw_data = f.read()
        
    size = len(fw_data)
    crc = binascii.crc32(fw_data) & 0xFFFFFFFF 
    
    return size, crc


def start_bluetooth_serial():
    try:
        sock = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)
        
        print(f"Connecting to {target_address}...")
        sock.connect((target_address, port))
        print("Connection successful!")
        return sock

    except Exception as e:
        print(f"Error: {e}")
    

def perform_handshake(sock):
    """
    Performs the bootloader handshake:
    1. Sends START (0xFC)
    2. Waits for ACK (0xFA)
    3. Sends Size (4 bytes, Little-Endian)
    4. Sends CRC (4 bytes, Little-Endian)
    5. Waits for final ACK (0xFA)
    """
    try:
        print("Sending START byte ")
        sock.sendall(START_BYTE)
        response = sock.recv(1)
        if response != ACK_BYTE:
            print(f"Error: Expected ACK (0xFA), but got {response.hex() if response else 'nothing'}")
            return False
        print("Received first ACK.")

        slot=sock.recv(1)
        print(f"Slot Empty {'A' if (slot==b'\x00') else 'B'}")
        size,crc = get_firmware_data(slot)

        metadata = struct.pack('<II', int(size), int(crc))
        sock.sendall(metadata)
        print(f"Metadata sent: Size={size}, CRC={hex(crc)}")

        response = sock.recv(1)
        if response == ACK_BYTE:
            print("Handshake successful! Bootloader ready for packets.")
            return True
        else:
            print("Handshake failed at final ACK.")
            return False

    except Exception as e:
        print(f"Handshake error: {e}")
        return False

def main():
    sock = start_bluetooth_serial()
    if(perform_handshake(sock)):
        send_firmware_packets(sock)
    


main()