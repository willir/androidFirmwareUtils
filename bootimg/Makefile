
CC = gcc
AR = ar rcv
STRIP = strip

ifeq ($(windir),)
EXE =
RM = rm -rf
CP = cp -a
MKDIR = mkdir -p
else
EXE = .exe
RM = del
CP = copy
MKDIR = mkdir
endif

OBJ_DIR = obj/
BIN_DIR = bin/

MKBOOTIMG = $(BIN_DIR)mkbootimg$(EXE)
UNPACKBOOTIMG = $(BIN_DIR)unpackbootimg$(EXE)

LIB_MINCRYPT_DIR = libmincrypt/
INSTALL_DIR = $(HOME)/bin/

all:mk_dirs $(BIN_DIR)libmincrypt.a $(MKBOOTIMG) $(UNPACKBOOTIMG)

$(BIN_DIR)libmincrypt.a: mk_dirs
	make -C libmincrypt
	$(CP) $(LIB_MINCRYPT_DIR)libmincrypt.a $@

$(MKBOOTIMG):$(OBJ_DIR)mkbootimg.o
	$(CC) -o $@ $^ -L$(BIN_DIR) -lmincrypt -static
	$(STRIP) $@

$(OBJ_DIR)mkbootimg.o:mkbootimg.c
	$(CC) -o $@ -c $< -I.


$(UNPACKBOOTIMG):$(OBJ_DIR)unpackbootimg.o
	$(CC) -o $@ $^ -static
	$(STRIP) $@

$(OBJ_DIR)unpackbootimg.o:unpackbootimg.c
	$(CC) -o $@ -c $< 

.PHONY: mk_dirs
mk_dirs:
	$(MKDIR) $(OBJ_DIR) $(BIN_DIR)

.PHONY: clean
clean:
	$(RM) $(OBJ_DIR) $(BIN_DIR) Makefile.~ libmincrypt.a
	make -C libmincrypt clean

.PHONY: install
install:
	chmod a+x $(UNPACKBOOTIMG) $(MKBOOTIMG)
	$(CP) $(UNPACKBOOTIMG) $(MKBOOTIMG) $(INSTALL_DIR)

