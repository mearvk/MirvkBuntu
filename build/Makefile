# =============================================================================
# MirvkBuntu — MASTER ISO Makefile
# =============================================================================
# The primary product of this directory is a bootable MirvkBuntu ISO. This
# Makefile is the single control point for building it end to end:
#
#     prereqs  ->  sources (kernel + GNOME)  ->  native gate  ->  ISO
#
# The authoritative logic lives in the build scripts (build/*.sh, kernels/git.sh,
# gnome-source/pull-all-source.sh); this Makefile orders and drives them.
#
# Quick start (Debian/Ubuntu host, as root, with network):
#     sudo make prereqs        # one-time: install host build tooling
#     make sources             # fetch kernel + GNOME source (no root needed)
#     sudo make iso            # build the desktop ISO (the default product)
#
# Or the whole chain in one go:
#     sudo make all
#
# Build variants:
#     sudo make iso        # desktop ISO   (default)
#     sudo make slim       # slim toram/GNOME ISO
#     sudo make minimal    # minimal ISO
#
# Flags pass straight through (see build/BUILD.FLAGS.md), e.g.:
#     sudo make iso JOBS=8 BUILD_SKIP_CHROMIUM=1
# =============================================================================

SHELL := /bin/bash
HERE  := $(CURDIR)
ROOT  := $(abspath $(HERE)/..)
ARCH  ?= amd64
OUTPUT_DIR := $(HERE)/output

# Marker files so setup steps are only run once (delete to force a redo).
WORK        := $(HERE)/work
STAMP_DIR   := $(WORK)/.stamps
KERNELS_STAMP := $(STAMP_DIR)/kernels
GNOME_STAMP   := $(STAMP_DIR)/gnome-source

# Default product: the desktop ISO.
.DEFAULT_GOAL := iso

.PHONY: all help iso desktop slim minimal \
        prereqs prereqs-remaster sources kernels gnome-source \
        native chromium bootstrap remaster \
        clean distclean

# ---- help -------------------------------------------------------------------
help:
	@echo "MirvkBuntu MASTER ISO Makefile (build/)"
	@echo ""
	@echo "  Primary product: a bootable MirvkBuntu ISO -> build/output/"
	@echo ""
	@echo "  End to end:"
	@echo "    sudo make all          prereqs + sources + desktop ISO"
	@echo ""
	@echo "  ISO variants (needs root):"
	@echo "    sudo make iso          Desktop ISO (default)  -> MirvkBuntu-desktop-$(ARCH).iso"
	@echo "    sudo make slim         Slim toram/GNOME ISO   -> MirvkBuntu-slim-$(ARCH).iso"
	@echo "    sudo make minimal      Minimal ISO            -> MirvkBuntu-minimal-$(ARCH).iso"
	@echo ""
	@echo "  Setup (run once; prereqs needs root, sources need network):"
	@echo "    sudo make prereqs      Install host build tooling (native path)"
	@echo "    sudo make prereqs-remaster  Host tooling for the remaster path"
	@echo "    make sources           Fetch kernel + GNOME source (kernels + gnome-source)"
	@echo "    make kernels           Fetch Linux kernel source"
	@echo "    make gnome-source      Populate GNOME module source/"
	@echo ""
	@echo "  Components / alternates:"
	@echo "    make native            Run the native compilation gate only"
	@echo "    make chromium          Build Chromium only"
	@echo "    sudo make bootstrap    Fetch kernel then build desktop ISO"
	@echo "    sudo make remaster ISO=x  Remaster a stock Ubuntu ISO (no compile)"
	@echo ""
	@echo "  Housekeeping:"
	@echo "    make clean             Remove build/work and build/output"
	@echo "    make distclean         clean + drop the setup stamps"
	@echo ""
	@echo "  Flags pass through (see build/BUILD.FLAGS.md), e.g.:"
	@echo "    sudo make iso JOBS=8 BUILD_SKIP_CHROMIUM=1"

# ---- end-to-end -------------------------------------------------------------
# Full chain: install tooling, fetch sources, then build the desktop ISO.
all: prereqs sources iso

# ---- ISO products -----------------------------------------------------------
# Each ISO build runs the native gate first (inside the build-*.sh scripts), so
# they depend on sources being present. The order-only prerequisite (| sources)
# ensures sources are fetched once without forcing a re-fetch on every ISO run.
iso: desktop

desktop: | sources
	bash "$(HERE)/build-desktop.sh"

slim: | sources
	bash "$(HERE)/build-slim.sh"

minimal: | sources
	bash "$(HERE)/build-minimal.sh"

# ---- setup: host tooling ----------------------------------------------------
prereqs:
	bash "$(HERE)/prerequisites.sh" native

prereqs-remaster:
	bash "$(HERE)/prerequisites.sh" remaster

# ---- setup: sources (stamped so they run once) ------------------------------
sources: kernels gnome-source

kernels: $(KERNELS_STAMP)
$(KERNELS_STAMP):
	bash "$(ROOT)/kernels/git.sh" $(KERNEL_VERSIONS)
	@mkdir -p "$(STAMP_DIR)" && touch "$@"

gnome-source: $(GNOME_STAMP)
$(GNOME_STAMP):
	bash "$(ROOT)/gnome-source/pull-all-source.sh"
	@mkdir -p "$(STAMP_DIR)" && touch "$@"

# ---- components -------------------------------------------------------------
native:
	bash "$(HERE)/native-build.sh"

chromium:
	bash "$(HERE)/chromium/build-chromium.sh"

bootstrap:
	bash "$(HERE)/bootstrap-native.sh"

remaster:
	@test -n "$(ISO)" || { echo "ERROR: set ISO=/path/to/ubuntu.iso"; exit 2; }
	bash "$(HERE)/quick-remaster.sh" "$(ISO)" $(if $(OUT),-o "$(OUT)",)

# ---- housekeeping -----------------------------------------------------------
clean:
	rm -rf "$(WORK)" "$(OUTPUT_DIR)"

distclean: clean
	rm -rf "$(STAMP_DIR)"
	@echo "Removed setup stamps; next build will re-run kernels/gnome-source fetch."
