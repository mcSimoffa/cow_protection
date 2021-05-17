import socket
import threading

def send_task():
    while True:
        strToSend = input("String to send:\r\n")
        conn.send(strToSend.encode()) 

def next_wait_task():
    while True:
        strToSend = input("String to send:\r\n")
        conn.send(strToSend.encode())             

sock = socket.socket()
sock.bind(('', 9090))
sock.listen(1)
conn, addr = sock.accept()

print ('connected:', addr)
thread = threading.Thread(target=send_task)
thread.start()

while True:
    data = conn.recv(1024)
    if (len(data)>0):
        print(data)    
 