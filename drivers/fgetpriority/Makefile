# NOTE: unity build. single unit.

obj-$(CONFIG_FGETPRIORITY) += fgetpriority.o
fgetpriority-y := main.o

# CFLAGS_main.o += -Weverything
# CFLAGS_main.o += $(call cc-option, -Wno-unsafe-buffer-usage)

# warnings are warnings.
CFLAGS_REMOVE_main.o += -Werror

CFLAGS_main.o += -Wall
CFLAGS_main.o += $(call cc-option, -Wno-declaration-after-statement)
CFLAGS_main.o += $(call cc-option, -Wno-deprecated-octal-literals)
CFLAGS_main.o += $(call cc-option, -Wno-int-to-pointer-cast)
CFLAGS_main.o += $(call cc-option, -Wno-pointer-to-int-cast)
CFLAGS_main.o += $(call cc-option, -Wno-int-conversion)
CFLAGS_main.o += $(call cc-option, -Wno-unused-variable)
CFLAGS_main.o += $(call cc-option, -Wno-unused-function)
CFLAGS_main.o += $(call cc-option, -Wno-format)
CFLAGS_main.o += $(call cc-option, -Wno-c2x-extensions)

# enforce atleast gnu11
CFLAGS_main.o += $(call cc-option, -std=gnu11)
CFLAGS_main.o += $(call cc-option, -std=gnu17)
CFLAGS_main.o += $(call cc-option, -std=gnu23)
CFLAGS_main.o += $(call cc-option, -std=gnu2y)

ifeq ($(CONFIG_CC_OPTIMIZE_FOR_SIZE),y)
CFLAGS_REMOVE_main.o += -Oz
CFLAGS_REMOVE_main.o += -Os
CFLAGS_main.o += -O2
endif

CFLAGS_main.o += $(call cc-option, -Wframe-larger-than=1024)

CFLAGS_REMOVE_main.o += -fno-builtin

CFLAGS_main.o += -g0 -fno-unwind-tables -fno-asynchronous-unwind-tables -fomit-frame-pointer
CFLAGS_main.o += $(call cc-option, -momit-leaf-frame-pointer)

CFLAGS_REMOVE_main.o += $(CC_FLAGS_FTRACE)
CFLAGS_REMOVE_main.o += -pg

CFLAGS_REMOVE_main.o += -ftrivial-auto-var-init=pattern
CFLAGS_REMOVE_main.o += -ftrivial-auto-var-init=zero
CFLAGS_REMOVE_main.o += -enable-trivial-auto-var-init-zero-knowing-it-will-be-removed-from-clang

# if cflags can be macro'd, this will be called 'TRUST_ME'
CFLAGS_main.o += -fno-stack-protector -fno-stack-check
CFLAGS_REMOVE_main.o += -fsanitize=shadow-call-stack
