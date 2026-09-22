# MirvkBuntu — top-level build orchestrator.
#
# Thin wrappers over the authoritative build scripts (build/*.sh, kernels/git.sh,
# gnome-source/pull-all-source.sh). The scripts remain the source of truth; this
# Makefile just gives ordered, discoverable entry points.
#
# Quick start:
#   make help            # list targets
#   sudo make prereqs    # install host build tooling (native path)
#   make sources         # fetch kernel + GNOME source (no root needed)
#   sudo make slim       # build the slim (toram/GNOME) ISO
#   sudo make desktop    # build the desktop ISO
#
# Flags (see build/BUILD.FLAGS.md) pass straight through, e.g.:
#   sudo make slim JOBS=8 BUILD_SKIP_CHROMIUM=1

SHELL := /bin/bash
ROOT  := $(CURDIR)

# Kernel versions fetched by `make kernels` (override on the command line).
KERNEL_VERSIONS ?=

.PHONY: all help prereqs prereqs-remaster kernels gnome-source chromium installer \
        sources native desktop slim minimal iso remaster bootstrap \
        packages packages-clean clean distclean

all: help

installer:
	@bash build/installer.sh

help:
	@echo "MirvkBuntu build targets:"
	@echo ""
	@echo "  Setup (run once, needs network; prereqs needs root):"
	@echo "    make prereqs           Install host tooling for the native build path"
	@echo "    make prereqs-remaster  Install host tooling for the quick remaster path"
	@echo "    make kernels           Fetch real Linux kernel source into kernels/"
	@echo "    make gnome-source      Populate every GNOME module source/ (pinned)"
	@echo "    make sources           kernels + gnome-source"
	@echo ""
	@echo "  Build (needs root):"
	@echo "    make desktop           Full desktop ISO      -> build/output/MirvkBuntu-desktop-amd64.iso"
	@echo "    make slim              Slim toram/GNOME ISO   -> build/output/MirvkBuntu-slim-amd64.iso"
	@echo "    make minimal           Minimal ISO           -> build/output/MirvkBuntu-minimal-amd64.iso"
	@echo "    make iso               Alias for 'desktop'"
	@echo "    make bootstrap         Fetch kernel then build desktop (build/bootstrap-native.sh)"
	@echo "    make remaster ISO=x    Remaster a stock Ubuntu ISO (path A, no compilation)"
	@echo ""
	@echo "  Components:"
	@echo "    make native            Run the native compilation gate only"
	@echo "    make chromium          Build Chromium only (DESTDIR=... to install)"
	@echo "    make installer         Build and package the native MirvkBuntu installer"
	@echo "    make packages          Build the compilable packages (installer)"
	@echo ""
	@echo "  Housekeeping:"
	@echo "    make clean             Remove build/work and build/output"
	@echo "    make distclean         clean + remove fetched kernel/GNOME source"
	@echo ""
	@echo "  Flags pass through (see build/BUILD.FLAGS.md), e.g.:"
	@echo "    sudo make slim JOBS=8 BUILD_SKIP_CHROMIUM=1"

# ---- setup ------------------------------------------------------------------
prereqs:
	bash "$(ROOT)/build/prerequisites.sh" native

prereqs-remaster:
	bash "$(ROOT)/build/prerequisites.sh" remaster

kernels:
	bash "$(ROOT)/kernels/git.sh" $(KERNEL_VERSIONS)

gnome-source:
	bash "$(ROOT)/gnome-source/pull-all-source.sh"

sources: kernels gnome-source

# ---- component builds -------------------------------------------------------
native:
	bash "$(ROOT)/build/native-build.sh"

chromium:
	bash "$(ROOT)/build/chromium/build-chromium.sh"

# Build the compilable subprojects under packages/ (installer, etc.).
packages:
	$(MAKE) -C "$(ROOT)/packages" all

packages-clean:
	$(MAKE) -C "$(ROOT)/packages" clean

# ---- ISO builds -------------------------------------------------------------
desktop:
	bash "$(ROOT)/build/build-desktop.sh"

iso: desktop

slim:
	bash "$(ROOT)/build/build-slim.sh"

minimal:
	bash "$(ROOT)/build/build-minimal.sh"

bootstrap:
	bash "$(ROOT)/build/bootstrap-native.sh"

# Remaster a stock Ubuntu ISO. Usage: make remaster ISO=/path/to/ubuntu.iso
remaster:
	@test -n "$(ISO)" || { echo "ERROR: set ISO=/path/to/ubuntu.iso"; exit 2; }
	bash "$(ROOT)/build/quick-remaster.sh" "$(ISO)" $(if $(OUT),-o "$(OUT)",)

# ---- housekeeping -----------------------------------------------------------
clean:
	rm -rf "$(ROOT)/build/work" "$(ROOT)/build/output"

distclean: clean
	@echo "Removing fetched kernel source trees under kernels/ ..."
	@find "$(ROOT)/kernels" -maxdepth 1 -type d -name 'linux-*' -exec rm -rf {} + 2>/dev/null || true
	@echo "Fetched GNOME module source/ trees are left in place (large; remove manually if desired)."
