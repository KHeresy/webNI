.SUFFIXES:
.SUFFIXES:	.cpp .o

INC_PATH 	= -I. \
		-I/usr/include \
		-I/Build/heresy/E3D_V3/external_linux/include \
		-I/Build/heresy/E3D_V3/external/include/OpenNI \
		-I/Build/heresy/E3D_V3/external/include/NiTE \
		-I/Build/heresy/E3D_V3/external/include/

LIB_PATH	= -L/usr/lib \
		-L/opt/cuda/lib64 \
		-L../../../lib \
		-L/Build/heresy/E3D_V3/external_linux/lib

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
