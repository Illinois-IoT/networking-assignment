OBJS_DIR = .objs

EXE_CLIENT = client
EXE_SERVER = server

OBJS_CLIENT = $(EXE_CLIENT).o
OBJS_SERVER = $(EXE_SERVER).o

# include dependencies
-include $(OBJS_DIR)/*.d

CC = clang
WARNINGS = -Wall -Wextra -Werror -Wno-error=unused-parameter -Wmissing-declarations -Wmissing-variable-declarations
CFLAGS_COMMON = $(WARNINGS) -std=c99 -MMD -MP -D_GNU_SOURCE
CFLAGS_DEBUG = $(CFLAGS_COMMON) -O0 -g -DDEBUG

.PHONY: client server
$(EXE_CLIENT):	$(OBJS_DIR)/$(OBJS_CLIENT)
		$(CC) $(CFLAGS_DEBUG) .objs/client.o -o client
.objs/client.o: 
		mkdir -p $(OBJS_DIR)	
		$(CC) $(CFLAGS_DEBUG) -c client.c -o $(OBJS_DIR)/$(OBJS_CLIENT)
$(EXE_SERVER):	$(OBJS_DIR)/$(OBJS_SERVER)
		$(CC) $(CFLAGS_DEBUG) .objs/server.o -o server
.objs/server.o: 
		mkdir -p $(OBJS_DIR)	
		$(CC) $(CFLAGS_DEBUG) -c server.c -o $(OBJS_DIR)/$(OBJS_SERVER)

.PHONY: clean
clean: 
	-rm -rf .objs client server client server


# clang -O0 -pthread -Wall -Wextra -g -std=c99 -D_GNU_SOURCE -DDEBUG client.c
