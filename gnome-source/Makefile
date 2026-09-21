# MirvkBuntu gnome-source/ — GNOME module source acquisition + build.
#
# Wrappers over pull-all-source.sh (fetch every module's source/, pinned via
# GNOME_VERSIONS) and build-module.sh (compile a single module). Flags:
#   FORCE=1        re-fetch modules whose source/ already exists
#   MODULES="a b"  restrict to a subset
#   <MODULE>_REF   pin a module (see GNOME_VERSIONS)

SHELL := /bin/bash
HERE  := $(CURDIR)

# Modules required by the native GNOME stage.
MODULES_ALL := cairo glib gdk-pixbuf gtk glib-networking gvfs mutter \
               gnome-shell gnome-control-center gnome-software gnome-terminal orca

.PHONY: help pull pull-force build clean-source

help:
	@echo "gnome-source/ targets:"
	@echo "  make pull                 Fetch all required module source/ (pinned)"
	@echo "  make pull MODULES=\"glib gtk\"  Fetch a subset"
	@echo "  make pull-force           Re-fetch even if source/ already present"
	@echo "  make build MODULE=gtk     Build one module via build-module.sh"
	@echo "  make clean-source         Remove all fetched module source/ trees"

pull:
	bash "$(HERE)/pull-all-source.sh" $(MODULES)

pull-force:
	FORCE=1 bash "$(HERE)/pull-all-source.sh" $(MODULES)

build:
	@test -n "$(MODULE)" || { echo "ERROR: set MODULE=<name> (e.g. MODULE=gtk)"; exit 2; }
	bash "$(HERE)/build-module.sh" "$(MODULE)"

clean-source:
	@for m in $(MODULES_ALL); do rm -rf "$(HERE)/$$m/source"; done
	@echo "Removed module source/ trees."
