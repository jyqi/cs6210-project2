UNAME = $(shell uname)
# the compiler
CC = gcc

# compiler flags:
# -g adds debugging information to the executable file
# -Wall turns on most, but not all, compiler warnings
CFLAGS = -g -O3 -fno-tree-vectorize -Werror -Wall -Wextra -pedantic-errors -Wformat=2 -Wno-import -Wimplicit -Wmain -Wchar-subscripts -Wsequence-point -Wmissing-braces -Wparentheses -Winit-self -Wswitch-enum -Wstrict-aliasing=2 -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wdisabled-optimization -Wunused-macros -Wno-unused 

# Libraries required for linking; MAC doesn't require linking to librt
ifeq ($(UNAME), Linux)
	LIBS = -lrt -pthread
endif
ifeq ($(UNAME), Darwin)
	LIBS = 

endif
# Required source files
SVC_SRC = src/service.c src/caesar.c src/errors.c
CLIENT_SRC = src/client.c src/service_api.c src/errors.c
OBJ = $(SRC:.c=.o)

service:
	$(CC) $(CFLAGS) $(SVC_SRC) -o bin/caesar_service $(LIBS)

client:
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o bin/caesar_client $(LIBS)

clean:
	@rm bin/* src/*.o
