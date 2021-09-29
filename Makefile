######################################
# target
######################################
TARGET = main

######################################
# building variables
######################################
# debug build?
DEBUG = 1
# optimization
OPT = -Og

#######################################
# paths
#######################################
# Build path
BUILD_DIR = build
CLI_DIR = build/cli

######################################
# source
######################################
# C sources
C_SOURCES =  \
src/main.c \
drivers/axi-bram.c

# C includes
C_INCLUDES =  \
-Isrc \
-Idrivers

# cli sources
CLI_SOURCES = \
cli/axi-bram.c \
cli/test.c

#######################################
# binaries
#######################################
PREFIX =
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
else
CC = $(PREFIX)gcc
endif

#######################################
# FLAGS
#######################################
# compile gcc flags
CFLAGS = $(C_INCLUDES) $(OPT) -Wall -Wextra
LDFLAGS = -Wall -g

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif


.PHONY:clean all cli

#######################################
# build the application
#######################################
# list of objects
C_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

# default action: build all
all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET): $(C_OBJECTS) Makefile
	$(CC) $(C_OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR):
	mkdir -p $@

#######################################
# build the cli application
#######################################
CLI_OBJECTS = $(patsubst %.c, %, $(CLI_SOURCES))

cli: $(CLI_OBJECTS)

$(CLI_OBJECTS):%:%.c | $(CLI_DIR)
	$(CC) $(LDFLAGS) $< -o $(BUILD_DIR)/$@

$(CLI_DIR):
	mkdir -p $@

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)
