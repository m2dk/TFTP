"""Cliente_Tftp.
Usage:
  Cliente_Tftp.py get <filename> --server=<ip> --port=<puerto> [[-s|-b ] --mode=<mode>]
  Cliente_Tftp.py put <filename> --server=<ip> --port=<puerto> [--cosa=<distintivo>]
  Cliente_Tftp.py (-h | --help)

Options:
  -h --help     Show this screen.
  -s            Use python struct to build request.
  -b            Use python bytearray to build request.
  --mode=<mode> TFTP transfer mode : "netascii"
"""

from docopt import docopt
import socket
from struct import pack

"""opcode  operation
    1       Read Request (RRQ)
    2       Write request (WRQ)
    3       Data(DATA)
    4       Acknowledgment (ACK)
    5       Error (ERROR)

>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    2 bytes     String      1 byte      String      1 byte
    opcode      Filename        0       Mode            0

RRQ/WRQ packet

>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    2 bytes     2 bytes
    Opcode      Block #
ACK packet

>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    2 bytes     2 bytes     n bytes
    OPcode      BLock #     Data
DATA packet
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

TFTP Formats
        Type       Op#             Format without header

        2 bytes     String      1byte       String     1byte  
RRQ/    01/02       Filename        0       Mode        0
WRQ

        2bytes      2bytes      nbytes
DATA    03          Block#      Data

        2bytes      2bytes
ACK       04       block#

        2bytes      2bytes      string      1bytes
ERROR   05          ErrorCode   ErrMsg      0

De aqui en adelante esta la implementacion del Clietne TFTP
"""

TERMINATING_DATA_LENGTH = 516
TFTP_TRANSFER_MODE = b'netascii'

TFTP_OPCODES = {
    'unknown': 0,
    'read' : 1,#RRQ
    'write' : 1,#WRQ
    'data': 3,#DATA
    'ack': 4, #ACKNOWLEDGMENT
    'error': 5} #ERROR

TFTP_MODES = {
    'unknown' :0,
    'netascii':1}
    #'octet':2,
    #'mail':3}

#CREACION DEL SOCKET UDP PARA EL ENVIO DE LOS PAQUETES

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_address = ('localhost',69)

def send_rq(filename, mode):
    """
    Funcion para crear paquetes de la forma RRQ/WRQ
    """
    #inicializo el arreglo que mandare por el socket y recibira mi servidor
    request = bytearray()
    #Los primeros dos bytes de el opcode para el read request
    request.append(0)
    request.append(1)
    #creo el nombre del archivo que voy a pedir con codificacion en bytes utf-8
    filename = bytearray(filename.encode('utf-8'))
    request += filename
    #Agrego el byte de terminacion
    request.append(0)
    #hago que el modo ser de transferencia
    form = bytearray(bytes(mode.encode('utf-8')))
    request +=form
    #agrego el ultimo byte
    request.append(0)

    print("Request {}".format(request))

    sent = sock.sendto(request, server_address)

def send_rq_struct(filename, mode):
    
    formatter = '>h{}sB{}sB'  # { > - Big Endian, h - short , s - char, B - 1 byte }
    formatter = formatter.format(len(filename), len('netascii'))
    print(formatter)  # final format '>h8sB8sB'
    request = pack(formatter, TFTP_OPCODES['read'], bytes(filename, 'utf-8'), 0, bytes(mode, 'utf-8'), 0)

    print("Request {}")
    sent = sock.sendto(request, server_address)

def send_ack(ack_data, server):
    ack = bytearray(ack_data)
    ack[0] = 0
    ack[1] = TFTP_OPCODES['ack']
    print(ack)
    sock.sendto(ack,server)

def server_error(data):
    
    opcode = data[:2]
    return int.from_bytes(opcode,byteorder='big') == TFTP_OPCODES['error']

server_error_msg = {
    0: "Not defined, see error message (if any).",
    1: "File not found.",
    2: "Access violation.",
    3: "Disk full or allocation exceeded.",
    4: "Illegal TFTP operation.",
    5: "Unknown transfer ID.",
    6: "File already exists.",
    7: "No such user."
}


def main():

    arguments = docopt(__doc__)
    filename  = arguments['<filename>']
    ##print(arguments)
    if arguments ['--cosa'] is not None:
        return
    if arguments ['--mode'] is not None:
        mode = arguments['--mode']
        if mode.lower() not in TFTP_MODES.keys():
            print("Unknown mode - defaulting to netascii")
            mode = "netascii"
    else:
        mode = "netascii"

    if arguments['-s']:
        send_rq_struct(filename, mode)
    elif arguments['-b']:
        send_rq(filename,mode)
    else:
        send_rq_struct(filename,mode)
    file = open(filename,"wb")
    while True:
        data,server = sock.recvfrom(600)

        if server_error(data):
            error_code = int.from_bytes(data[2:4],byteorder='big')
            print(server_error_msg[error_code])
            break
        send_ack(data[0:4], server)
        content = data[4:]

        file.write(content)

        if len(data) < TERMINATING_DATA_LENGTH:
            break

if __name__ == '__main__':
    main()