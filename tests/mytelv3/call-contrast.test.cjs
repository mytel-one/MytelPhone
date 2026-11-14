const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const root = join(__dirname, '../../Linphone/view');
const paletteSource = readFileSync(join(root, 'Style/DefaultStyle.qml'), 'utf8');
const windowSource = readFileSync(join(root, 'Page/Window/Call/CallsWindow.qml'), 'utf8');
const stickerSource = readFileSync(join(root, 'Control/Display/Sticker.qml'), 'utf8');
function palette(dark) {
    return new Proxy({}, {get: (_, key) => {
        const match = paletteSource.match(new RegExp(`(?:readonly )?property (?:var|color) ${key}: ([^\\r\\n]+)`));
        assert.ok(match, `production color ${key} exists`);
        return vm.runInNewContext(match[1], {dark});
    }});
}
function luminance(hex) {
    const rgb = [1,3,5].map(i => parseInt(hex.slice(i, i + 2), 16) / 255)
        .map(v => v <= 0.04045 ? v / 12.92 : ((v + 0.055) / 1.055) ** 2.4);
    return rgb[0]*0.2126 + rgb[1]*0.7152 + rgb[2]*0.0722;
}
function whiteContrast(color) { return 1.05 / (luminance(color) + 0.05); }
const backgroundKey = windowSource.slice(windowSource.indexOf('/************************* CONTENT'))
    .match(/color: DefaultStyle\.(\w+)/)[1];
const stickerKey = stickerSource.match(/property color color: DefaultStyle\.(\w+)/)[1];
const buttonColor = windowSource.slice(windowSource.indexOf('component BottomButton:'))
    .match(/color: (bottomButton.enabled[^\r\n]+)/)[1];
for (const dark of [false, true]) {
    const mode = dark ? 'dark' : 'light';
    for (const [name, key] of [['call window', backgroundKey], ['participant tile', stickerKey]]) {
        test(`${mode}: ${name} keeps white call labels readable`, () => {
            const ratio = whiteContrast(palette(dark)[key]);
            assert.ok(ratio >= 4.5, `${key}: contrast ${ratio.toFixed(2)}`);
        });
    }
    for (const [state, enabled, pressed, checked] of [
        ['normal', true, false, false], ['pressed', true, true, false],
        ['checked', true, false, true], ['disabled', false, false, false]]) {
        test(`${mode}: ${state} call control keeps its icon readable`, () => {
            const color = vm.runInNewContext(buttonColor, {DefaultStyle: palette(dark),
                bottomButton: {enabled, pressed, checked}, disabledIcon: ''});
            const ratio = whiteContrast(color);
            assert.ok(ratio >= 3, `control ${state}: contrast ${ratio.toFixed(2)}`);
        });
    }
}
