const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const source = readFileSync(join(__dirname, '../../Linphone/view/Control/Display/Chat/ChatMessagesListView.qml'), 'utf8');
function block(start) {
    const body = source.indexOf('{', start);
    let end = body + 1, depth = 1;
    for (; depth; end++) { if (source[end] === '{') depth++; if (source[end] === '}') depth--; }
    return source.slice(start, end);
}
for (const value of [undefined, null]) {
    test(`unset deletion selection ${value} hides the remote-delete action`, () => {
        const expression = source.split('\n').find(line => line.includes('visible:') && line.includes('messageDeletionDialog.chatMessage'))
            .split('visible:')[1];
        const context = vm.createContext({ messageDeletionDialog: { chatMessage: value } });
        assert.equal(Boolean(vm.runInContext(expression, context)), false);
    });
}
test('bringing an empty conversation window forward does not mark a missing chat read', () => {
    const context = vm.createContext({ mainWindow: {active: true}, mainItem: {atYEnd: true, chat: null} });
    const callback = vm.runInContext(`(${block(source.indexOf('function onActiveChanged('))})`, context);
    assert.doesNotThrow(() => callback());
});
test('model reset with no instantiated last message keeps the last-item flag false', () => {
    const context = vm.createContext({
        mainItem: { itemAtIndex: () => null, positionViewAtIndex() {}, lastItemVisible: true },
        eventLogProxy: { findFirstUnreadIndex: () => -1, markIndexAsRead() {} },
        itemAtIndex: () => null, ListView: { Beginning: 0 }
    });
    const callback = vm.runInContext(`(() => ${block(source.indexOf('{', source.indexOf('onModelUpdated:')))})`, context);
    assert.doesNotThrow(() => callback());
    assert.equal(context.mainItem.lastItemVisible, false);
});
