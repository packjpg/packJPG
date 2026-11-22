# packJPG universal Makefile (Linux, macOS, Windows MinGW-w64)

TARGET   ?= packJPG
CXXSTD   ?= -std=c++17
CXXFLAGS ?= -O3 -Wall -pedantic -funroll-loops -fomit-frame-pointer
AR       ?= ar
RANLIB   ?= ranlib
WINDRES  ?= windres
PYTHON   ?= python3
FUZZ_CXX ?= clang++
UPX      ?= upx
USE_UPX  ?= 0
USE_LTO  ?= 1
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
THREAD_FLAGS   =
OS_LDFLAGS     = -static -static-libgcc -static-libstdc++ -lpthread
BIN_LDFLAGS    = $(OS_LDFLAGS) -lshell32
SHLIB          = $(TARGET).dll
SHLIB_FLAGS    = -shared -Wl,--out-implib,lib$(TARGET).a -fvisibility=hidden
else ifeq ($(TARGET_OS),Darwin)
ifeq ($(origin CXX),default)
CXX            = c++
endif
EXEEXT         =
RES            =
OS_CPPFLAGS    = -DUNIX
PIC_FLAGS      = -fPIC
THREAD_FLAGS   = -pthread
OS_LDFLAGS     =
BIN_LDFLAGS    =
SHLIB          = lib$(TARGET).dylib
SHLIB_FLAGS    = -dynamiclib -Wl,-install_name,@rpath/lib$(TARGET).dylib -fvisibility=hidden
else
ifeq ($(origin CXX),default)
CXX            = g++
endif
EXEEXT         =
RES            =
OS_CPPFLAGS    = -DUNIX
PIC_FLAGS      = -fPIC
THREAD_FLAGS   = -pthread
OS_LDFLAGS     =
ifeq ($(STATIC),1)
BIN_LDFLAGS    = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
else
BIN_LDFLAGS    = $(OS_LDFLAGS)
endif
SHLIB          = lib$(TARGET).so
SHLIB_FLAGS    = -shared -fvisibility=hidden
endif

ifeq ($(TARGET_OS),Darwin)
STRIP ?=
else ifneq ($(findstring -g,$(CXXFLAGS)),)
STRIP ?=
else
STRIP ?= -s
endif

ifeq ($(USE_LTO),1)
ifeq ($(findstring -fsanitize,$(CXXFLAGS))$(findstring -g,$(CXXFLAGS)),)
LTO_FLAGS ?= -flto
endif
endif

BIN          = $(TARGET)$(EXEEXT)
TEST_BIN     = test_runner$(EXEEXT)
FUZZ_BIN     = fuzz_roundtrip$(EXEEXT)
STATIC_LIB   = $(TARGET)lib.a
CORE_SRC     = aricoder.cpp bitops.cpp codec_pipeline.cpp jpeg_codec.cpp pjg_codec.cpp
CLI_SRC      = $(CORE_SRC) cli_main.cpp
DEV_SRC      = $(CLI_SRC) dev_tools.cpp
LIB_SRC      = $(CORE_SRC) packjpg_api.cpp
DEPS         = aricoder.h bitops.h codec_context.h codec_pipeline.h dct8x8.h dev_tools.h \
               jpeg_codec.h packjpgdll.h packjpglib.h path_utils.h pjg_codec.h pjpgtbl.h Makefile
OBJ          = $(CLI_SRC:.cpp=.o)
DEV_OBJ      = $(DEV_SRC:.cpp=.dev.o)
LIB_OBJ      = $(LIB_SRC:.cpp=.lib.o)
DLL_OBJ      = $(LIB_SRC:.cpp=.dll.o)
TEST_SRC     = ../tests/test_main.cpp ../tests/test_bitops.cpp ../tests/test_aricoder.cpp \
               ../tests/test_library_api.cpp ../tests/test_huffman_and_concurrency.cpp
ALL_CXXFLAGS = -I. $(CXXSTD) $(OS_CPPFLAGS) $(CPPFLAGS) $(CXXFLAGS)
CLEAN_FILES  = $(OBJ) $(DEV_OBJ) $(LIB_OBJ) $(DLL_OBJ) icons.res $(STATIC_LIB) lib$(TARGET).a \
               $(TARGET) $(TARGET).exe $(TARGET).dll lib$(TARGET).so lib$(TARGET).dylib \
               test_runner test_runner.exe fuzz_roundtrip fuzz_roundtrip.exe \
               .stamp_std .stamp_dev .stamp_dev_bin

