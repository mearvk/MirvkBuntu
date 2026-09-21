# MirvkBuntu kernels/ — kernel source acquisition.
#
# Wraps kernels/git.sh, which downloads real Linux kernel source from kernel.org
# into kernels/linux-<version>/linux-<version>/ (with Makefile+Kconfig+arch/ as
# required by build/native-build.sh). Default versions are defined in git.sh;
# override with VERSIONS.

SHELL := /bin/bash
HERE  := $(CURDIR)

# e.g. make fetch VERSIONS="6.12.110"
VERSIONS ?=

.PHONY: help fetch clean

help:
	@echo "kernels/ targets:"
	@echo "  make fetch                    Download the default kernel version(s)"
	@echo "  make fetch VERSIONS=\"6.12.110\"  Download specific version(s)"
	@echo "  make clean                    Remove fetched linux-* source trees"

fetch:
	bash "$(HERE)/git.sh" $(VERSIONS)

clean:
	@find "$(HERE)" -maxdepth 1 -type d -name 'linux-*' -exec rm -rf {} + 2>/dev/null || true
	@echo "Removed fetched kernel source trees."
