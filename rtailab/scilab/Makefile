INST_DIR = /usr/local
BIN_DIR  = $(INST_DIR)/bin
RTAI_DIR = $(INST_DIR)/scicos

install: 
	install -d $(RTAI_DIR)
	install -d $(RTAI_DIR)/include
	install *.c $(RTAI_DIR)
	install devices/*.a $(RTAI_DIR)
	install devices/*.h $(RTAI_DIR)/include

	install -d $(BIN_DIR)
	install utility/gen_io $(BIN_DIR)
	install utility/genconf_io $(BIN_DIR)
	install utility/gen_dev $(BIN_DIR)



