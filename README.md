# Real-time-chat-application-in-c
Using Socket programming and threading to make a real-time chatroom in c 

## Note-: 
1.  The above code is tested for MAC-OS, it should work with linux and windows but is untested. 
2.  Server and Client script can be on the same local machine or on different machines. If you want to run the scripts on     different machine make sure you have the public IP address and appropriate machine-level permissions (remote connections) on the machine where you run the server script.

## Usage server script 
```bash

./server. ## script will run on port 80 by default
./server 90 ## run the script on port 90

```
## Usage client script 
```bash
./client [-h] [-a] [-p] [-u]
 -h           show this help message and exit [**optional**]
 -a           IP address of the server ["localhost" if running on local machine] [**required**]
 -p           port number of the server [**required**]
 -u           username of the person [**required**]
 
 
```
