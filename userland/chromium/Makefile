# SPDX-License-Identifier: BSD-3-Clause
#
# Makefile for Chromium Browser - Galactic Cherry Marvell Edition 98
#
# Chromium is fetched at build time via shallow git clone (~5-8 GB).
# Building from source requires depot_tools, GN, and Ninja.
#
# Source: github.com/chromium/chromium (BSD-3-Clause)
# Copyright (C) 2026 MEARVK LLC

SHELL := /bin/bash

PREFIX  ?= /usr
DESTDIR ?=

SRC_DIR := chromium-src

.PHONY: all fetch build install clean help

all: fetch

help:
	@echo "Chromium Browser (Open Source)"
	@echo ""
	@echo "Targets:"
	@echo "  fetch       - Shallow clone Chromium source (~5-8 GB download)"
	@echo "  build       - Build Chromium from source (requires depot_tools)"
	@echo "  install     - Install built Chromium to DESTDIR"
	@echo "  clean       - Remove source and build artifacts"
	@echo ""
	@echo "Prerequisites for building:"
	@echo "  - depot_tools (gn, autoninja)"
	@echo "  - 16+ GB RAM, 100+ GB disk space"
	@echo "  - Build time: 2-4 hours on modern hardware"
	@echo ""
	@echo "For most users, install the prebuilt package instead:"
	@echo "  apt install chromium-browser"

# ==============================================================================
# Fetch source (shallow clone)
# ==============================================================================

fetch:
	bash fetch-chromium.sh "$(CURDIR)/$(SRC_DIR)"

# ==============================================================================
# Build from source (requires depot_tools in PATH)
# ==============================================================================

build: $(SRC_DIR)
	@if ! command -v gn >/dev/null 2>&1; then \
		echo "ERROR: 'gn' not found. Install depot_tools first:"; \
		echo "  git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git"; \
		echo "  export PATH=\$$PATH:/path/to/depot_tools"; \
		exit 1; \
	fi
	cd $(SRC_DIR) && \
		gn gen out/Release --args=' \
			is_debug=false \
			is_component_build=false \
			symbol_level=0 \
			enable_nacl=false \
			use_cups=true \
			use_dbus=true \
			use_gio=true \
			use_pulseaudio=true' && \
		autoninja -C out/Release chrome

# ==============================================================================
# Install
# ==============================================================================

install:
	@if [ ! -f "$(SRC_DIR)/out/Release/chrome" ]; then \
		echo "ERROR: Chromium not built. Run 'make build' first."; \
		exit 1; \
	fi
	@if [ -z "$(DESTDIR)" ]; then \
		echo "ERROR: Set DESTDIR (e.g., make install DESTDIR=/mnt/rootfs)"; \
		exit 1; \
	fi
	install -d $(DESTDIR)$(PREFIX)/lib/chromium
	install -m 755 $(SRC_DIR)/out/Release/chrome $(DESTDIR)$(PREFIX)/lib/chromium/
	install -d $(DESTDIR)$(PREFIX)/bin
	ln -sf $(PREFIX)/lib/chromium/chrome $(DESTDIR)$(PREFIX)/bin/chromium-browser
	@echo "Chromium installed to $(DESTDIR)$(PREFIX)/lib/chromium/"

# ==============================================================================
# Clean
# ==============================================================================

clean:
	rm -rf $(SRC_DIR)
