## TODO: for solaris, need -lsocket and -lnsl
CC = g++ 

BIN = ../bin
LIB = ../lib
RPC_BINS = $(BIN)/rpc.o $(BIN)/message.o $(BIN)/connection.o
SERVER_BINS = $(BIN)/message.o $(BIN)/connection.o $(BIN)/server_functions.o $(BIN)/server_function_skels.o 
BINDER_BINS = $(BIN)/message.o $(BIN)/connection.o

all: rpc client server binder

message: 
	$(CC) -c -o $(BIN)/message.o message.cpp

connection:
	$(CC) -c -o $(BIN)/connection.o connection.cpp
	
server_functions:
	$(CC) -c -o $(BIN)/server_functions.o server_functions.cpp
	$(CC) -c -o $(BIN)/server_function_skels.o server_function_skels.cpp

rpc: message connection
	$(CC) -c -o $(BIN)/rpc.o rpc.cpp
	ar rc $(LIB)/librpc.a $(RPC_BINS)
	ranlib $(LIB)/librpc.a
	
client: rpc
	$(CC) -o $(BIN)/client client.cpp $(LIB)/librpc.a

server: message connection server_functions rpc
	$(CC) -o $(BIN)/server $(SERVER_BINS) server.cpp $(LIB)/librpc.a
	
binder: message connection
	$(CC) -o $(BIN)/binder $(BINDER_BINS) binder.cpp
	
clean:
	rm $(BIN)/*