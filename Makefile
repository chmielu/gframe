#  Makefile
#
#  Robert Chmielowiec 2011
#  robert@chmielowiec.net
#
#  TODO: make install

PROGRAM = gframe
VERSION = 0.2

VALAC = valac

SRC_FILES = \
	src/main.vala \
	src/frame.vala \
	src/settings.vala

DATA_FILES = \
	dialog.ui

VALA_PKGS = \
	gtk+-2.0

.PHONY: all
all: $(PROGRAM)

.PHONY: clean
clean:
	rm -f $(SRC_FILES:.vala=.c)
	rm -f $(PROGRAM)

$(PROGRAM): $(SRC_FILES) Makefile
	$(VALAC) $(foreach pkg,$(VALA_PKGS),--pkg=$(pkg)) \
		$(SRC_FILES) \
		-o $@