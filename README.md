# Travel Agency

Travel Agency client/server application
Forked from https://gitlab.com/openhid/travel-agency 

## Description
This is a travel agency. The agency can  serve up to 10000 flights, all are round trip with different 
amount of seats. The agency also provide a chat room for customers.

## Build
```shell
git clone https://github.com/pachev/travel-agency.git
git submodule update --init --recursive

make clean && make
```

## travel_server
Usage: `travel_server ip_address start_port no_ports data_file output_file`

Example:  `travel_server localhost 9000 5 in.txt out.txt`

## travel_client
Usage: `travel_server ip_address start_port no_ports data_file output_file`

Example `travel_client localhost 9000`

### Commands for SERVER
#### EXIT 
Exits the server.

#### LIST 
List all users connected.

#### LIST_CHAT 
List all users in chat.

#### WRITE 
Write output file with current data.


### Commands for CLIENT

#### LOGON `username`

#### QUERY `flight`
Returns number of seats available on flight.
usage: `QUERY MIA-ORL`

#### RESERVE `flight` `seats`
Reserves number of seats on flight.

`>>> RESERVE MIAMI-ORL 4`

#### RETURN `flight` `seats`
Returns a number of seats on a flight (don't cheat the system, it knows).

`>>> RETURN MIAMI-ORL 4`

#### LIST 
Lists all flights and their available seats.

#### LIST `N`
Lists `N` flights and their available seats.

#### LIST_AVAILABLE
Lists only flights with available seats.

#### LIST_AVAILABLE `N`
List the first N flights with available seats.

`>>> LIST_AVAILABLE 4`

### LOGOFF
Logs users off from client and disconnects from server. 

#### ENTER_CHAT `nickname`
Opens chat with nickname.


### Chat Commands

#### TEXT `Message to be sent`
Sends a text message that will be seen by all users in the chat.

#### LIST 
List all users connected that are online in the chat. Also returns a count. 

#### LIST_ALL 
List all users, including those offline.  Also returns a count.

#### LIST_OFFLINE 
List all the users that are offline.  Also returns a count.
