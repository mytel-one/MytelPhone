const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const root = join(__dirname, '../..');
const qml = readFileSync(join(root, 'Linphone/view/Page/Form/Call/NewCallForm.qml'), 'utf8');
const start = qml.indexOf('function placeCall(');
let end = qml.indexOf('{', start) + 1, depth = 1;
for (; depth; end++) { if (qml[end] === '{') depth++; if (qml[end] === '}') depth--; }
function fixture(transfer, value = '302') {
    const calls = [], transfers = [];
    const context = vm.createContext({ displayCurrentCalls: transfer, dialField: {text: value}, dialError: '',
        qsTr: value => value, console: { log() {} },
        UtilsCpp: { createCall: value => calls.push(value) },
        mainItem: { transferDestinationRequested: value => transfers.push(value) } });
    vm.runInContext(readFileSync(join(root, 'Linphone/view/Control/Tool/Helper/utils.js'), 'utf8')
        .replace(/^\.(?:pragma|import).*$/gm, ''), context);
    context.normalizeInput = context.normalizeSipDestination;
    const submit = vm.runInContext(`(${qml.slice(start, end)})`, context);
    return { submit, context, calls, transfers };
}
test('typing a transfer destination requests transfer and never creates an extra call', () => {
    const f = fixture(true, '۳۰۲');
    f.submit();
    assert.deepEqual(f.transfers, ['302']);
    assert.equal(f.calls.length, 0);
});
test('the ordinary new-call form still places a call', () => {
    const f = fixture(false);
    f.submit();
    assert.deepEqual(f.calls, ['302']);
    assert.equal(f.transfers.length, 0);
});
test('invalid transfer destinations invoke neither SDK action', () => {
    const f = fixture(true, 'invalid number');
    f.submit();
    assert.equal(f.calls.length, 0);
    assert.equal(f.transfers.length, 0);
    assert.ok(f.context.dialError.length > 0);
});
