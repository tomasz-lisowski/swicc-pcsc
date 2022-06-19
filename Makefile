SEMVER_MAJOR:=0
SEMVER_MINOR:=0
SEMVER_PATCH:=1
SEMVER_STR:=$(SEMVER_MAJOR).$(SEMVER_MINOR).$(SEMVER_PATCH)

DIR_LIB:=lib
include $(DIR_LIB)/make-pal/pal.mak
DIR_SRC:=src
DIR_INCLUDE:=include
DIR_BUILD:=build
CC:=gcc
LD:=ld

# Information needed for installation.
DIR_READER_CONFD:=/etc/reader.conf.d
DIR_PCSC_SERIAL:=/usr/lib/pcsc/drivers/serial
DIR_PCSC_DEV:=/dev/null

MAIN_NAME:=swicc-drv-ifd
MAIN_SRC:=$(wildcard $(DIR_SRC)/*.c)
MAIN_OBJ:=$(MAIN_SRC:$(DIR_SRC)/%.c=$(DIR_BUILD)/%.o)
MAIN_DEP:=$(MAIN_OBJ:%.o=%.d)
MAIN_CC_FLAGS:=\
	-W \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-unused-parameter \
	-Wconversion \
	-Wshadow \
	-fPIC \
	-O2 \
	-I$(DIR_INCLUDE) \
	-I$(DIR_LIB)/swicc/include \
	$(shell pkg-config --cflags-only-I libpcsclite) \
	-DDIR_PCSC_DEV=\"$(DIR_PCSC_DEV)\"
MAIN_LD_FLAGS:=\
	-O \
	-Bdynamic \
	-Bshareable
EXT_LIB_SHARED:=$(EXT_LIB_SHARED).$(SEMVER_STR)

all: main
.PHONY: all

main: MAIN_LD_FLAGS+=-s
main: $(DIR_BUILD)/$(LIB_PREFIX)$(MAIN_NAME).$(EXT_LIB_SHARED) $(DIR_BUILD)/reader.conf
main-dbg: MAIN_CC_FLAGS+=-g -DDEBUG -fsanitize=address
main-dbg: main
.PHONY: main main-dbg

install: $(DIR_BUILD)/$(LIB_PREFIX)$(MAIN_NAME).$(EXT_LIB_SHARED) $(DIR_BUILD)/reader.conf
ifeq ($(OS),Windows_NT)
	$(call pal_clrtxt, $(CLR_RED), Installing is only supported on Linux.)
	exit 1
else
	$(call pal_cp,$(DIR_BUILD)/reader.conf,$(DIR_READER_CONFD)/$(LIB_PREFIX)$(MAIN_NAME))
	$(call pal_cp,$(DIR_BUILD)/$(LIB_PREFIX)$(MAIN_NAME).$(EXT_LIB_SHARED),$(DIR_PCSC_SERIAL)/$(LIB_PREFIX)$(MAIN_NAME).$(EXT_LIB_SHARED))
endif
uninstall:
	$(call pal_rm,$(DIR_READER_CONFD)/$(LIB_PREFIX)$(MAIN_NAME))
	$(call pal_rm,$(DIR_PCSC_SERIAL)/$(LIB_PREFIX)$(MAIN_NAME).$(EXT_LIB_SHARED))
.PHONY: install uninstall

# Create the dynamic lib.
$(DIR_BUILD)/$(LIB_PREFIX)$(MAIN_NAME).$(EXT_LIB_SHARED): $(DIR_BUILD) $(MAIN_OBJ)
	$(LD) -o $(@) $(MAIN_LD_FLAGS) $(MAIN_OBJ)

$(DIR_BUILD)/reader.conf: $(DIR_BUILD)
	echo "\
	FRIENDLYNAME \"swICC PC/SC IFD Driver v$(SEMVER_STR)\"\
	\nDEVICENAME   $(DIR_PCSC_DEV)\
	\nLIBPATH      $(DIR_PCSC_SERIAL)/$(LIB_PREFIX)$(MAIN_NAME).$(EXT_LIB_SHARED)"\
	 > $(@)

# Compile source files to object files.
$(DIR_BUILD)/%.o: $(DIR_SRC)/%.c
	$(CC) -o $(@) $(MAIN_CC_FLAGS) -c -MMD $(<)

# Recompile source files after a header they include changes.
-include $(MAIN_DEP)

$(DIR_BUILD):
	$(call pal_mkdir,$(@))
clean:
	$(call pal_rmdir,$(DIR_BUILD))
.PHONY: clean
