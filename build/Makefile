# MirvkBuntu — build-directory master Makefile
SHELL := /bin/bash
HERE := $(CURDIR)
ROOT := $(abspath $(HERE)/..)
ARCH ?= amd64
OUTPUT_DIR := $(HERE)/output

.PHONY: all help health health-strict prereqs prereqs-remaster sources kernels gnome-source native chromium desktop full slim minimal iso all-profiles bootstrap remaster clean distclean
.DEFAULT_GOAL := full

all: full

help:
	@echo "MirvkBuntu build/"
	@echo "  make health        source/profile preflight"
	@echo "  make health-strict completed native-stage health gate"
	@echo "  make slim           Slim ISO"
	@echo "  make minimal        Minimum ISO"
	@echo "  make full           Full desktop ISO"
	@echo "  make all-profiles   all three profiles"

health:
	bash "$(HERE)/build-health.sh" preflight

health-strict:
	bash "$(HERE)/build-health.sh" strict

prereqs:
	bash "$(HERE)/prerequisites.sh" native

prereqs-remaster:
	bash "$(HERE)/prerequisites.sh" remaster

sources: kernels gnome-source

kernels:
	KERNEL_FORCE_FETCH="$(KERNEL_FORCE_FETCH)" bash "$(ROOT)/kernels/git.sh" $(KERNEL_VERSIONS)

gnome-source:
	bash "$(ROOT)/gnome-source/pull-all-source.sh"

native:
	bash "$(HERE)/native-build.sh"

chromium:
	bash "$(HERE)/chromium/build-chromium.sh"

desktop: health
	bash "$(HERE)/build-desktop.sh"

full: desktop

iso: full

slim: health
	bash "$(HERE)/build-slim.sh"

minimal: health
	bash "$(HERE)/build-minimal.sh"

all-profiles: slim minimal full

bootstrap:
	bash "$(HERE)/bootstrap-native.sh"

remaster:
	@test -n "$(ISO)" || { echo "ERROR: set ISO=/path/to/ubuntu.iso"; exit 2; }
	bash "$(HERE)/quick-remaster.sh" "$(ISO)" $(if $(OUT),-o "$(OUT)",)

clean:
	rm -rf "$(HERE)/work" "$(OUTPUT_DIR)"

distclean: clean
	rm -rf "$(HERE)/work/.stamps"
	@echo "Removed build work and setup stamps."