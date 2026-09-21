# MirvkBuntu packages/ — build the compilable package subprojects.
#
# Not everything under packages/ is compiled:
#   * basic-packages.txt  — a list of DISTRO package NAMES installed into the
#     ISO by live-build; there is nothing to compile here.
#   * installer/          — the MirvkBuntu installer: a JavaFX app (Maven) plus
#     native C helpers (installer/linux/Makefile). Compilable.
#   * aptitude/           — a script-based installer layer (.sh/.ps1). Nothing
#     to compile; provided for completeness via the 'aptitude-check' target.
#
# See build/BUILD.FLAGS.md for flags. The ISO itself is built from the
# repository-root Makefile (make desktop / slim / minimal).

SHELL := /bin/bash
HERE  := $(CURDIR)

DESTDIR ?=
PREFIX  ?= /usr

.PHONY: help all installer installer-native installer-java aptitude-check clean

help:
	@echo "packages/ targets:"
	@echo "  make all               Build all compilable packages (installer)"
	@echo "  make installer         Build the installer (native C + JavaFX)"
	@echo "  make installer-native  Build only the native C installer bits"
	@echo "  make installer-java    Build only the JavaFX installer (Maven)"
	@echo "  make aptitude-check     Sanity-check the script-based aptitude layer"
	@echo "  make clean             Clean built package artifacts"
	@echo ""
	@echo "  Note: basic-packages.txt is a distro package-name manifest consumed"
	@echo "        by live-build; it is not compiled. Build the ISO from the repo"
	@echo "        root: 'make desktop' / 'make slim' / 'make minimal'."

all: installer

# ---- installer (native C + JavaFX) -----------------------------------------
installer: installer-native installer-java

installer-native:
	@if [ -f "$(HERE)/installer/linux/Makefile" ]; then \
	  echo "packages: building native installer (installer/linux)"; \
	  $(MAKE) -C "$(HERE)/installer/linux" all; \
	else \
	  echo "packages: no native installer Makefile; skipping"; \
	fi

installer-java:
	@if [ -f "$(HERE)/installer/pom.xml" ]; then \
	  if command -v mvn >/dev/null 2>&1 && command -v java >/dev/null 2>&1; then \
	    echo "packages: building JavaFX installer (Maven package)"; \
	    ( cd "$(HERE)/installer" && mvn -B clean package ); \
	  else \
	    echo "packages: ERROR: Java 21+ and Maven are required for the JavaFX installer" >&2; \
	    exit 2; \
	  fi; \
	else \
	  echo "packages: no installer/pom.xml; skipping JavaFX installer"; \
	fi

# ---- aptitude (script layer; nothing to compile) ---------------------------
aptitude-check:
	@echo "packages: aptitude is script-based; checking scripts parse..."
	@if [ -f "$(HERE)/aptitude/src/aptitude.sh" ]; then \
	  bash -n "$(HERE)/aptitude/src/aptitude.sh" && echo "  aptitude.sh: OK"; \
	else \
	  echo "  aptitude/src/aptitude.sh not found"; \
	fi

# ---- clean ------------------------------------------------------------------
clean:
	@if [ -f "$(HERE)/installer/linux/Makefile" ]; then $(MAKE) -C "$(HERE)/installer/linux" clean || true; fi
	@rm -rf "$(HERE)/installer/target" 2>/dev/null || true
	@echo "packages: cleaned."