.PHONY: all dev lib dll shared test fuzz install uninstall clean .force_dev_bin

ifneq ($(filter clean,$(MAKECMDGOALS)),)
.NOTPARALLEL:
endif
ifneq (,$(and $(filter all,$(MAKECMDGOALS)),$(filter dev,$(MAKECMDGOALS))))
.NOTPARALLEL:
endif

ifeq (,$(wildcard $(BIN)))
DEV_BIN_FORCE = .force_dev_bin
else
DEV_BIN_FORCE =
endif

all: $(BIN)

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(ALL_CXXFLAGS) $(THREAD_FLAGS) $(LTO_FLAGS)

%.dev.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(ALL_CXXFLAGS) $(THREAD_FLAGS) $(LTO_FLAGS) -DDEV_BUILD

%.lib.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(ALL_CXXFLAGS) $(THREAD_FLAGS) -DBUILD_LIB $(PIC_FLAGS)

%.dll.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(ALL_CXXFLAGS) $(THREAD_FLAGS) $(LTO_FLAGS) -DBUILD_DLL $(PIC_FLAGS) -fvisibility=hidden

%.res: %.rc app_icon.ico file_icon.ico packjpg.manifest
	$(WINDRES) -O coff $< $@

ifeq ($(OS)_$(findstring sh,$(SHELL)),Windows_NT_)
RM_STAMPS_DEV = @if exist .stamp_dev del /F /Q .stamp_dev & if exist .stamp_dev_bin del /F /Q .stamp_dev_bin
RM_STAMPS_STD = @if exist .stamp_std del /F /Q .stamp_std
else
RM_STAMPS_DEV = @$(RM) .stamp_dev .stamp_dev_bin
RM_STAMPS_STD = @$(RM) .stamp_std
endif

.stamp_std:
	$(RM_STAMPS_DEV)
	@echo std > $@

.stamp_dev:
	$(RM_STAMPS_STD)
	@echo dev > $@

.force_dev_bin:

$(BIN): $(OBJ) $(RES) .stamp_std
	$(CXX) $(ALL_CXXFLAGS) $(THREAD_FLAGS) $(LTO_FLAGS) -o $@ $(OBJ) $(RES) $(STRIP) $(THREAD_FLAGS) $(BIN_LDFLAGS) $(LDFLAGS)
ifeq ($(USE_UPX),1)
	$(UPX) --best --lzma $@
endif

dev: .stamp_dev_bin

.stamp_dev_bin: $(DEV_OBJ) $(RES) .stamp_dev $(DEV_BIN_FORCE)
	$(CXX) $(ALL_CXXFLAGS) $(THREAD_FLAGS) $(LTO_FLAGS) -o $(BIN) $(DEV_OBJ) $(RES) $(STRIP) $(THREAD_FLAGS) $(BIN_LDFLAGS) $(LDFLAGS)
ifeq ($(USE_UPX),1)
	$(UPX) --best --lzma $(BIN)
endif
	@echo dev > $@

lib: $(STATIC_LIB)

$(STATIC_LIB): $(LIB_OBJ)
	$(AR) rcs $@ $(LIB_OBJ)
	$(RANLIB) $@

shared: dll

dll: $(SHLIB)

$(SHLIB): $(DLL_OBJ)
	$(CXX) $(ALL_CXXFLAGS) $(THREAD_FLAGS) $(LTO_FLAGS) $(SHLIB_FLAGS) -o $@ $(DLL_OBJ) $(THREAD_FLAGS) $(OS_LDFLAGS) $(LDFLAGS)

$(TEST_BIN): $(TEST_SRC) ../tests/test_framework.h $(STATIC_LIB)
	$(CXX) $(ALL_CXXFLAGS) $(THREAD_FLAGS) -I../tests -o $@ $(TEST_SRC) $(STATIC_LIB) $(THREAD_FLAGS) $(OS_LDFLAGS) $(LDFLAGS)

test: $(BIN) $(TEST_BIN)
	./$(TEST_BIN)
	$(PYTHON) ../tests/run_tests.py ./$(BIN)

fuzz:
	$(FUZZ_CXX) -I. $(CXXSTD) $(OS_CPPFLAGS) $(THREAD_FLAGS) -DBUILD_LIB -O1 -g -fno-omit-frame-pointer \
		-fsanitize=fuzzer,address,undefined $(LIB_SRC) ../tests/fuzz_roundtrip.cpp -o $(FUZZ_BIN)

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
