.SUFFIXES:
.SUFFIXES:	.cpp .o

INC_PATH 	= -I. \
		-I/usr/include \
		-I/../../WebSocket \
		-I/../../OpenNI \
		-I/../../NiTE \
		-I/../../Boost

LIB_PATH	= -L/usr/lib \
		-L../../OpenNI/lib \
		-L../../NiTE/lib \
		-L../../Boost/lib

LIBS =	-lboost_program_options \
				-lboost_system \
				-lOpenNI2 \
				-lNiTE2
				
COMDEF =	-std=c++0x \
					-Dlinux

all: WebNIServer

WebNIServer: NIModule.o main.o
	gcc -o $@ $^ $(LIB_PATH) $(LIBS) -lm

.cpp.o:
	gcc &(COMDEF) $(INC_PATH) -c $<

MAKE_CLEAN = $(MAKE) "clean"

clean:
	$(RM) *.o $(TARGET)
