#
# Copyright (c) 2020 jdeokkim
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

.PHONY: clean

LK_PATH := leko-demo

BIN_PATH := $(LK_PATH)/bin
LIB_PATH := $(LK_PATH)/lib
SRC_PATH := $(LK_PATH)/src

RL_PATH := $(LIB_PATH)/raylib
RG_PATH := $(LIB_PATH)/raygui

CC := gcc
CFLAGS := -I$(RG_PATH) -std=c99 -O2 -D_DEFAULT_SOURCE
LDFLAGS := -g
LDLIBS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

INPUT := $(SRC_PATH)/btn.c $(SRC_PATH)/core.c $(SRC_PATH)/game.c $(SRC_PATH)/main.c
OUTPUT := ld_linux

_: 
	mkdir -p $(BIN_PATH)
	$(CC) $(INPUT) -o $(BIN_PATH)/$(OUTPUT) $(CFLAGS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(BIN_PATH)/$(OUTPUT)