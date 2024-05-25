# Compiler
CC = gcc

# Source and object directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Installation directories
INSTALL_DIR_USER = ~/bin
INSTALL_DIR_SYSTEM = /usr/local/bin
CONF_DIR = /etc
CONF_DIR_USR = ~/.config/
CONF_D_DIR = $(CONF_DIR)/ps_pp.conf.d
CONF_U_DIR = $(CONF_DIR_USR)/ps_pp.conf.d

# Source files
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)

# Object files
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Executable name
EXECUTABLE = $(BIN_DIR)/ps_pp

# Main target
all: $(BIN_DIR) $(EXECUTABLE)

# Rule to create the bin directory
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Rule to create the obj directory
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Rule to build object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) -std=c17 -O2 -c $< -o $@

# Rule to build the executable
$(EXECUTABLE): $(OBJ_FILES)
	mkdir -p bin/
	$(CC) $^ -lncurses -o $@

clean:
	rm -f $(OBJ_DIR)/*.o $(EXECUTABLE)

install_user: $(EXECUTABLE)
	cp $(EXECUTABLE) $(INSTALL_DIR_USER)
	mkdir -p $(CONF_U_DIR)
	touch $(CONF_U_DIR)/ps_pp.conf
	echo -e "#color format: RGB(values 0-1000)\n\
#key format: single character([A-Z][a-z])\n\
#available variables:\n\n\
#COLOR_BG=(R,G,B)\n\
#COLOR_TEXT=(R,G,B)\n\
#COLOR_PRCNT_BAR=(R,G,B)\n\
#COLOR_SELECTPROC=(R,G,B)\n\
#COLOR_INFOTEXT=(R,G,B)\n\
#COLOR_INFO_SUC=(R,G,B)\n\
#COLOR_INFO_ERR=(R,G,B)\n\
#QUIT_KEY=K\n\
#KILL_KEY=K\n\
#IO_KEY=K\n\
#ID_KEY=K\n\
#CLASSIC_KEY=K\n\
#MEMSORT_KEY=K\n\
#CPUSORT_KEY=K\n\
#NORMALSORT_KEY=K\n\
#PRIOSORT_KEY=K\n\
#TIMESORT_KEY=K\n\
#TIMESORT_KEY_OLDEST=K\n\
#CUSTOM_KEY=K\n\
#TOGGLESUSPEND_KEY=K\n\
#BACK_KEY=K\n\
#HELP_KEY=K\n\
#END_KEY=K\n\n\
#uncomment for no logo\n\
#NOLOGO" > $(CONF_U_DIR)/ps_pp.conf
	cp -r themes $(CONF_U_DIR)
	rm -rf $(BIN_DIR)
	rm -rf $(OBJ_DIR)

install_system: $(EXECUTABLE)
	cp $(EXECUTABLE) $(INSTALL_DIR_SYSTEM)
	mkdir -p $(CONF_D_DIR)
	touch $(CONF_D_DIR)/ps_pp.conf
	echo -e "#color format: RGB(values 0-1000)\n\
#key format: single character([A-Z][a-z])\n\
#available variables:\n\n\
#COLOR_BG=(R,G,B)\n\
#COLOR_TEXT=(R,G,B)\n\
#COLOR_PRCNT_BAR=(R,G,B)\n\
#COLOR_SELECTPROC=(R,G,B)\n\
#COLOR_INFOTEXT=(R,G,B)\n\
#COLOR_INFO_SUC=(R,G,B)\n\
#COLOR_INFO_ERR=(R,G,B)\n\
#QUIT_KEY=K\n\
#KILL_KEY=K\n\
#IO_KEY=K\n\
#ID_KEY=K\n\
#CLASSIC_KEY=K\n\
#MEMSORT_KEY=K\n\
#CPUSORT_KEY=K\n\
#NORMALSORT_KEY=K\n\
#PRIOSORT_KEY=K\n\
#TIMESORT_KEY=K\n\
#TIMESORT_KEY_OLDEST=K\n\
#CUSTOM_KEY=K\n\
#TOGGLESUSPEND_KEY=K\n\
#BACK_KEY=K\n\
#HELP_KEY=K\n\
#END_KEY=K\n\n\
#uncomment for no logo\n\
#NOLOGO" > $(CONF_D_DIR)/ps_pp.conf
	cp -r themes $(CONF_D_DIR)
	rm -rf $(BIN_DIR)
	rm -rf $(OBJ_DIR)

uninstall_user: 
	rm -f $(INSTALL_DIR_USER)/ps_pp
	rm -rf $(CONF_U_DIR)

uninstall_system: 
	rm -f $(INSTALL_DIR_SYSTEM)/ps_pp
	rm -rf $(CONF_D_DIR)




.PHONY: all clean install_user install_system uninstall_user uninstall_system
