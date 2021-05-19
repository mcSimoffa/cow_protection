import select, socket
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setblocking(0)
server.bind(('', 9090))
server.listen(5)
inputs = [server]
outputs = []
message_queues = {}

while inputs:
    readable, writable, exceptional = select.select(inputs, outputs, inputs)
    for s in readable:
        if s is server:
            connection, client_address = s.accept()
            connection.setblocking(0)
            inputs.append(connection)
            print('\n', s.getsockname(), ' success connected top client')
        else:
            data = s.recv(1024)
            if data:
                if message_queues.get(s, None):
                    message_queues[s].append(data)
                else:
                    message_queues[s]=[data]
                         
                print('data receive_ ',data, ' from ', (s.getpeername())[0])
                if s not in outputs:
                    outputs.append(s)
            else:
                print('0 bytes receved')
                if s in outputs:
                    outputs.remove(s)
                inputs.remove(s)
                s.close()
                del message_queues[s]

    for s in writable:
        print('writable')
        next_msg = message_queues.get(s, None)
        if len(next_msg):
            temp=next_msg.pop(0).decode('utf-8').upper()
            s.send(temp.encode())
        else:
            outputs.remove(s)

    for s in exceptional:
        print('dissapear client')
        inputs.remove(s)
        if s in outputs:
            outputs.remove(s)
        s.close()
        del message_queues[s]
