UNAME := $(shell uname -s)
ifeq ($(UNAME),Darwin)
 CC=clang++
    CC+=-D_XOPEN_SOURCE
    CC+=-I/opt/homebrew/opt/boost/include
    LIBFSCLIENT=libfs_client_macos.o
    LIBFSSERVER=libfs_server_macos.o
    BOOST_THREAD=boost_thread
   
else
    CC=g++-13
    LIBFSCLIENT=libfs_client.o
    LIBFSSERVER=libfs_server.o
    BOOST_THREAD=boost_thread
     
endif

CC+=-g -Wall -std=c++17 -Wno-deprecated-declarations

# List of source files for your file server
FS_SOURCES=fs_system.cpp

# Generate the names of the file server's object files
FS_OBJS=${FS_SOURCES:.cpp=.o}

all: fs test2

# Compile the file server and tag this compilation
#
# Remember to set the CPLUS_INCLUDE_PATH, LIBRARY_PATH, and LD_LIBRARY_PATH
# environment variables to include your Boost installation directory.
fs: ${FS_OBJS} ${LIBFSSERVER}
	./autotag.sh push ${FS_SOURCES}
	${CC} -o $@ $^ -l${BOOST_THREAD} -lboost_system -pthread -ldl
    
# Compile a client program
test2: test2.cpp ${LIBFSCLIENT}
	${CC} -o $@ $^

# Compile a client program
test5: test5.cpp ${LIBFSCLIENT}
	${CC} -o $@ $^


# Generic rules for compiling a source file to an object file
%.o: %.cpp
	${CC} -c $<
%.o: %.cc
	${CC} -c $<

clean:
	rm -f ${FS_OBJS} fs test2


