CPP       = /opt/am4372/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-g++
CC        = /opt/am4372/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc

OBJ       = \
	main.o

LINKOBJ   = $(OBJ) -pthread -lmodbus

LIBS      = -L /opt/am4372/lib/lib/
INCS      =
BIN       = test_bin

DEFINES   =
CFLAGS    = $(INCS) $(DEFINES) -pthread
LINK      = $(CPP)

.PHONY: all all-before all-after clean clean-custom
all: all-before $(BIN) all-after

clean: clean-custom
	$(RM) $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(LINK) $(LINKOBJ) -o $(BIN) $(LIBS)

%.o : %.cpp
	$(CPP) $(CFLAGS) -c -o $@ $<