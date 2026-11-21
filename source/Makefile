# packJPG universal Makefile (Linux, macOS, Windows MinGW-w64)

TARGET   ?= packJPG
CXXSTD   ?= -std=c++17
CXXFLAGS ?= -O3 -Wall -pedantic -funroll-loops -ffast-math -fomit-frame-pointer
AR       ?= ar
RANLIB   ?= ranlib
WINDRES  ?= windres
UPX      ?= upx
USE_UPX  ?= 0
STATIC   ?= 0
PREFIX   ?= /usr/local
BINDIR   ?= $(PREFIX)/bin

ifeq ($(origin TARGET_OS),undefined)
ifeq ($(OS),Windows_NT)
TARGET_OS := Windows_NT
else
TARGET_OS := $(shell uname -s 2>/dev/null || echo Unix)
endif
endif

ifeq ($(TARGET_OS),Windows_NT)
ifeq ($(origin CXX),default)
CXX            = g++
endif
EXEEXT         = .exe
RES            = icons.res
OS_CPPFLAGS    =
PIC_FLAGS      =
OS_LDFLAGS     = -static -static-libgcc -static-libstdc++ -lpthread
SHLIB          = $(TARGET).dll
SHLIB_LDFLAGS  = -shared -Wl,--out-implib,lib$(TARGET).a -fvisibility=hidden $(OS_LDFLAGS)
else ifeq ($(TARGET_OS),Darwin)
ifeq ($(origin CXX),default)
CXX            = c++
endif
EXEEXT         =
RES            =
OS_CPPFLAGS    = -DUNIX
PIC_FLAGS      = -fPIC
OS_LDFLAGS     =
SHLIB          = lib$(TARGET).dylib
SHLIB_LDFLAGS  = -dynamiclib -fvisibility=hidden
else
ifeq ($(origin CXX),default)
CXX            = g++
endif
EXEEXT         =
RES            =
OS_CPPFLAGS    = -DUNIX
PIC_FLAGS      = -fPIC
ifeq ($(STATIC),1)
OS_LDFLAGS     = -static -static-libgcc -static-libstdc++
else
OS_LDFLAGS     =
endif
SHLIB          = lib$(TARGET).so
SHLIB_LDFLAGS  = -shared -fvisibility=hidden
endif

ifeq ($(TARGET_OS),Darwin)
STRIP ?=
else ifneq ($(findstring -g,$(CXXFLAGS)),)
STRIP ?=
else
STRIP ?= -s
endif

BIN          = $(TARGET)$(EXEEXT)
STATIC_LIB   = $(TARGET)lib.a
CPPSRC       = aricoder.cpp bitops.cpp packjpg.cpp
DEPS         = aricoder.h bitops.h dct8x8.h packjpgdll.h packjpglib.h pjpgtbl.h Makefile
OBJ          = $(CPPSRC:.cpp=.o)
ALL_CXXFLAGS = -I. $(CXXSTD) $(OS_CPPFLAGS) $(TARGET_CPPFLAGS) $(CPPFLAGS) $(CXXFLAGS)
CLEAN_FILES  = $(OBJ) icons.res $(STATIC_LIB) lib$(TARGET).a $(TARGET) $(TARGET).exe $(TARGET).dll lib$(TARGET).so lib$(TARGET).dylib

.PHONY: all dev lib dll shared install uninstall clean

ifneq ($(filter clean,$(MAKECMDGOALS)),)
.NOTPARALLEL:
endif

all: $(BIN)

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(ALL_CXXFLAGS)

%.res: %.rc app_icon.ico file_icon.ico
	$(WINDRES) -O coff $< $@

$(BIN): $(OBJ) $(RES)
	$(CXX) -o $@ $^ $(STRIP) $(OS_LDFLAGS) $(LDFLAGS)
ifeq ($(USE_UPX),1)
	$(UPX) --best --lzma $@
endif

dev: TARGET_CPPFLAGS += -DDEV_BUILD
dev: $(BIN)

lib: TARGET_CPPFLAGS += -DBUILD_LIB $(PIC_FLAGS)
lib: $(STATIC_LIB)

$(STATIC_LIB): $(OBJ)
	$(AR) rcs $@ $(OBJ)
	$(RANLIB) $@

shared: dll

dll: TARGET_CPPFLAGS += -DBUILD_DLL $(PIC_FLAGS) -fvisibility=hidden
dll: $(SHLIB)

$(SHLIB): $(OBJ)
	$(CXX) $(SHLIB_LDFLAGS) -o $@ $^ $(LDFLAGS)

install: $(BIN)
	install -d "$(DESTDIR)$(BINDIR)"
	install -m 755 $(BIN) "$(DESTDIR)$(BINDIR)/$(BIN)"

uninstall:
	$(RM) "$(DESTDIR)$(BINDIR)/$(BIN)"

clean:
ifeq ($(OS)_$(findstring sh,$(SHELL)),Windows_NT_)
	@for %%f in ($(CLEAN_FILES)) do @if exist "%%f" del /F /Q "%%f"
else
	$(RM) $(CLEAN_FILES)
endif
