const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const root = join(__dirname, '../../Linphone/view');
const read = file => readFileSync(join(root, file), 'utf8');
const paletteSource = read('Style/DefaultStyle.qml');
function color(key, dark) {
    const match = paletteSource.match(new RegExp(`(?:readonly )?property (?:var|color) ${key}: ([^\\r\\n]+)`));
    assert.ok(match, `production color ${key} exists`);
    return vm.runInNewContext(match[1], { dark });
}
function luminance(hex) {
    const rgb = [1, 3, 5].map(i => parseInt(hex.slice(i, i + 2), 16) / 255)
        .map(v => v <= 0.04045 ? v / 12.92 : ((v + 0.055) / 1.055) ** 2.4);
    return rgb[0] * 0.2126 + rgb[1] * 0.7152 + rgb[2] * 0.0722;
}
function readable(foreground, background) {
    const a = luminance(foreground), b = luminance(background);
    const ratio = (Math.max(a, b) + 0.05) / (Math.min(a, b) + 0.05);
    assert.ok(ratio >= 4.5, `${foreground} on ${background}: ${ratio.toFixed(2)}`);
}
const surfaces = [
    ['call/main options', 'Control/Button/PopupButton.qml', 'id: popupBackground'],
    ['main search suggestions', 'Page/Layout/Main/MainLayout.qml', 'id: popupBg'],
    ['call settings header', 'Page/Main/Call/CallSettingsPanel.qml', 'id: headerBackground'],
    ['in-call DTMF input', 'Page/Window/Call/CallsWindow.qml', 'id: searchBar'],
    ['presence account popup', 'Control/Display/Contact/ContactStatusPopup.qml', 'popup.contentItem: FocusScope'],
    ['security dialog body', 'Control/Popup/Dialog/ZrtpAuthenticationDialog.qml', 'y: Math.round(parent.y + Utils.getSizeWithScreenRatio(85))'],
];
for (const dark of [false, true]) {
    const mode = dark ? 'dark' : 'light';
    for (const [name, file, marker] of surfaces) {
        test(`${mode}: ${name} keeps menu text readable`, () => {
            const block = read(file).slice(read(file).indexOf(marker));
            const background = block.slice(0, block.indexOf('Presence {') > -1 ? block.indexOf('Presence {') : 300)
                .match(/color: DefaultStyle\.(\w+)/)?.[1] || 'grey_0';
            readable(color('main2_600', dark), color(background, dark));
        });
    }
    test(`${mode}: call control search surfaces keep their input text readable`, () => {
        const backgrounds = [...read('Page/Window/Call/CallsWindow.qml')
            .matchAll(/searchBarColor: DefaultStyle\.(\w+)/g)];
        assert.equal(backgrounds.length, 3);
        for (const match of backgrounds) readable(color('main2_600', dark), color(match[1], dark));
    });
    for (const type of ['Call', 'Message']) {
        test(`${mode}: incoming ${type.toLowerCase()} notification keeps white text readable`, () => {
            const background = read(`Control/Popup/Notification/NotificationReceived${type}.qml`)
                .match(/backgroundColor: DefaultStyle\.(\w+)/)[1];
            readable('#FFFFFF', color(background, dark));
        });
    }
}
