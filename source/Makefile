# packJPG universal Makefile (Linux, macOS, Windows MinGW-w64)

TARGET   ?= packJPG
CXXSTD   ?= -std=c++17
CXXFLAGS ?= -O3 -Wall -pedantic -funroll-loops -ffast-math -fomit-frame-pointer
AR       ?= ar
RANLIB   ?= ranlib
WINDRES  ?= windres
PYTHON   ?= python3
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
TEST_BIN     = test_runner$(EXEEXT)
FUZZ_BIN     = fuzz_roundtrip$(EXEEXT)
STATIC_LIB   = $(TARGET)lib.a
CPPSRC       = aricoder.cpp bitops.cpp packjpg.cpp
DEPS         = aricoder.h bitops.h dct8x8.h packjpgdll.h packjpglib.h pjpgtbl.h Makefile
OBJ          = $(CPPSRC:.cpp=.o)
LIB_OBJ      = $(CPPSRC:.cpp=.lib.o)
DLL_OBJ      = $(CPPSRC:.cpp=.dll.o)
TEST_SRC     = ../tests/test_main.cpp ../tests/test_bitops.cpp ../tests/test_aricoder.cpp ../tests/test_library_api.cpp
ALL_CXXFLAGS = -I. $(CXXSTD) $(OS_CPPFLAGS) $(TARGET_CPPFLAGS) $(CPPFLAGS) $(CXXFLAGS)
CLEAN_FILES  = $(OBJ) $(LIB_OBJ) $(DLL_OBJ) icons.res $(STATIC_LIB) lib$(TARGET).a \
               $(TARGET) $(TARGET).exe $(TARGET).dll lib$(TARGET).so lib$(TARGET).dylib \
               test_runner test_runner.exe fuzz_roundtrip fuzz_roundtrip.exe

.PHONY: all dev lib dll shared test fuzz install uninstall clean

ifneq ($(filter clean,$(MAKECMDGOALS)),)
.NOTPARALLEL:
endif

all: $(BIN)

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(ALL_CXXFLAGS)

%.lib.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(ALL_CXXFLAGS) -DBUILD_LIB $(PIC_FLAGS)

%.dll.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(ALL_CXXFLAGS) -DBUILD_DLL $(PIC_FLAGS) -fvisibility=hidden

%.res: %.rc app_icon.ico file_icon.ico
	$(WINDRES) -O coff $< $@

$(BIN): $(OBJ) $(RES)
	$(CXX) -o $@ $^ $(STRIP) $(OS_LDFLAGS) $(LDFLAGS)
ifeq ($(USE_UPX),1)
	$(UPX) --best --lzma $@
endif

dev: TARGET_CPPFLAGS += -DDEV_BUILD
dev: $(BIN)

lib: $(STATIC_LIB)

$(STATIC_LIB): $(LIB_OBJ)
	$(AR) rcs $@ $(LIB_OBJ)
	$(RANLIB) $@

shared: dll

dll: $(SHLIB)

$(SHLIB): $(DLL_OBJ)
	$(CXX) $(SHLIB_LDFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_BIN): $(TEST_SRC) ../tests/test_framework.h $(STATIC_LIB)
	$(CXX) $(ALL_CXXFLAGS) -I../tests -o $@ $(TEST_SRC) $(STATIC_LIB) $(OS_LDFLAGS) $(LDFLAGS)

test: $(BIN) $(TEST_BIN)
	./$(TEST_BIN)
	$(PYTHON) ../tests/run_tests.py ./$(BIN)

fuzz:
	clang++ -I. $(CXXSTD) $(OS_CPPFLAGS) -DBUILD_LIB -O1 -g -fno-omit-frame-pointer \
		-fsanitize=fuzzer,address,undefined $(CPPSRC) ../tests/fuzz_roundtrip.cpp -o $(FUZZ_BIN)

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
