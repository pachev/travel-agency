# Travel Agency

Travel Agency client/server applications
Forked from https://gitlab.com/openhid/travel-agency 

## Build
```
git clone https://github.com/pachev/travel-agency.git

make clean && make
```

## travel_server
 usage: `travel_server ip_address start_port no_ports data_file output_file`

 `$ travel_server 127.0.0.1 9000 5 in.txt out.txt`

### Commands
#### EXIT 
Exits the server.

#### LIST 
List all users connected.

#### LIST_CHAT 
List all users in chat.

#### WRITE `filename`
Write filename with current data.



## travel_client
usage: `travel_client ip_address port`

`$ travel_client 127.0.0.1 9000`

### Commands
#### LOGON `username`

#### QUERY `flight`
Returns number of seats available on flight.
usage: `QUERY MIAMI-ORL`

#### RESERVE `flight` `seats`
Reserves number of seats on flight.

`$ RESERVE MIAMI-ORL 4`

#### LIST 
Lists all flights and their available seats.

#### LIST_AVAILABLE
Lists only flights with available seats.

#### LIST_AVAILABLE `N`
List the first N flights with available seats.

`$ LIST_AVAILABLE 4`

#### ENTER_CHAT `nickname`
Opens chat with nickname.

#### LOGOFF
