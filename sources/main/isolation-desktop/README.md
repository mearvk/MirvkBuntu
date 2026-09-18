# Ubuntu White Isolation Desktop

A lightweight developer preview of the Ubuntu White desktop idiom.

## Purpose

Isolation Desktop is a **desktop application preview**, not an ISO builder and not a virtual machine. It lets GUI developers compile and launch the desktop shell locally so they can iterate on look-and-feel without rebooting or installing the operating system.

The current interactive reference implementation is `org.ubuntu.white.desktop.DesktopSynthesizer`. See [`DESKTOP_SYNTHESIZER.md`](DESKTOP_SYNTHESIZER.md) for its support specification.

## Desktop Synthesizer

The Desktop Synthesizer is intended to behave like a Linux desktop shell while remaining an independently runnable JavaFX application:

- Fullscreen JavaFX presentation.
- Ubuntu White wallpaper scaled proportionally to fill the desktop.
- Draggable desktop icons with deterministic grid snapping.
- Host-file drag-and-drop onto the desktop surface.
- Simple top and bottom desktop chrome for the visual preview.
- Ubuntu White `set-002` icons are the canonical desktop icon set.
- The desktop includes one Smaug desktop identity using one canonical image.

## Icon Architecture

The desktop consumes the newly normalized Ubuntu White assets from:

```text
ubuntu-white/icons/set-002/
```

The twelve standard desktop icons are loaded from that directory. The previous `set-001` reference is no longer the desktop source.

Smaug has a deliberately singular visual identity. The desktop uses **only**:

```text
ubuntu-white/icons/smaug/smaug-icon-001.jpeg
```

This is the canonical Smaug image for the JavaFX desktop. The other Smaug images in the repository are not selected by the desktop and are not rotated or substituted at runtime. If Smaug is represented more than once in future desktop surfaces, the same `smaug-icon-001.jpeg` asset must be reused rather than introducing another Smaug image.

## Linux Desktop Behavior

The JavaFX GUI is structured as a desktop shell rather than a conventional centered application window. The desktop surface owns the wallpaper and icon layer; the icon layer supports direct manipulation, front-to-back ordering, grid snapping, and external file drops. This establishes the interaction model for the eventual Ubuntu White desktop while keeping the implementation safe to run on a host Linux system.

Current reference behavior:

- Fullscreen desktop surface.
- Wallpaper behind the desktop icon layer.
- Desktop icons positioned in a regular grid.
- Mouse press/drag/release for icon movement.
- Grid snapping after movement.
- External files accepted by drag-and-drop.
- Escape or Ctrl+Tab exits the preview.

## Developer workflow

```text
source → compile → desktop preview → edit → compile → preview
```

Run the current synthesizer with:

```text
gradle -b desktop-synthesizer.gradle run
```

or:

```text
./launch-synthesizer
```

The dedicated Gradle configuration targets Java 21 and JavaFX 21.0.8.

The eventual Ubuntu White OS installer may consume the same GUI assets and source, but the preview remains independently runnable.

## Isolation principles

- No bootloader changes.
- No disk repartitioning.
- No ISO creation required.
- No filesystem installation required.
- Preview state remains separate from the host OS.
- Production packaging is a separate build target.

## Planned entry points

`compile-desktop` — compile the desktop preview.

`launch-desktop` — launch the compiled preview.

`Makefile` — convenient developer build/run interface.

`launch-synthesizer` — launch the current Gradle Desktop Synthesizer.

The implementation uses JavaFX as the current Ubuntu White GUI technology.

## Next support work

- Persist desktop icon positions.
- Add double-click/open behavior.
- Add selection states and context menus.
- Package wallpaper and Ubuntu White `set-002` icons as application resources for distribution.
- Integrate the synthesizer with the boot-to-Desktop startup path.
- Add JavaFX build/smoke checks to CI.
