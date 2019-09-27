# TFTP
A Simple implementation of a TFTP server in C and a client in Python. The client includes:

* Get ~ Command that led you download files from the server.
* Put ~ Command that led you upload files to the server.

## Dependencies
* Python ~ docopt 0.6.2

##Usage
* Server
Compile the server with the makefile provided
execute with `./tftp ./<directory>`
  
* Client
Just run `./Cliente_Tftp.py` in your terminal with `<get|put> <filename> --server=<server IP> --port=<69 by default>`.
you can also run `./Cliente_Tftp.py` in your terminal with `h` to get aditional help.
