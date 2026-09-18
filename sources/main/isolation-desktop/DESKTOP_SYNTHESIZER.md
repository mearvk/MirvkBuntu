# Ubuntu White Desktop Synthesizer

## Status

The JavaFX desktop preview is now implemented as `org.ubuntu.white.desktop.DesktopSynthesizer`.

It is the proposed full-window Ubuntu White desktop shell and is intended to be the reference implementation for the Gradle boot-to-Desktop path.

## Visual composition

- Fullscreen JavaFX window.
- Previously established Ubuntu White wallpaper is used as the desktop background when available.
- Wallpaper is scaled proportionally to fill the desktop frame.
- A restrained white top and bottom shell provides the proposed desktop chrome.
- The desktop uses the twelve reference assets in `images/desktop-icons/set-001/`.

## Icon reference

The synthesizer currently loads:

1. `icon-001.png`
2. `icon-002.png`
3. `icon-003.png`
4. `icon-004.png`
5. `icon-005.png`
6. `icon-006.png`
7. `icon-007.png`
8. `icon-008.png`
9. `icon-009.png`
10. `icon-010.png`
11. `icon-011.png`
12. `icon-012.png`

Icons are rendered at a uniform 64 px display size with aspect ratio preserved. The source artwork remains unchanged.

The corresponding design/reference document is `ubuntu-white/IconGreeting2.md`.

## Desktop interaction

### Local icon drag

Desktop icons can be grabbed and moved with the mouse. On release, the icon is snapped to the nearest desktop grid position. This keeps the arrangement orderly while still allowing the user to establish a personal layout.

### Grid alignment

The preview uses a deterministic grid:

- Horizontal spacing: 132 px
- Vertical spacing: 112 px
- Initial desktop margin: 28 px

These values are implementation defaults rather than a permanent UX specification and may be tuned as the desktop design develops.

### External drag-and-drop

The desktop surface accepts files dragged from the host environment. The current preview reports the received paths to standard output; later iterations can turn dropped files into persistent desktop objects, shortcuts, or application launchers.

## Gradle entry point

`desktop-synthesizer.gradle` provides the dedicated Gradle application configuration and selects:

`org.ubuntu.white.desktop.DesktopSynthesizer`

The preview targets Java 21 and JavaFX 21.0.8.

`launch-synthesizer` is provided as a convenience launcher for systems with Gradle installed.

## Relationship to IsolationDesktop

The existing `IsolationDesktop` remains as the earlier JavaFX desktop preview. `DesktopSynthesizer` is the newer implementation focused on the twelve current icon references, fullscreen presentation, interactive positioning, and external drag-and-drop.

## Next implementation stages

1. Persist icon positions between launches.
2. Replace placeholder desktop labels with the final desktop vocabulary.
3. Resolve wallpaper and icon assets through a packaged application-resource layout for distribution builds.
4. Add double-click/open behavior for desktop objects.
5. Add context menus and selection states.
6. Integrate the boot-to-Desktop synthesizer with the project's installer/startup path.
7. Add automated JavaFX smoke/build checks to CI.

## Design principle

The preview is deliberately a working desktop surface rather than a static mockup. The twelve icons, wallpaper, grid, and drag model are therefore treated as source-level UI references that can evolve into the production Ubuntu White desktop shell.
