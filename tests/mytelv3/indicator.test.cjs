const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const test = require('node:test');
const source = readFileSync(join(__dirname,
    '../../Linphone/view/Control/Display/MyTel/MyTelConnectionChip.qml'), 'utf8');
function luminance(hex) {
    const channels = [1, 3, 5].map(offset => parseInt(hex.slice(offset, offset + 2), 16) / 255)
        .map(value => value <= 0.04045 ? value / 12.92 : ((value + 0.055) / 1.055) ** 2.4);
    return channels[0] * 0.2126 + channels[1] * 0.7152 + channels[2] * 0.0722;
}
for (const state of ['connected', 'disconnected']) {
    test(`${state} indicator has readable white text`, () => {
        const color = source.match(new RegExp(`${state}Color: "(#[0-9a-f]+)"`, 'i'))[1];
        const ratio = 1.05 / (luminance(color) + 0.05);
        assert.ok(ratio >= 4.5, `contrast ${ratio.toFixed(2)} for ${color}`);
    });
}
