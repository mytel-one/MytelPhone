const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const root = join(__dirname, '../..');
function productionFunction(file, name, context) {
    const source = readFileSync(join(root, file), 'utf8');
    const start = source.indexOf(`function ${name}(`);
    assert.ok(start >= 0, `${name} production function exists`);
    let end = source.indexOf('{', start) + 1;
    let depth = 1;
    for (; depth; end++) {
        if (source[end] === '{') depth++;
        if (source[end] === '}') depth--;
    }
    return vm.runInContext(`(${source.slice(start, end)})`, context);
}
function fixture() {
    const calls = [], errors = [];
    let closed = 0;
    const context = vm.createContext({
        dialSearchBar: { inputText: '302', text: '30' },
        numericPadPopupItem: { close: () => closed++ },
        UtilsCpp: { createCall: value => calls.push(value), showInformationPopup: (...args) => errors.push(args) },
        qsTr: value => value
    });
    vm.runInContext(readFileSync(join(root, 'Linphone/view/Control/Tool/Helper/utils.js'), 'utf8')
        .replace(/^\.(?:pragma|import).*$/gm, ''), context);
    context.mytelNormalizeDial = context.normalizeSipDestination;
    const call = productionFunction('Linphone/view/Page/Main/Call/CallPage.qml', 'placeDialedCall', context);
    return { call, context, calls, errors, closed: () => closed };
}
test('keypad calls the current input without waiting for the history search debounce', () => {
    const f = fixture();
    f.call();
    assert.deepEqual(f.calls, ['302']);
    assert.equal(f.closed(), 1);
    assert.equal(f.errors.length, 0);
});
test('Enter passes the current keyboard value directly', () => {
    const f = fixture();
    f.call('۳۰۱');
    assert.deepEqual(f.calls, ['301']);
    assert.equal(f.closed(), 1);
});
test('invalid dial requests keep the keypad open and show an error', () => {
    const f = fixture();
    f.call('https://invalid.example');
    assert.equal(f.calls.length, 0);
    assert.equal(f.errors.length, 1);
    assert.equal(f.closed(), 0);
});
test('a page transition with no dial field cannot trigger a hidden call', () => {
    const f = fixture();
    f.context.dialSearchBar = null;
    f.call();
    assert.equal(f.calls.length, 0);
    assert.equal(f.errors.length, 1);
});
test('clearing a dial field clears displayed and filtered values immediately', () => {
    let stopped = 0;
    const context = vm.createContext({
        delayTimer: { stop: () => stopped++ },
        textField: { text: '302', searchText: '30' }
    });
    productionFunction('Linphone/view/Control/Input/SearchBar.qml', 'clearText', context)();
    assert.equal(stopped, 1);
    assert.equal(context.textField.text, '');
    assert.equal(context.textField.searchText, '');
});
