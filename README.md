# TFTP
Simple implementation of a TFTP server in C and a TFTP client in Python. The client includes:

* Get ~ Command that lets you download files from the server.
* Put ~ Command that lets you upload files to the server.

## Dependencies
* Python ~ docopt 0.6.2

## Usage
* Server

Compile and execute the server:

```
cd server
make
sudo ./tftps [directory]
```
  
* Client
Run

```
python3 tftpc.py <get|put> <ip_addr>
```

You can also run

```
python3 tftpc.py -h
```

for more information about usage.
