CC = gcc
# Compiler flags recommended by Dr. Hogg
CFLAGS_HOGG = --std=gnu23 -g
# Compiler flags recommended by the OpenSSF Best Practices Working Group as of 2025-01-23
CFLAGS_OPENSSF = -O2 -Wall -Wformat -Wformat=2 -Wconversion -Wimplicit-fallthrough \
-Werror=format-security \
-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 \
-D_GLIBCXX_ASSERTIONS \
-fstrict-flex-arrays=3 \
-fstack-clash-protection -fstack-protector-strong \
-Wl,-z,nodlopen -Wl,-z,noexecstack \
-Wl,-z,relro -Wl,-z,now \
-Wl,--as-needed -Wl,--no-copy-dt-needed-entries \
-Wtrampolines \
-Wbidi-chars=any \
-fPIE -pie \
-fcf-protection=full \
-Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion

CFLAGS = ${CFLAGS_HOGG} ${CFLAGS_OPENSSF}

.PHONY: default readline noreadline clean all


# By default compile with readline if the readline library is available
HAS_READLINE := $(shell ldconfig -p | grep -q readline && echo yes)
ifeq ($(HAS_READLINE), yes)
DEFAULT_TARGET = bestsh
else
DEFAULT_TARGET = bestsh-noreadline
endif

default: $(DEFAULT_TARGET)


# Compiler flags necessary to compile with the GNU readline library
READLINE_FLAGS = -D USE_READLINE -lreadline -lhistory

bestsh: bestsh.c musys.c
	${CC} ${CFLAGS} bestsh.c musys.c $(READLINE_FLAGS) -o bestsh

bestsh-noreadline: bestsh.c musys.c
	${CC} ${CFLAGS} bestsh.c musys.c -o bestsh-noreadline


# Phony targets
readline: bestsh

noreadline: bestsh-noreadline

all: bestsh bestsh-noreadline

# if you add a goal that doesn't make the default executable, update
# clean to rm its executable
clean:
	@if [ "$(MAKECMDGOALS)" = "clean" ]; then \
		rm -f $(DEFAULT_TARGET); \
	else \
		for goal in $(MAKECMDGOALS); do \
			if [ "$$goal" = "all" ]; then \
				rm -f bestsh bestsh-noreadline; \
			elif [ "$$goal" = "bestsh" ] || [ "$$goal" = "readline" ]; then \
				rm -f bestsh; \
			elif [ "$$goal" = "bestsh-noreadline" ] || [ "$$goal" = "noreadline" ]; then \
				rm -f bestsh-noreadline; \
			elif [ "$$goal" != "clean" ]; then \
				rm -f $(DEFAULT_TARGET); \
			fi; \
		done; \
	fi;
