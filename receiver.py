import socket

esp32_ip = "192.168.137.63"  #NOTE 2 SELF: MAY need to change if network changes
#esp32_ip = "192.168.2.x"

esp32_port = 6000 # (i.e. sendPort)           

receivingSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
receivingSock.bind(("0.0.0.0", 5005))

sendingSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print("Listening on Laptop UDP port 5005 and sending to ESP32 at", esp32_ip)

while True:

    #RECEIVE DATA
    data, addr = receivingSock.recvfrom(1024)
    valueReceived = int(data.decode())
    print("Received from ESP32:", valueReceived)

    #PROCESS DATA
    testCalculatedValue = valueReceived * 2

    #SEND DATA BACK
    outboundMessage = str(testCalculatedValue).encode()
    sendingSock.sendto(outboundMessage, (esp32_ip, esp32_port))
    print("Sent to ESP32:", testCalculatedValue)


    


