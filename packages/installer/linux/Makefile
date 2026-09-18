.PHONY: all clean install uninstall install-user-bin install-user-runnables install-usr-bin install-usr-runnables install-deck-bin install-deck-runnables update-path update-front-link nxtt nxtt-install nxtt-uninstall nxtt-clean nxtt-test white-installer white-installer-static package-installer-install package-installer-uninstall

CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
TARGET := desktop_install_probe
SRC := desktop_install_probe.c
NXTT_TARGET := nxtt
NXTT_SRC := nxtt-uninstaller.c

# White Edition smooth install orchestrator ELF (the "usual" front door).
# It wraps and delegates to scripts/galactic-cherry-installer; it contains no
# disk/system-provisioning logic of its own.
WIO_TARGET := white-installer
WIO_SRC := white_installer_orchestrator.c
WIO_HDR := white_installer_orchestrator.h

# package-installer: installs the repository's package software directly into
# the /user and /deck trees, selected by disc (a named bundle) or by function
# (a role/keyword). Unlike white-installer it does not delegate; it copies the
# resolved artifacts itself. Default run is a safe dry-run.
PKG_TARGET := package-installer
PKG_SRC := package_installer.c
PKG_HDR := package_installer.h desktop_install_probe.h

USER_ROOT ?= /user
USR_ROOT ?= /usr
DECK_ROOT ?= /deck
INSTALL_CHOICE ?= user-bin
FRONT_LINK ?= ../../desktop_install_probe
NXTT_BIN := $(USER_ROOT)/bin
NXTT_RUNNABLES := $(USER_ROOT)/runnables

USER_BIN := $(USER_ROOT)/bin
USER_RUNNABLES := $(USER_ROOT)/runnables
USR_BIN := $(USR_ROOT)/bin
USR_RUNNABLES := $(USR_ROOT)/runnables
DECK_BIN := $(DECK_ROOT)/bin
DECK_RUNNABLES := $(DECK_ROOT)/runnables

all: $(TARGET) $(NXTT_TARGET) $(WIO_TARGET) $(PKG_TARGET)

$(TARGET): $(SRC) desktop_install_probe.h
	$(CC) $(CFLAGS) -o $@ $(SRC)

$(NXTT_TARGET): $(NXTT_SRC)
	$(CC) $(CFLAGS) -o $@ $(NXTT_SRC)

# Default (in-sandbox validated) build: normal dynamic link. -static-libgcc is
# fine here; full -static is provided as a separate portable target below.
$(WIO_TARGET): $(WIO_SRC) $(WIO_HDR) desktop_install_probe.h
	$(CC) $(CFLAGS) -static-libgcc -o $@ $(WIO_SRC)

# Portable/static build for the live ISO and minimal environments where a
# dynamic loader / shared libc may be unavailable. NOTE: full `-static` needs a
# static libc, which is NOT present in the restricted build sandbox
# ("cannot find -lc"); this target is expected to link on a full toolchain and
# MAY FAIL in the sandbox. That failure is acceptable — use the default
# `white-installer` target for in-sandbox compile validation.
white-installer-static: $(WIO_SRC) $(WIO_HDR) desktop_install_probe.h
	$(CC) $(CFLAGS) -static -o $(WIO_TARGET) $(WIO_SRC)

# Build package-installer (normal dynamic link, matches white-installer).
$(PKG_TARGET): $(PKG_SRC) $(PKG_HDR)
	$(CC) $(CFLAGS) -static-libgcc -o $@ $(PKG_SRC)

# Install package-installer into BOTH the /user and /deck bin trees.
package-installer-install: $(PKG_TARGET)
	install -d $(USER_BIN) $(DECK_BIN)
	install -m 0755 $(PKG_TARGET) $(USER_BIN)/$(PKG_TARGET)
	install -m 0755 $(PKG_TARGET) $(DECK_BIN)/$(PKG_TARGET)
	@echo "Installed $(PKG_TARGET) to $(USER_BIN) and $(DECK_BIN)"

package-installer-uninstall:
	rm -f $(USER_BIN)/$(PKG_TARGET) $(DECK_BIN)/$(PKG_TARGET)

install: $(TARGET)
	@case "$(INSTALL_CHOICE)" in \
		user-bin) $(MAKE) install-user-bin ;; \
		user-runnables) $(MAKE) install-user-runnables ;; \
		*) echo "Usage: make install [INSTALL_CHOICE=user-bin|user-runnables]" >&2; exit 2 ;; \
	esac

install-user-bin: $(TARGET)
	install -d $(USER_BIN)
	install -m 0755 $(TARGET) $(USER_BIN)/$(TARGET)
	@$(MAKE) update-path DIR=$(USER_BIN)
	@$(MAKE) update-front-link TARGET_DIR=$(USER_BIN)

