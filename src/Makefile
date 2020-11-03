TARGET = xmbctrl
OBJS = main.o utils.o stubkk.o exports.o conf.o

INCDIR = $(PRO_HOME)/include
CFLAGS = -Os -G0 -Wall -fshort-wchar -fno-pic -mno-check-zero-division
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

ifeq ($(CONFIG_620), 1)
CFLAGS += -DCONFIG_620=1
OBJS += scePaf_620.o vsh_620.o sceVshCommonGui_620.o
PSP_FW_VERSION=620
endif

ifeq ($(CONFIG_635), 1)
CFLAGS += -DCONFIG_635=1
OBJS += scePaf_639.o vsh_639.o sceVshCommonGui_639.o
PSP_FW_VERSION=635
endif

ifeq ($(CONFIG_639), 1)
CFLAGS += -DCONFIG_639=1
OBJS += scePaf_639.o vsh_639.o sceVshCommonGui_639.o
PSP_FW_VERSION=639
endif

BUILD_PRX = 1
PRX_EXPORTS = exports.exp

USE_PSPSDK_LIBS = 1
USE_PSPSDK_LIBC = 1

LIBS = -lpspsystemctrl_user -lpspkubridge -lpspvshctrl

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak

