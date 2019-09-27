#!/usr/bin/env python3

"""Cliente tftp.
Usage:
  tftpc.py get <filename> <ip> --port=<puerto> [[-s|-b ] --mode=<mode>]
  tftpc.py put <filename> <ip> --port=<puerto> [--mode=<mode>]
  tftpc.py (-h | --help)

Options:
  -h --help     Show this screen.
  -s            Use python struct to build request.
  -b            Use python bytearray to build request.
  --mode=<mode> TFTP transfer mode : "netascii"
"""

from docopt import docopt
import socket
from struct import pack
import os

TERMINATING_DATA_LENGTH = 516
TFTP_TRANSFER_MODE = b'octet'

TFTP_OPCODES = {
    'unknown': 0,
    'read' : 1,#RRQ
    'write' : 2,#WRQ
    'data': 3,#DATA
    'ack': 4, #ACKNOWLEDGMENT
    'error': 5} #ERROR

TFTP_MODES = {
    'unknown' :0,
    'netascii':1,
    'octet':2,
    'mail':3}

#CREACION DEL SOCKET UDP PARA EL ENVIO DE LOS PAQUETES
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.settimeout(3000)

"""
        2 bytes     String      1byte       String     1byte  
RRQ/    01/02       Filename        0       Mode        0
WRQ
"""
def send_rq(filename,demand, mode,server_address):
    """
    Funcion para crear paquetes de la forma RRQ/WRQ
    """
    #inicializo el arreglo que mandare por el socket y recibira mi servidor
    request = bytearray()
    #Los primeros dos bytes del opcode para el read request
    request.append(0)
    request.append(demand)
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
    
    
"""
        2bytes      2bytes      nbytes
DATA    03          Block#      Data   
""" 
def make_data_packet(numblock, data,server):
    packet = bytearray()
    #create a bytearray to send
    packet.append(0)
    packet.append(3)
    #data_packet opcode
    packet+=(numblock).to_bytes(2,byteorder='big')
    #agrego el numero del paquete
    info = bytearray(data)
    packet += info
    #Agrego la informacion de la data
    #print("Sending data packet {}".format(packet))
    
    sock.sendto(packet, server)#Envia la informacion
    

"""
    2 bytes     2 bytes
    Opcode      Block #
ACK packet
"""
def send_ack(ack_data, server):#Envia ACK
    ack = bytearray(ack_data)
    ack[0] = 0
    ack[1] = TFTP_OPCODES['ack']
    print(ack)
    sock.sendto(ack,server)
    

"""
        2bytes      2bytes      string      1bytes
ERROR   05          ErrorCode   ErrMsg      0
"""
def server_error(data):#funcion para imprimir errores dependiendo de lo que mande el servidor
    opcode = data[:2]
    return int.from_bytes(opcode,byteorder='big') == TFTP_OPCODES['error']

"""
Informacion para poder imprimir errores
"""
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

#Main program
def main():
    
    arguments = docopt(__doc__)#Argumento
    filename  = arguments['<filename>']#nombre del archivo
    server_address = (arguments['<ip>'],69)#first set
    if arguments['get']:#para recibir archivos
        if arguments ['--mode'] is not None:
            mode = arguments['--mode']
            if mode.lower() not in TFTP_MODES.keys():
                print("Unknown mode - defaulting to octet")
                mode = "octet"
        else:
            mode = "octet"#Octet por defecto

        request = 1#Solo para decir que voy a recibir un archivo
        send_rq(filename,request,mode,server_address)
        file = open(filename,"wb")
        while True:
            data,server = sock.recvfrom(600)#Escucho el socket

            if server_error(data):
                error_code = int.from_bytes(data[2:4],byteorder='big')#Verifico cual es el error
                print(server_error_msg[error_code])#Imprimo con la tabla de arriba
                break
            send_ack(data[0:4], server)#envio el ACK
            content = data[4:]#Obtengo el contenido

            file.write(content)#Escribo en el archivo

            if len(data) < TERMINATING_DATA_LENGTH:#Si es menor a 516 termino
                break      
    else:#Para enviar archivos
        if arguments ['--mode'] is not None:
            mode = arguments['--mode']
            if mode.lower() not in TFTP_MODES.keys():
                print("Unknown mode - defaulting to octet")
                mode = "octet"
        else:
            mode = "octet"
        request = 2 # para decir que voy sa escribir
        send_rq(filename,request,mode,server_address)#Envio el request para escribir
        data,server = sock.recvfrom(600)#Escucho por el ack y por el servidor
        file = open(filename,"rb")#abro el archivo
        num_packet = 1 #numero del primer paquete
        while True:
            """
                Leo 512 del archivo
                saco el tamano del datapacket
                creo el paquete
                verifico si la cantidad que leo es menor a 512
                    de ser asi termina
                escucha el puerto por el ack
                si no hay nada en la data reenvia
                aumenta el numero del paquete
                mueve el puntero del archivo
                repite
            """
            data_packet = file.read(512)
            size = len(data_packet)
            make_data_packet(num_packet,data_packet,server)
            if(size <  512):
                #print("corte")
                break
            data,server = sock.recvfrom(600)
            #print(data)
            if len(data) == 0:
                #print("reenvio")
                make_data_packet(num_packet,data_packet,server)
            else:
                num_packet = num_packet + 1
                #print(num_packet)
                file.seek(0,1)
                #print(file.seekable())
                
        #envia un paquete 0
        make_data_packet(num_packet,0,server)
        #cierra el archivo
        file.close()

if __name__ == '__main__':
    main()