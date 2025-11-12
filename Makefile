ifeq ($(origin CXX), environment)
  override CXX := em++
endif
ifeq ($(origin CC), environment)
  override CC := emcc
endif

web:   CXX := em++
web:   CC  := emcc
web:   all

linux: CXX := clang++
linux: CC  := clang
linux: all

CXX ?= em++
CC  ?= emcc

ROOT_BUILD_DIR ?= _Intermediate
ROOT_OUTPUT_DIR ?= _Builds

ifeq ($(findstring em++,$(notdir $(CXX))),em++)
	BUILD_DIR ?= $(ROOT_BUILD_DIR)/web
	OUTPUT_DIR ?= $(ROOT_OUTPUT_DIR)/web
	OUT ?= index
	EXT ?= .html
else
	BUILD_DIR ?= $(ROOT_BUILD_DIR)/linux
	OUTPUT_DIR ?= $(ROOT_OUTPUT_DIR)/linux
	OUT ?= secondreality
	EXT ?= 
endif

SRC_DIR   ?= .
TARGET    := $(OUTPUT_DIR)/$(OUT)$(EXT)

CPPFLAGS += -I$(SRC_DIR)

CXXFLAGS  ?= -O3 -std=c++20 -Wall -Wextra -Wno-missing-braces -Wno-unused-function -pthread
CFLAGS    ?= -O3 -Wall -Wextra -Wno-missing-braces -Wno-unused-function

ifeq ($(findstring em++,$(notdir $(CXX))),em++)
  EM_COMPILE_FLAGS := -sUSE_SDL=2
  EM_LINK_FLAGS    := -sUSE_SDL=2 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3=1 -sALLOW_MEMORY_GROWTH=1 -sWASM=1 -sASYNCIFY

  CXXFLAGS += $(EM_COMPILE_FLAGS)
  CFLAGS   += $(EM_COMPILE_FLAGS)
  LDFLAGS  += $(EM_LINK_FLAGS)
else
  CXXFLAGS += $(shell sdl2-config --cflags)
  LDLIBS   += $(shell sdl2-config --libs) -lGLESv2
endif

ifeq ($(OS),Windows_NT)
  MKDIR_LINE = if not exist "$(@D)" mkdir "$(@D)"
  RM_RF      = rmdir /S /Q
  WHICH     := where
  NULLDEV   := NUL
  CP := copy /Y  
else
  MKDIR_LINE = mkdir -p "$(@D)"
  RM_RF      = rm -rf
  WHICH     := which
  NULLDEV   := /dev/null
endif

rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*/),$(call rwildcard,$d,$2))

CPP_EXTS := cpp cc cxx CPP C++
C_EXTS   := c

CPP_SOURCES := $(foreach e,$(CPP_EXTS),$(call rwildcard,$(SRC_DIR)/,*.$(e)))
C_SOURCES   := $(foreach e,$(C_EXTS),  $(call rwildcard,$(SRC_DIR)/,*.$(e)))

EXCLUDE_DIRS ?= References
EXCLUDE_GLOBS := $(foreach d,$(EXCLUDE_DIRS),$(SRC_DIR)/$(d)/%)

CPP_SOURCES   := $(filter-out $(EXCLUDE_GLOBS),$(CPP_SOURCES))
C_SOURCES     := $(filter-out $(EXCLUDE_GLOBS),$(C_SOURCES))

CPP_OBJECTS := $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(CPP_SOURCES))
C_OBJECTS   := $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(C_SOURCES))

# Normalize extensions to .o
OBJECTS := $(CPP_OBJECTS:.cpp=.o)
OBJECTS := $(OBJECTS:.cc=.o)
OBJECTS := $(OBJECTS:.cxx=.o)
OBJECTS := $(OBJECTS:.CPP=.o)
OBJECTS := $(OBJECTS:.C++=.o)
OBJECTS := $(OBJECTS:.c=.o)
OBJECTS := $(sort $(OBJECTS))

.PHONY: all clean tree verify-toolchain print-% help

all: verify-toolchain $(TARGET)

help:
	@echo Targets:
	@echo   make tree : show sources/objects/target
	@echo   make clean : clean-up
	@echo   make linux : linux platform
	@echo   emmake make web : web platform
	@echo.
	@echo if you get that [fatal error: SDL2/SDL.h: No such file or directory], run command below:
	@echo   Ubuntu/Debian: sudo apt install libsdl2-dev
	@echo   Fedora: sudo dnf install SDL2-devel
	@echo   Arch: sudo pacman -S sdl2
	@echo   openSUSE: sudo zypper install libSDL2-devel
	@echo   Alpine: sudo apk add sdl2-dev

tree:
	@echo "Sources (C++):"
	@$(foreach s,$(CPP_SOURCES),echo "  $(s)";)
	@echo "Sources (C):"
	@$(foreach s,$(C_SOURCES),echo "  $(s)";)
	@echo "Objects:"
	@$(foreach o,$(OBJECTS),echo "  $(o)";)
	@echo "Target: $(TARGET)"

print-%:
	@echo '$* = $($*)'

verify-toolchain:
	@$(WHICH) "$(CXX)" > $(NULLDEV) 2>&1 || ( \
	  echo ERROR: CXX '$(CXX)' not found. Use emsdk_env.bat or set CXX=path\to\em++.bat ; exit 1 )
	@$(WHICH) "$(CC)" > $(NULLDEV) 2>&1 || ( \
	  echo ERROR: CC '$(CC)' not found.  Use emsdk_env.bat or set CC=path\to\emcc.bat ; exit 1 )

$(BUILD_DIR):
	@$(MKDIR_LINE)

$(ROOT_OUTPUT_DIR):
	@$(MKDIR_LINE)

$(OUTPUT_DIR):
	@$(MKDIR_LINE)

$(TARGET): $(OBJECTS) | $(OUTPUT_DIR)
	@$(MKDIR_LINE)
ifeq ($(findstring em++,$(notdir $(CXX))),em++)
	$(CXX) --shell-file Web/shell.html -o "$@" $(OBJECTS) $(LDFLAGS) $(LDLIBS)
	@cp -f Resources/atomic_playboy.ico $(OUTPUT_DIR)/favicon.ico
else
	$(CXX) -o "$@" $(OBJECTS) $(LDFLAGS) $(LDLIBS)
endif

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@$(MKDIR_LINE)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "$<" -o "$@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cc
	@$(MKDIR_LINE)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "$<" -o "$@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cxx
	@$(MKDIR_LINE)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "$<" -o "$@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.CPP
	@$(MKDIR_LINE)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "$<" -o "$@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.C++
	@$(MKDIR_LINE)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "$<" -o "$@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@$(MKDIR_LINE)
	$(CC)  $(CPPFLAGS) $(CFLAGS)   -c "$<" -o "$@"

serve: all
	python -m http.server -d $(OUTPUT_DIR)

clean:
	-@$(RM_RF) "$(ROOT_BUILD_DIR)" 2>$(NULLDEV)
	-@$(RM_RF) "$(ROOT_OUTPUT_DIR)" 2>$(NULLDEV)
