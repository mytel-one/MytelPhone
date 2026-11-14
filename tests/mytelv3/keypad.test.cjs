const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const source = readFileSync(join(__dirname, '../../Linphone/view/Control/Input/NumericPad.qml'), 'utf8');
function block(start) {
    const body = source.indexOf('{', start);
    let depth = 1, end = body + 1;
    for (; depth; end++) {
        if (source[end] === '{') depth++;
        if (source[end] === '}') depth--;
    }
    return source.slice(start, end);
}
function fixture() {
    const Qt = { KeypadModifier: 1, ControlModifier: 2, AltModifier: 4,
        Key_Enter: 100, Key_Return: 101, Key_Asterisk: 102, Key_NumberSign: 103,
        Key_Plus: 104, Key_Backspace: 105 };
    for (let i = 0; i < 10; i++) Qt[`Key_${i}`] = 48 + i;
    const digits = [], actions = [];
    const context = vm.createContext({ Qt,
        keypadKeyPressedAtIndex: index => digits.push(['1','2','3','4','5','6','7','8','9','*','0','#'][index]),
        mainItem: { buttonPressed: value => digits.push(value), launchCall: () => actions.push('call'), wipe: () => actions.push('wipe') }
    });
    context.handleKeyPadEvent = vm.runInContext(`(${block(source.indexOf('function handleKeyPadEvent('))})`, context);
    const start = source.indexOf(':', source.indexOf('Keys.onPressed:')) + 1;
    const handler = vm.runInContext(`(${block(start)})`, context);
    return { Qt, digits, actions, press: (key, modifiers = 0) => {
        const event = { key, modifiers, accepted: false };
        handler(event);
        return event;
    }};
}
for (const digit of ['0', '1', '5', '9']) {
    test(`top-row keyboard ${digit} enters a digit while the keypad has focus`, () => {
        const f = fixture();
        assert.equal(f.press(f.Qt[`Key_${digit}`]).accepted, true);
        assert.deepEqual(f.digits, [digit]);
    });
}
test('keyboard # is entered as a service-code/DTMF digit', () => {
    const f = fixture();
    assert.equal(f.press(f.Qt.Key_NumberSign).accepted, true);
    assert.deepEqual(f.digits, ['#']);
});
test('numpad Enter launches one call', () => {
    const f = fixture();
    f.press(f.Qt.Key_Enter, f.Qt.KeypadModifier);
    assert.deepEqual(f.actions, ['call']);
});
test('Backspace removes one digit', () => {
    const f = fixture();
    f.press(f.Qt.Key_Backspace);
    assert.deepEqual(f.actions, ['wipe']);
});
test('Ctrl+number is left available for shortcuts', () => {
    const f = fixture();
    assert.equal(f.press(f.Qt.Key_1, f.Qt.ControlModifier).accepted, false);
    assert.deepEqual(f.digits, []);
});
