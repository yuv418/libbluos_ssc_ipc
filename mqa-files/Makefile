ARCH ?= $(shell uname -m)
-include $(or $(CONFIG),$(ARCH)).mk

MAKEFLAGS += -r

O ?= $(ARCH)
override O := $(O:%=$(O:%/=%)/)

SYSROOT = $(addprefix --sysroot=,$(ROOT))

CC = $(CROSS_COMPILE)gcc $(SYSROOT)
AR = $(CROSS_COMPILE)ar

CPPFLAGS += -MMD
CFLAGS = -O2 -g -Wall $(CPUFLAGS)
LDFLAGS += -Wl,--as-needed
LDLIBS += -lsndfile $(LDLIBS-$(ARCH))

ALL = mqbgen mqbscan

ALL-arm += mqadec mqarender mqaplay mqaplay_ipc
LIBOBJ-arm += bluos_ssc.o

ALL += $(ALL-$(ARCH))
ALL := $(ALL:%=$(O)%)
all: $(ALL)

LIBOBJ = bits.o mqa-common.o blake2s-ref.o mqa-keys.o sfbits.o mqascan.o
LIBOBJ += $(LIBOBJ-$(ARCH))
LIBOBJ := $(LIBOBJ:%=$(O)%)

LIB = $(O)libmqa.a

$(LIB): $(LIBOBJ)
	$(RM) $@
	$(AR) rcs $@ $^

$(ALL): $(LIB)

$(O)mqadec: LDLIBS += -ldl
$(O)mqaplay: LDLIBS += -ldl
$(O)mqaplay_ipc: LDLIBS += -ldl
$(O)mqarender: LDLIBS += -ldl
$(O)mqascan: LDLIBS += -lgmp

$(O)%.o: %.c | $(O)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(O)%: $(O)%.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(O):
	@mkdir -p $@

.PHONY: all
.SECONDARY:

-include $(ALL:%=%.d) $(OBJ:.o=.d)
