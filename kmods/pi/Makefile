obj-m += pi.o

ccflags-y += -DDEBUG -std=gnu99 -g

MK_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(MK_DIR)../kmod-common.mk

