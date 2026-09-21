# MirvkBuntu build/ — module-level build targets.
#
# Wrappers over the build scripts in this directory. See build/BUILD.FLAGS.md
# for the environment flags every target accepts, and the repository-root
# Makefile for the full setup+build flow.

SHELL := /bin/bash
HERE  := $(CURDIR)

.PHONY: help prereqs prereqs-remaster native chromium desktop slim minimal iso \
        remaster bootstrap clean

help:
	@echo "build/ targets:"
	@echo "  make prereqs           build/prerequisites.sh native"
	@echo "  make prereqs-remaster  build/prerequisites.sh remaster"
	@echo "  make native            build/native-build.sh (compilation gate)"
	@echo "  make chromium          build/chromium/build-chromium.sh"
	@echo "  make desktop           build/build-desktop.sh"
	@echo "  make slim              build/build-slim.sh"
	@echo "  make minimal           build/build-minimal.sh"
	@echo "  make iso               alias for desktop"
	@echo "  make bootstrap         build/bootstrap-native.sh"
	@echo "  make remaster ISO=x    build/quick-remaster.sh"
	@echo "  make clean             remove work/ and output/"

prereqs:           ; bash "$(HERE)/prerequisites.sh" native
prereqs-remaster:  ; bash "$(HERE)/prerequisites.sh" remaster
native:            ; bash "$(HERE)/native-build.sh"
chromium:          ; bash "$(HERE)/chromium/build-chromium.sh"
desktop:           ; bash "$(HERE)/build-desktop.sh"
iso: desktop
slim:              ; bash "$(HERE)/build-slim.sh"
minimal:           ; bash "$(HERE)/build-minimal.sh"
bootstrap:         ; bash "$(HERE)/bootstrap-native.sh"

remaster:
	@test -n "$(ISO)" || { echo "ERROR: set ISO=/path/to/ubuntu.iso"; exit 2; }
	bash "$(HERE)/quick-remaster.sh" "$(ISO)" $(if $(OUT),-o "$(OUT)",)

clean:
	rm -rf "$(HERE)/work" "$(HERE)/output"
