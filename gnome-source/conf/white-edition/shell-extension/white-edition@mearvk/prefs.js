// Ubuntu White Edition Panel preferences.
//
// Presents the two settings in GNOME Extensions / Control Center:
//   - start-menu-alignment: Left / Center / Right (popup anchoring only)
//   - start-button-logo: which bundled Ubuntu-themed glyph the Start button uses
//
// Both rows are bound to GSettings so changes apply through the extension's
// changed:: handlers.

import Adw from 'gi://Adw';
import Gio from 'gi://Gio';
import Gtk from 'gi://Gtk';

import {ExtensionPreferences} from 'resource:///org/gnome/Shell/Extensions/js/extensions/prefs.js';

// Ordered to match the enum nick order in the gschema for both keys.
const ALIGNMENT_VALUES = ['left', 'center', 'right'];
const ALIGNMENT_LABELS = ['Left', 'Center', 'Right'];

const LOGO_VALUES = ['circle-of-friends', 'mono-accent', 'focus-ring'];
const LOGO_LABELS = ['Circle of Friends', 'Mono Accent', 'Focus Ring'];

export default class WhiteEditionPanelPreferences extends ExtensionPreferences {
    fillPreferencesWindow(window) {
        const settings = this.getSettings();

        const page = new Adw.PreferencesPage({
            title: 'White Edition Panel',
            icon_name: 'preferences-desktop-symbolic',
        });

        const group = new Adw.PreferencesGroup({
            title: 'Start Menu',
            description: 'The Start button stays fixed at the left of the panel. Only the Start menu popup and the button logo are configurable.',
        });
        page.add(group);

        // start-menu-alignment row.
        const alignmentModel = new Gtk.StringList();
        for (const label of ALIGNMENT_LABELS)
            alignmentModel.append(label);

        const alignmentRow = new Adw.ComboRow({
            title: 'Start menu alignment',
            subtitle: 'Where the Start menu popup opens: left, center, or right.',
            model: alignmentModel,
        });
        alignmentRow.set_selected(
            Math.max(0, ALIGNMENT_VALUES.indexOf(
                settings.get_string('start-menu-alignment'))));
        alignmentRow.connect('notify::selected', row => {
            settings.set_string('start-menu-alignment',
                ALIGNMENT_VALUES[row.get_selected()]);
        });
        settings.connect('changed::start-menu-alignment', () => {
            alignmentRow.set_selected(
                Math.max(0, ALIGNMENT_VALUES.indexOf(
                    settings.get_string('start-menu-alignment'))));
        });
        group.add(alignmentRow);

        // start-button-logo row.
        const logoModel = new Gtk.StringList();
        for (const label of LOGO_LABELS)
            logoModel.append(label);

        const logoRow = new Adw.ComboRow({
            title: 'Start button logo',
            subtitle: 'Which Ubuntu-themed logo the Start button uses.',
            model: logoModel,
        });
        logoRow.set_selected(
            Math.max(0, LOGO_VALUES.indexOf(
                settings.get_string('start-button-logo'))));
        logoRow.connect('notify::selected', row => {
            settings.set_string('start-button-logo',
                LOGO_VALUES[row.get_selected()]);
        });
        settings.connect('changed::start-button-logo', () => {
            logoRow.set_selected(
                Math.max(0, LOGO_VALUES.indexOf(
                    settings.get_string('start-button-logo'))));
        });
        group.add(logoRow);

        window.add(page);

        // Keep a reference so the settings object lives as long as the window.
        window._settings = settings;
        void Gio;
    }
}
