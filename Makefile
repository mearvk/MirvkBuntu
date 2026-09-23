# MirvkBuntu — top-level custom build orchestrator
SHELL := /bin/bash
ROOT  := $(CURDIR)
include $(ROOT)/build/base-config.mk
KERNEL_VERSIONS ?=

.PHONY: all help health health-strict prereqs prereqs-remaster kernels gnome-source chromium installer builder scout sources native limited desktop full slim minimal iso all-profiles remaster bootstrap packages packages-clean clean distclean

all: full

help:
	@echo "MirvkBuntu custom source build"
	@echo ""
	@echo "  make health          Preflight source/profile/build health"
	@echo "  make health-strict   Completed native-stage health gate"
	@echo "  make prereqs         Install native build host tooling"
	@echo "  make sources         Fetch kernel + GNOME source"
	@echo "  make slim            Build Slim ISO"
	@echo "  make minimal         Build Minimum ISO"
	@echo "  make full            Build Full desktop ISO"
	@echo "  make all-profiles    Build Slim, Minimum, then Full"
	@echo "  make installer       Build installer binary/packages"
	@echo "  make builder         Build native builder"
	@echo "  make native          Run native compilation gate"
	@echo ""
	@echo "The Slim/Minimum/Full pipeline never consumes an existing Ubuntu ISO."

health:
	bash "$(ROOT)/build/build-health.sh" preflight
	bash "$(ROOT)/build/component-inventory.sh"
	@echo "MirvkBuntu health: profile definitions and component inventory READY"

health-strict:
	bash "$(ROOT)/build/build-health.sh" strict

prereqs:
	bash "$(ROOT)/build/prerequisites.sh" native

prereqs-remaster:
	bash "$(ROOT)/build/prerequisites.sh" remaster

kernels:
	bash "$(ROOT)/kernels/git.sh" $(KERNEL_VERSIONS)

gnome-source:
	bash "$(ROOT)/gnome-source/pull-all-source.sh"

sources: kernels gnome-source

native:
	bash "$(ROOT)/build/native-build.sh"

chromium:
	bash "$(ROOT)/build/chromium/build-chromium.sh"

packages:
	$(MAKE) -C "$(ROOT)/packages" all

packages-clean:
	$(MAKE) -C "$(ROOT)/packages" clean

limited:
	bash "$(ROOT)/build/build-limited.sh"

desktop:
	bash "$(ROOT)/build/build-desktop.sh"

full: health
	bash "$(ROOT)/build/build-desktop.sh"

iso: full

slim: health
	bash "$(ROOT)/build/build-slim.sh"

minimal: health
	bash "$(ROOT)/build/build-minimal.sh"

all-profiles: slim minimal full

bootstrap:
	bash "$(ROOT)/build/bootstrap-native.sh"

installer:
	bash "$(ROOT)/build/installer.sh"
	@echo "Installer binary/package outputs: $(MIRVKBUNTU_INSTALLER_OUTPUT_ROOT)"

builder:
	bash "$(ROOT)/build/builder.sh"
	@echo "Builder output directory: $(MIRVKBUNTU_BUILDER_OUTPUT_ROOT)"

scout:
	bash "$(ROOT)/build/scout.sh"

remaster:
	@test -n "$(ISO)" || { echo "ERROR: set ISO=/path/to/ubuntu.iso"; exit 2; }
	bash "$(ROOT)/build/quick-remaster.sh" "$(ISO)" $(if $(OUT),-o "$(OUT)",)

clean:
	rm -rf "$(ROOT)/build/work" "$(ROOT)/build/output"

distclean: clean
	@echo "Removed build output/work. Fetched source trees are retained."
