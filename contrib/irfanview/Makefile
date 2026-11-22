# Makefile for packJPG IrfanView & Susie Plug-in (Windows x86 / x64 MinGW-w64)

CXX      ?= g++
CXXSTD   ?= -std=c++17
CXXFLAGS ?= -O2 -Wall -Wextra -Werror -pedantic
STRIP    ?= -s

TOOLCHAIN_PREFIX := $(patsubst %g++,%,$(patsubst %clang++,%,$(CXX)))
ifneq ($(TOOLCHAIN_PREFIX),$(CXX))
ifeq ($(origin AR),default)
AR = $(TOOLCHAIN_PREFIX)ar
endif
RANLIB ?= $(TOOLCHAIN_PREFIX)ranlib
else
AR     ?= ar
RANLIB ?= ranlib
endif

CORE_DIR   = ../../source
STATIC_LIB = $(CORE_DIR)/packJPGlib.a

MACHINE := $(shell $(CXX) -dumpmachine 2>/dev/null)
ifneq (,$(filter i386-% i486-% i586-% i686-%,$(MACHINE)))
ARCH_TAG := x86
SPI_EXT  := spi
KILL_AT  := -Wl,--kill-at
else ifneq (,$(findstring x86_64,$(MACHINE))$(findstring aarch64,$(MACHINE))$(findstring arm64,$(MACHINE)))
ARCH_TAG := x64
SPI_EXT  := sph
KILL_AT  :=
else
ARCH_TAG := x86
SPI_EXT  := spi
KILL_AT  := -Wl,--kill-at
endif

ARCH_STAMP = .arch_$(ARCH_TAG)
IV_DLL     = PackJPG$(ARCH_TAG).dll
SPI_DLL    = ifpjg.$(SPI_EXT)
TEST_EXE   = test_irfanview_plugin.exe

PLUGIN_SRC = pjg_wic_dib.cpp plugin_exports.cpp
PLUGIN_HDR = pjg_wic_dib.h spi00in.h $(CORE_DIR)/packjpglib.h $(CORE_DIR)/codec_pipeline.h
PLUGIN_OBJ = $(PLUGIN_SRC:.cpp=.o)

ALL_CXXFLAGS = -I. -I$(CORE_DIR) -DBUILD_LIB $(CXXSTD) $(CXXFLAGS)
PLUGIN_LDFLAGS = -shared $(KILL_AT) -Wl,--exclude-libs,ALL \
                 $(STRIP) -static-libgcc -static-libstdc++ \
                 -Wl,-Bstatic -lpthread -Wl,-Bdynamic \
                 -lwindowscodecs -lole32 -loleaut32 -luuid $(LDFLAGS)

TEST_LDFLAGS = -static -static-libgcc -static-libstdc++ -lpthread $(LDFLAGS)

.PHONY: all test clean core_lib

ifneq ($(filter clean,$(MAKECMDGOALS)),)
.NOTPARALLEL:
endif

all: $(IV_DLL) $(SPI_DLL)

$(ARCH_STAMP):
	rm -f .arch_* $(PLUGIN_OBJ)
	$(MAKE) -C $(CORE_DIR) clean TARGET_OS=Windows_NT
	@echo $(ARCH_TAG) > $@

core_lib: $(ARCH_STAMP)
	$(MAKE) -C $(CORE_DIR) lib TARGET_OS=Windows_NT CXX="$(CXX)" AR="$(AR)" RANLIB="$(RANLIB)"

%.o: %.cpp $(PLUGIN_HDR) $(ARCH_STAMP)
	$(CXX) -c -o $@ $< $(ALL_CXXFLAGS)

$(IV_DLL): $(PLUGIN_OBJ) core_lib
	$(CXX) -o $@ $(PLUGIN_OBJ) $(STATIC_LIB) $(PLUGIN_LDFLAGS)

$(SPI_DLL): $(IV_DLL)
	cp -f $(IV_DLL) $(SPI_DLL)

$(TEST_EXE): test_irfanview_plugin.cpp spi00in.h core_lib $(IV_DLL) $(SPI_DLL)
	$(CXX) -I. -I$(CORE_DIR) -DBUILD_LIB $(CXXSTD) $(CXXFLAGS) -o $@ test_irfanview_plugin.cpp $(STATIC_LIB) $(TEST_LDFLAGS)

test: all $(TEST_EXE)
	./$(TEST_EXE) ./$(IV_DLL) ./$(SPI_DLL)

clean:
	rm -f $(PLUGIN_OBJ) .arch_* PackJPGx64.dll PackJPGx86.dll ifpjg.sph ifpjg.spi $(TEST_EXE)
