// Ubuntu White Edition Panel extension.
//
// Behavior contract:
//   - The Start BUTTON is always placed at the left of the panel and never
//     moves. It is inserted at the left box, position 0, unconditionally.
//   - Only the Start MENU POPUP's horizontal anchoring changes, driven by the
//     start-menu-alignment GSettings key (left | center | right).
//   - The button glyph is chosen from three bundled logos via start-button-logo.
//
// This layer is additive and reversible: enable() creates one PanelMenu.Button
// and connects setting handlers; disable() destroys the button and clears state.
//
// Why the alignment override wraps BoxPointer._reposition instead of reacting to
// the menu 'open-state-changed' signal:
//
//   PopupMenu.open() (js/ui/popupMenu.js) emits 'open-state-changed' FIRST (via
//   super.open()) and only AFTERWARD calls
//   this._boxPointer.setPosition(this.sourceActor, this._arrowAlignment), which
//   queues a relayout. The final popup geometry is then computed inside
//   BoxPointer.vfunc_allocate -> _reposition(box) (js/ui/boxpointer.js), which
//   derives the origin from the source actor extents and _arrowAlignment and
//   ends with allocationBox.set_origin(...). Because position is applied through
//   the allocation box (not the actor's x property) and recomputed on every
//   relayout, anything set from the open signal (actor.set_x / setArrowOrigin)
//   is clobbered on the same open cycle, so center/right never took effect.
//
//   To survive the allocation cycle we wrap the boxpointer instance's
//   _reposition so our horizontal override runs AFTER the Shell has computed the
//   default origin, on every relayout. We let the Shell own the arrow origin and
//   only shift the allocation box X, translating the target into the parent's
//   coordinate space exactly as _reposition does (transform_stage_point).

import GObject from 'gi://GObject';
import Gio from 'gi://Gio';
import St from 'gi://St';

import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import * as PanelMenu from 'resource:///org/gnome/shell/ui/panelMenu.js';
import * as PopupMenu from 'resource:///org/gnome/shell/ui/popupMenu.js';
import {Extension} from 'resource:///org/gnome/shell/extensions/extension.js';

