const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');

test('button surfaces and labels follow theme changes without restarting', () => {
    const palette = { grey_0: '#ffffff', surface: '#ffffff', main2_600: '#4e6074' };
    const context = vm.createContext({ Linphone: { DefaultStyle: palette, AppIcons: {} } });
    const source = readFileSync(join(__dirname, '../../Linphone/view/Style/buttonStyle.js'), 'utf8')
        .replace(/^\.(?:pragma|import).*$/gm, '');
    vm.runInContext(source, context);
    assert.equal(context.numericPad.color.normal, '#ffffff');
    assert.equal(context.numericPad.text.normal, '#4e6074');
    palette.surface = '#142030';
    palette.main2_600 = '#c4d2e3';
    assert.equal(context.numericPad.color.normal, '#142030');
    assert.equal(context.numericPad.text.normal, '#c4d2e3');
    assert.equal(context.main.text.normal, '#ffffff', 'accent button text stays white');
});