install-user-runnables: $(TARGET)
	install -d $(USER_RUNNABLES)
	install -m 0755 $(TARGET) $(USER_RUNNABLES)/$(TARGET)
	@$(MAKE) update-path DIR=$(USER_RUNNABLES)
	@$(MAKE) update-front-link TARGET_DIR=$(USER_RUNNABLES)

install-usr-bin: $(TARGET)
	install -d $(USR_BIN)
	install -m 0755 $(TARGET) $(USR_BIN)/$(TARGET)
	@$(MAKE) update-front-link TARGET_DIR=$(USR_BIN)

install-usr-runnables: $(TARGET)
	install -d $(USR_RUNNABLES)
	install -m 0755 $(TARGET) $(USR_RUNNABLES)/$(TARGET)
	@$(MAKE) update-front-link TARGET_DIR=$(USR_RUNNABLES)

install-deck-bin: $(TARGET)
	install -d $(DECK_BIN)
	install -m 0755 $(TARGET) $(DECK_BIN)/$(TARGET)
	@$(MAKE) update-front-link TARGET_DIR=$(DECK_BIN)

install-deck-runnables: $(TARGET)
	install -d $(DECK_RUNNABLES)
	install -m 0755 $(TARGET) $(DECK_RUNNABLES)/$(TARGET)
	@$(MAKE) update-front-link TARGET_DIR=$(DECK_RUNNABLES)

nxtt: $(NXTT_TARGET)

nxtt-install: $(NXTT_TARGET)
	install -d $(NXTT_BIN) $(NXTT_RUNNABLES)
	install -m 0755 $(NXTT_TARGET) $(NXTT_BIN)/$(NXTT_TARGET)
	install -m 0755 $(NXTT_TARGET) $(NXTT_RUNNABLES)/$(NXTT_TARGET)
	@echo "Installed $(NXTT_TARGET) to $(NXTT_BIN) and $(NXTT_RUNNABLES)"

nxtt-uninstall:
	rm -f $(NXTT_BIN)/$(NXTT_TARGET) $(NXTT_RUNNABLES)/$(NXTT_TARGET)

nxtt-test: $(NXTT_TARGET)
	./$(NXTT_TARGET) >/dev/null

nxtt-clean:
	rm -f $(NXTT_TARGET)

update-path:
	@if [ -n "$(DIR)" ] && [ -n "$$HOME" ]; then \
		profile="$$HOME/.profile"; \
		path_line='export PATH="$(DIR):$$PATH"'; \
		if ! grep -Fq '$(DIR)' "$$profile" 2>/dev/null; then \
			echo '' >> "$$profile"; \
			echo '# Ubuntu Determinant installer path' >> "$$profile"; \
			echo "$$path_line" >> "$$profile"; \
			echo "[ok] added $(DIR) to $$profile"; \
		else \
			echo "[ok] $(DIR) is already in $$profile"; \
		fi; \
	fi

update-front-link:
	@link="$(FRONT_LINK)"; \
	target="$(TARGET_DIR)/$(TARGET)"; \
	if [ -x "$$target" ]; then \
		rm -f "$$link"; \
		ln -s "$$target" "$$link"; \
		echo "[ok] front link: $$link -> $$target"; \
	elif [ -f ../../install-native.sh ]; then \
		rm -f "$$link"; \
		ln -s installer/install-native.sh "$$link"; \
		echo "[ok] front link fallback: $$link -> installer/install-native.sh"; \
	else \
		echo "[warn] no executable or install script available for front link"; \
	fi

clean:
	rm -f $(TARGET) $(NXTT_TARGET) $(WIO_TARGET) $(PKG_TARGET)

uninstall:
	rm -f $(USER_BIN)/$(TARGET) $(USER_RUNNABLES)/$(TARGET) \
	      $(USR_BIN)/$(TARGET) $(USR_RUNNABLES)/$(TARGET) \
	      $(DECK_BIN)/$(TARGET) $(DECK_RUNNABLES)/$(TARGET) \
	      $(USER_BIN)/$(WIO_TARGET) $(USER_RUNNABLES)/$(WIO_TARGET) \
	      $(USR_BIN)/$(WIO_TARGET) $(USR_RUNNABLES)/$(WIO_TARGET) \
	      $(DECK_BIN)/$(WIO_TARGET) $(DECK_RUNNABLES)/$(WIO_TARGET) \
	      $(USER_BIN)/$(PKG_TARGET) $(DECK_BIN)/$(PKG_TARGET) \
	      $(FRONT_LINK)
