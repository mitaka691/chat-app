CC = gcc
CFLAGS = -Wall -Wextra -g

CRYPTO_LIBS = -lssl -lcrypto
THREAD_LIBS = -lpthread

CLIENT_SRC = client.c crypto.c net.c
SERVER_SRC = server.c net.c

CLIENT = client
SERVER = server

all: $(CLIENT) $(SERVER)

$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT) $(CRYPTO_LIBS) $(THREAD_LIBS)

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER) $(THREAD_LIBS)

clean:
	rm -f $(CLIENT) $(SERVER)

rebuild: clean all
