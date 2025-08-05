# MAKEFILE
#
# Copyright (c) 2010-2015
# Roland Karlsson (roland@proxel.se)
# BSD-style - see doc/copyright.txt

# -----------------------------------------------------------
# Usage:
# Write "make" shall build the executables on Mac, Linux or Windows.
# Write "make clean" or "make clobber" cleans up less or more.
# -----------------------------------------------------------

# Set the SYS variable
# -----------------------------------------------------------

include ../sys.mk

# Set version stuff
# -----------------------------------------------------------
NAME = x3f_tools
VER := $(shell git describe --always --dirty --tags)
VERSION = $(VER)-$(TARGET)


# Set compilation and load flags etc
# -----------------------------------------------------------

ifeq (windows, $(TARGET_SYS))
  EXE = .exe
  CFBASE =
  LDBASE = -static
  AUXOBJS = mingw_dowildcard.o
	OCV_LIB = lib
else
ifeq (linux, $(TARGET_SYS))
	OCV_LIB = lib/x86_64-linux-gnu
  EXE =
  CFBASE =
  LDBASE = -lz -ldl -lpthread
  AUXOBJS =
else
ifeq (osx, $(TARGET_SYS))
  EXE =
  CFBASE = $(SDKFLAGS) -Wno-pragmas -arch x86_64 -arch arm64
  LDBASE = $(SDKFLAGS) -Wl,-dead_strip_dylibs -liconv -framework OpenCL -framework Accelerate -arch x86_64 -arch arm64
  AUXOBJS =
else
	OSOCV =
  $(warning WARNING: Unknown target: $(TARGET))
  EXE =
  CFBASE =
  LDBASE =
  AUXOBJS =
endif
endif
endif

OCV = ../deps/lib/$(TARGET)/opencv

ZLIB =
ifneq ($(TARGET_SYS), linux)
ZLIB =
endif
OCV_AUX = $(ZLIB)

OCV_CFLAGS = -I$(OCV)/include/opencv4
OCV_LIBS = $(addprefix $(OCV)/lib/libopencv_,photo.a imgproc.a core.a) $(OCV_AUX)

TIFF_INC1 = ../deps/src/opencv/3rdparty/libtiff
TIFF_INC2 = ../deps/src/build/x86_64/3rdparty/libtiff
TIFF_CFLAGS = -I$(TIFF_INC1) -I$(TIFF_INC2)
TIFF_LIBS = ../deps/src/build/liblibtiff-universal.a

CFLAGS = $(CFBASE) $(TIFF_CFLAGS) -g -O3 -Wall $(C)
CXXFLAGS = $(CFLAGS) $(OCV_CFLAGS) -fvisibility-inlines-hidden
LDFLAGS = $(LDBASE) $(L)

BINDIR = ../bin/$(TARGET)
PROGS = x3f_extract$(EXE) x3f_io_test$(EXE) x3f_matrix_test$(EXE)
VERSION_O = x3f_version-$(VERSION).o

# Build dependencies
# -----------------------------------------------------------

.PHONY: all dist clean clobber

all: $(addprefix $(BINDIR)/,$(PROGS))

-include $(BINDIR)/*.d

# tegra_hal only exists for arm64, so we need to handle it specially
ifeq (osx, $(TARGET_SYS))
TEGRA_HAL = ../deps/src/build/arm64/3rdparty/lib/libtegra_hal.a
else
TEGRA_HAL =
endif

$(BINDIR)/x3f_extract$(EXE): $(addprefix $(BINDIR)/,x3f_extract.o $(VERSION_O) x3f_io.o x3f_process.o x3f_meta.o x3f_image.o x3f_spatial_gain.o x3f_output_dng.o x3f_output_tiff.o x3f_output_ppm.o x3f_histogram.o x3f_print_meta.o x3f_dump.o x3f_matrix.o x3f_dngtags.o x3f_denoise_utils.o x3f_denoise_aniso.o x3f_denoise.o x3f_printf.o $(AUXOBJS)) $(OCV_LIBS) $(TIFF_LIBS) ../deps/src/build/liblibjpeg-turbo-universal.a ../deps/src/build/libittnotify-universal.a ../deps/src/build/libzlib-universal.a $(TEGRA_HAL)
	$(CXX) $^ -o $@ $(LDFLAGS) -lm

$(BINDIR)/x3f_io_test$(EXE): $(addprefix $(BINDIR)/,x3f_io_test.o $(VERSION_O) x3f_io.o x3f_print_meta.o x3f_printf.o $(AUXOBJS))
	$(CC) $^ -o $@ $(LDFLAGS)

$(BINDIR)/x3f_matrix_test$(EXE): $(addprefix $(BINDIR)/,x3f_matrix_test.o x3f_matrix.o x3f_printf.o $(AUXOBJS))
	$(CC) $^ -o $@ $(LDFLAGS) -lm

$(BINDIR)/%.o: %.c | $(BINDIR)
	$(CC) $(CFLAGS) $< -c -MD -o $@

$(BINDIR)/%.o: %.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) $< -c -MD -o $@

#

$(BINDIR)/$(VERSION_O): x3f_version.c | $(BINDIR)
	$(CC) $(CFLAGS) -DVERSION=\"$(VERSION)\" $< -c -MD -o $@

$(BINDIR):
	mkdir $(BINDIR)

# Packaging
# -----------------------------------------------------------

DIST = $(NAME)-$(VERSION)
DIST_DIR = ../dist/$(DIST)
DIST_TGZ = ../dist/$(DIST).tar.gz
DIST_ZIP = ../dist/$(DIST).zip
DIST_FILES = doc/readme.txt doc/copyright.txt bin/x3f_extract$(EXE)
TARFLAGS = --owner=root --group=root

ifeq ($(TARGET_SYS), windows)
dist: $(DIST_ZIP)
else
dist: $(DIST_TGZ)
endif

$(DIST_TGZ): $(addprefix $(DIST_DIR)/,$(DIST_FILES))
	tar $(TARFLAGS) -czf $@ -C ../dist $(addprefix $(DIST)/,$(DIST_FILES))

$(DIST_ZIP): $(addprefix $(DIST_DIR)/,$(DIST_FILES))
	rm -f $@
	cd ../dist && zip $(abspath $@) $(addprefix $(DIST)/,$(DIST_FILES))

$(DIST_DIR)/bin/%: $(BINDIR)/%
	install $(INSTFLAGS) -D -m 755 -s $< $@

$(DIST_DIR)/%: ../%
	install -D -m 644 $< $@

# Clean up dependencies
# -----------------------------------------------------------

clean:
	-@rm ../bin/*/*.o
	-@rm ../bin/*/*.d
	-@rm *~ ../doc/*~
	-@rm -r ../dist/$(NAME)-*/

clobber: clean
	-@rm -r ../bin/*-*/
	-@rm ../dist/$(NAME)-*.tar.gz
	-@rm ../dist/$(NAME)-*.zip
