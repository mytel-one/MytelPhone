const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const source = readFileSync(join(__dirname, '../../Linphone/view/Control/Input/Chat/ChatDroppableTextArea.qml'), 'utf8');
const start = source.indexOf('function _emitFiles (');
let end = source.indexOf('{', start) + 1, depth = 1;
for (; depth; end++) { if (source[end] === '{') depth++; if (source[end] === '}') depth--; }
function fixture(available) {
    const drops = [];
    const context = vm.createContext({
        mainItem: { fileTransferAvailable: available },
        Utils: { getSystemPathFromUri: value => value.slice(7) },
        dropped: files => drops.push(Array.from(files))
    });
    return { emit: vm.runInContext(`(${source.slice(start, end)})`, context), drops };
}
test('attachments cannot be submitted without a configured sharing service', () => {
    const f = fixture(false);
    f.emit(['file:///C:/sample.txt']);
    assert.equal(f.drops.length, 0);
});
test('configured sharing preserves local file selection', () => {
    const f = fixture(true);
    f.emit(['file:///C:/sample.txt']);
    assert.deepEqual(f.drops, [['/C:/sample.txt']]);
});
test('dropping a remote URL never becomes a file upload', () => {
    const f = fixture(true);
    f.emit(['https://example.org/file.txt']);
    assert.equal(f.drops.length, 0);
});
