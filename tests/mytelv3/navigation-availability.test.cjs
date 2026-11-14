const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const source = readFileSync(join(__dirname, '../../Linphone/view/Control/Container/VerticalTabBar.qml'), 'utf8');
const start = source.indexOf('function updateVisibleCount(');
let end = source.indexOf('{', start) + 1, depth = 1;
for (; depth; end++) { if (source[end] === '{') depth++; if (source[end] === '}') depth--; }
function availableCount(children) {
    const context = vm.createContext({ mainItem: { visibleCount: -1 }, contentChildren: children });
    vm.runInContext(source.slice(start, end), context);
    context.updateVisibleCount();
    return context.mainItem.visibleCount;
}
test('visible navigation counts enabled product features', () => {
    assert.equal(availableCount([{ visible: true, isAvailable: true }, { visible: true, isAvailable: true },
        { visible: false, isAvailable: false }]), 2);
});
test('hiding the retained page does not change the navigation feature count', () => {
    assert.equal(availableCount([{ visible: false, isAvailable: true }, { visible: false, isAvailable: true },
        { visible: false, isAvailable: true }, { visible: false, isAvailable: false }]), 3);
});
test('a navigation bar with all product features disabled stays empty', () => {
    assert.equal(availableCount([{ visible: false, isAvailable: false }, { visible: false, isAvailable: false }]), 0);
});