const StartButton = GObject.registerClass(
class StartButton extends PanelMenu.Button {
    _init(extension) {
        // menuAlignment 0.0 keeps the boxpointer arrow at the left edge of the
        // button; the White Edition alignment override below repositions the
        // popup horizontally after the Shell has applied that default.
        super._init(0.0, 'Ubuntu White Edition Start', false);

        this._extension = extension;
        this._settings = extension.getSettings();

        this.add_style_class_name('we-start-button');

        this._icon = new St.Icon({
            style_class: 'we-start-icon',
            icon_size: 24,
        });
        this.add_child(this._icon);
        this._reloadLogo();

        // Populate the Start menu popup. Content is intentionally minimal here;
        // the White Edition Start menu items are layered on top of this popup.
        this.menu.addMenuItem(
            new PopupMenu.PopupMenuItem('Ubuntu White Edition'));
        this.menu.addMenuItem(new PopupMenu.PopupSeparatorMenuItem());
        this.menu.addMenuItem(
            new PopupMenu.PopupMenuItem('Activities Overview')).connect(
            'activate', () => Main.overview.toggle());

        // Install the horizontal-alignment override on the popup's boxpointer.
        // This wraps BoxPointer._reposition so the override runs during every
        // allocation, which is the only point where the popup position is
        // authoritative (see the file header for why the open signal is too
        // early). The override reads the live setting each time it runs, so a
        // changed:: handler for the alignment key only needs to trigger a
        // relayout rather than recompute geometry itself.
        this._installAlignmentOverride();

        // When the alignment key changes while the popup is open, queue a
        // relayout so the wrapped _reposition re-applies with the new value.
        // The Start button position never depends on this key.
        this._alignmentChangedId = this._settings.connect(
            'changed::start-menu-alignment', () => {
                const boxPointer = this.menu._boxPointer;
                if (boxPointer && this.menu.isOpen)
                    boxPointer.queue_relayout();
            });

        this._logoChangedId = this._settings.connect(
            'changed::start-button-logo', () => this._reloadLogo());
    }

    _reloadLogo() {
        // start-button-logo selects which bundled SVG is drawn on the button.
        const logo = this._settings.get_string('start-button-logo');
        const path = this._extension.dir
            .get_child('logos')
            .get_child(`${logo}.svg`)
            .get_path();
        this._icon.set_gicon(Gio.icon_new_for_string(path));
    }

    _installAlignmentOverride() {
        // Wrap the popup boxpointer's _reposition so our horizontal override
        // runs after the Shell has computed the default allocation origin, on
        // every relayout. We keep a reference to the original so disable() can
        // restore it and leave the boxpointer untouched.
        const boxPointer = this.menu._boxPointer;
        if (!boxPointer || this._origReposition)
            return;

        this._alignedBoxPointer = boxPointer;
        this._origReposition = boxPointer._reposition;

        const self = this;
        boxPointer._reposition = function (allocationBox) {
            // Let the Shell compute the default geometry first (origin, arrow
            // side, arrow origin). This owns the arrow origin so we never pass
            // an invalid value to setArrowOrigin.
            self._origReposition.call(this, allocationBox);
            self._applyHorizontalAlignment(this, allocationBox);
        };
    }

    _applyHorizontalAlignment(boxPointer, allocationBox) {
        // Only the popup moves. start-menu-alignment picks the horizontal
        // anchor of the boxpointer over the primary monitor work area. This
        // runs inside the boxpointer's own _reposition (during allocation), so
        // the value it sets is the authoritative one and is not clobbered by a
        // later relayout on the same open cycle.
        //
        // NOTE: this still cannot be exercised at runtime in this sandbox
        // (there is no running GNOME Shell). The geometry is computed against
        // the primary monitor work area and clamped to it, mirroring the clamp
        // the Shell's own _reposition applies.
        if (!this._settings)
            return;

        const alignment = this._settings.get_string('start-menu-alignment');

        // Left is the Shell default (popup anchored under the Start button at
        // the left edge), so leave the computed origin as-is.
        if (alignment === 'left')
            return;

        const monitorIndex = Main.layoutManager.primaryIndex;
        const workArea = Main.layoutManager.getWorkAreaForMonitor(monitorIndex);
        if (!workArea)
            return;

        // The Shell uses the boxpointer natural width and an -arrow-rise
        // padding when clamping to the work area; reuse the same padding so our
        // clamp matches the Shell's edge behavior.
        const themeNode = boxPointer.get_theme_node();
        const padding = themeNode.get_length('-arrow-rise');
        const [, , natWidth] = boxPointer.get_preferred_size();
        const menuWidth = natWidth > 0 ? natWidth : boxPointer.width;

        let targetX;
        switch (alignment) {
        case 'center':
            targetX = workArea.x + Math.floor((workArea.width - menuWidth) / 2);
            break;
        case 'right':
            targetX = workArea.x + workArea.width - (padding + menuWidth);
            break;
        default:
            return;
        }

        // Clamp so the popup never spills off the monitor work area, matching
        // the [workarea.x + padding, workarea.x + workarea.width - (padding +
        // natWidth)] range the Shell's _reposition uses.
        const minX = workArea.x + padding;
        const maxX = workArea.x + workArea.width - (padding + menuWidth);
        targetX = Math.max(minX, Math.min(targetX, Math.max(minX, maxX)));

        // _reposition set the origin via allocationBox.set_origin(x, y) in the
        // parent's coordinate space, transforming the stage-space result with
        // transform_stage_point. Translate our stage-space targetX the same way
        // and keep the Shell-computed Y untranslated by round-tripping the
        // existing origin back to stage space first.
        let parent = boxPointer.get_parent();
        if (!parent)
            return;

        // Current (Shell-computed) origin, in the parent's coordinate space.
        const curX = allocationBox.get_x();

        // Convert desired stage X into parent space. Walk up the parent chain
        // exactly like _reposition does, in case an ancestor is not yet mapped.
        let success = false;
        let px = targetX;
        let node = parent;
        while (!success && node) {
            [success, px] = node.transform_stage_point(targetX, 0);
            node = node.get_parent();
        }
        if (!success)
            return;

        // Shift the box horizontally to the aligned position, leaving Y as the
        // Shell computed it.
        const dx = Math.floor(px) - curX;
        if (dx !== 0)
            allocationBox.set_origin(curX + dx, allocationBox.get_y());
    }

    destroyButton() {
        // Restore the boxpointer's original _reposition so nothing of ours
        // lingers on the (soon to be destroyed) menu.
        if (this._alignedBoxPointer && this._origReposition) {
            this._alignedBoxPointer._reposition = this._origReposition;
        }
        this._alignedBoxPointer = null;
        this._origReposition = null;

        if (this._alignmentChangedId) {
            this._settings.disconnect(this._alignmentChangedId);
            this._alignmentChangedId = 0;
        }
        if (this._logoChangedId) {
            this._settings.disconnect(this._logoChangedId);
            this._logoChangedId = 0;
        }
        this._settings = null;
        this.destroy();
    }
});

export default class WhiteEditionPanelExtension extends Extension {
    enable() {
        this._button = new StartButton(this);

        // Always add the Start button to the panel LEFT box at position 0 so it
        // is fixed at the left of the panel regardless of the configured Start
        // menu alignment. Alignment only affects the popup, never this.
        Main.panel.addToStatusArea(this.uuid, this._button, 0, 'left');
    }

    disable() {
        if (this._button) {
            this._button.destroyButton();
            this._button = null;
        }
    }
}
