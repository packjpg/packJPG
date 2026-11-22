# Makefile for packJPG IrfanView & Susie Plug-in (Windows x86 / x64 / arm64 MinGW-w64)

CXX      ?= g++
CXXSTD   ?= -std=c++17
CXXFLAGS ?= -O2 -Wall -Wextra -Werror -pedantic

ifeq ($(origin STRIP),command line)
ifeq ($(STRIP),)
STRIP_FLAGS ?=
else ifneq (,$(filter -%,$(STRIP)))
STRIP_FLAGS ?= $(STRIP)
else
STRIP_FLAGS ?= -s
endif
else ifneq ($(findstring -g,$(CXXFLAGS)),)
STRIP_FLAGS ?=
else ifneq (,$(filter -%,$(STRIP)))
STRIP_FLAGS ?= $(STRIP)
else
STRIP_FLAGS ?= -s
endif

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

CORE_DIR = ../../source

MACHINE := $(shell $(CXX) $(CXXFLAGS) -dumpmachine 2>/dev/null)
ifneq (,$(filter i386-% i486-% i586-% i686-%,$(MACHINE)))
ARCH_TAG := x86
SPI_EXT  := spi
KILL_AT  := -Wl,--kill-at
else ifneq (,$(findstring aarch64,$(MACHINE))$(findstring arm64,$(MACHINE)))
ARCH_TAG := arm64
SPI_EXT  := sph
KILL_AT  :=
else ifneq (,$(findstring x86_64,$(MACHINE)))
ARCH_TAG := x64
SPI_EXT  := sph
KILL_AT  :=
else
ARCH_TAG := x86
SPI_EXT  := spi
KILL_AT  := -Wl,--kill-at
endif

BUILD_DIR   = build_$(ARCH_TAG)
STATIC_LIB  = $(BUILD_DIR)/packJPGlib.a
IV_DLL      = PackJPG$(ARCH_TAG).dll
SPI_DLL     = ifpjg.$(SPI_EXT)
TEST_EXE    = test_irfanview_plugin.exe

CORE_NAMES  = aricoder bitops codec_pipeline jpeg_codec pjg_codec packjpg_api
CORE_OBJ    = $(addprefix $(BUILD_DIR)/,$(addsuffix .core.o,$(CORE_NAMES)))
CORE_HDR    = $(CORE_DIR)/aricoder.h $(CORE_DIR)/bitops.h $(CORE_DIR)/codec_context.h \
              $(CORE_DIR)/codec_pipeline.h $(CORE_DIR)/dct8x8.h $(CORE_DIR)/jpeg_codec.h \
              $(CORE_DIR)/packjpgdll.h $(CORE_DIR)/packjpglib.h $(CORE_DIR)/path_utils.h \
              $(CORE_DIR)/pjg_codec.h $(CORE_DIR)/pjpgtbl.h

PLUGIN_SRC  = pjg_wic_dib.cpp plugin_exports.cpp
PLUGIN_HDR  = pjg_wic_dib.h spi00in.h $(CORE_DIR)/packjpglib.h $(CORE_DIR)/codec_pipeline.h
PLUGIN_OBJ  = $(addprefix $(BUILD_DIR)/,$(PLUGIN_SRC:.cpp=.plugin.o))

ALL_CXXFLAGS   = -I. -I$(CORE_DIR) -DBUILD_LIB $(CXXSTD) $(CXXFLAGS)
PLUGIN_LDFLAGS = -shared $(KILL_AT) -Wl,--exclude-libs,ALL \
                 $(STRIP_FLAGS) -static-libgcc -static-libstdc++ \
                 -Wl,-Bstatic -lpthread -Wl,-Bdynamic \
                 -lwindowscodecs -lole32 -loleaut32 -luuid $(LDFLAGS)

TEST_LDFLAGS   = -static -static-libgcc -static-libstdc++ -lpthread $(LDFLAGS)

COMPILER_ID    := $(CXX)|$(CXXFLAGS)
COMPILER_STAMP  = $(BUILD_DIR)/.compiler_stamp

ifeq (,$(filter clean,$(MAKECMDGOALS)))
_PREP_BUILD := $(shell mkdir -p "$(BUILD_DIR)"; \
	if [ ! -f "$(COMPILER_STAMP)" ] || [ "$$(cat "$(COMPILER_STAMP)" 2>/dev/null)" != "$(COMPILER_ID)" ]; then \
		rm -f "$(BUILD_DIR)"/*.o "$(STATIC_LIB)"; \
		printf '%s\n' "$(COMPILER_ID)" > "$(COMPILER_STAMP)"; \
	fi)
endif

.PHONY: all test clean core_lib

ifneq ($(filter clean,$(MAKECMDGOALS)),)
.NOTPARALLEL:
endif

all: $(IV_DLL) $(SPI_DLL)

$(BUILD_DIR)/%.core.o: $(CORE_DIR)/%.cpp $(CORE_HDR) $(COMPILER_STAMP)
	$(CXX) -c -o $@ $< $(ALL_CXXFLAGS)

$(STATIC_LIB): $(CORE_OBJ)
	$(AR) rcs $@ $(CORE_OBJ)
	$(RANLIB) $@

core_lib: $(STATIC_LIB)

$(BUILD_DIR)/%.plugin.o: %.cpp $(PLUGIN_HDR) $(COMPILER_STAMP)
	$(CXX) -c -o $@ $< $(ALL_CXXFLAGS)

$(IV_DLL): $(PLUGIN_OBJ) $(STATIC_LIB)
	$(CXX) -o $@ $(PLUGIN_OBJ) $(STATIC_LIB) $(PLUGIN_LDFLAGS)

$(SPI_DLL): $(IV_DLL)
	cp -f $(IV_DLL) $(SPI_DLL)

$(TEST_EXE): test_irfanview_plugin.cpp pjg_wic_dib.h spi00in.h $(STATIC_LIB) $(IV_DLL) $(SPI_DLL)
	$(CXX) -I. -I$(CORE_DIR) -DBUILD_LIB $(CXXSTD) $(CXXFLAGS) -o $@ test_irfanview_plugin.cpp $(STATIC_LIB) $(TEST_LDFLAGS)

test: all $(TEST_EXE)
	./$(TEST_EXE) ./$(IV_DLL) ./$(SPI_DLL)

clean:
	rm -rf build_x86 build_x64 build_arm64 *.o .arch_* PackJPGx64.dll PackJPGx86.dll PackJPGarm64.dll ifpjg.sph ifpjg.spi $(TEST_EXE)
