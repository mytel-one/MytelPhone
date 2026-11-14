const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const root = join(__dirname, '../../Linphone/view');
const read = file => readFileSync(join(root, file), 'utf8');
const paletteSource = read('Style/DefaultStyle.qml');
function palette(dark) {
    return new Proxy({}, { get: (_, key) => {
        if (key === 'dark') return dark;
        const match = paletteSource.match(new RegExp(`(?:readonly )?property (?:var|color) ${key}: ([^\\r\\n]+)`));
        assert.ok(match, `production color ${key} exists`);
        return vm.runInNewContext(match[1], { dark });
    }});
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
const callPage = read('Page/Main/Call/CallPage.qml');
const history = callPage.slice(callPage.indexOf('id: detailListBackground'));
const historyBackground = history.match(/color: DefaultStyle\.(\w+)/)[1];
const historyDateExpression = history.slice(history.indexOf('UtilsCpp.formatDate'))
    .match(/color: ([^\r\n]+)/)[1];
const editors = [...read('Page/Form/Contact/ContactEdition.qml')
    .matchAll(/backgroundColor: DefaultStyle\.(\w+)/g)].map(m => m[1]);
const banner = read('Control/Container/Contact/ContactLayout.qml')
    .match(/GradientStop \{ position: 0.0; color: DefaultStyle\.(\w+) \}/g)
    .map(m => m.match(/DefaultStyle\.(\w+)/)[1]).at(-1);
const noteBackground = read('Control/Container/Contact/PresenceNoteLayout.qml')
    .match(/color: DefaultStyle\.(\w+)/)[1];
const noteText = [...read('Control/Container/Contact/PresenceNoteLayout.qml')
    .matchAll(/color: DefaultStyle\.(\w+)/g)].at(-1)[1];
const creation = read('Control/Container/CreationFormLayout.qml');
const creationBackgrounds = [...creation.matchAll(/GradientStop \{ position: [01].0; color: DefaultStyle\.(\w+)/g)]
    .map(m => m[1]);
const creationText = creation.slice(creation.indexOf('text: mainItem.startGroupButtonText'))
    .match(/color: DefaultStyle\.(\w+)/)[1];
for (const dark of [false, true]) {
    const p = palette(dark), mode = dark ? 'dark' : 'light';
    test(`${mode}: history card labels stay readable`, () => readable(p.main2_600, p[historyBackground]));
    for (const missed of [false, true]) {
        test(`${mode}: history ${missed ? 'missed' : 'normal'} date stays readable`, () => {
            const color = vm.runInNewContext(historyDateExpression, { DefaultStyle: p,
                modelData: { core: { status: missed ? 1 : 0 } }, LinphoneEnums: { CallStatus: { Missed: 1 } } });
            readable(color, p[historyBackground]);
        });
    }
    test(`${mode}: contact editor fields keep entered text readable`, () => {
        assert.equal(editors.length, 8);
        for (const key of editors) readable(p.main2_600, p[key]);
    });
    test(`${mode}: contact banner stays readable`, () => readable(p.main2_600, p[banner]));
    test(`${mode}: presence note title and body stay readable`, () => {
        readable(p.main2_600, p[noteBackground]);
        readable(p[noteText], p[noteBackground]);
    });
    test(`${mode}: creation card label stays readable across its gradient`, () => {
        assert.equal(creationBackgrounds.length, 2);
        for (const key of creationBackgrounds) readable(p[creationText], p[key]);
    });
}
for (const file of ['Page/Main/Call/CallPage.qml', 'Control/Input/SearchBar.qml']) {
    test(`${file}: dialer has a readable accessible name`, () => {
        assert.match(read(file), /Accessible.name: qsTr\("Open dialer"\)/);
    });
}
