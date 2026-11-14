const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const source = readFileSync(join(__dirname, '../../Linphone/view/Control/Button/PopupButton.qml'), 'utf8');
function block(marker) {
    const start = source.indexOf(marker);
    let end = source.indexOf('{', start) + 1, depth = 1;
    for (; depth; end++) { if (source[end] === '{') depth++; if (source[end] === '}') depth--; }
    return source.slice(start, end);
}
function fixture(children, visible = true) {
    const context = vm.createContext({});
    for (const name of ['isFocusable', 'hasFocusableChild', '_findFocusableItem', '_getPreviousItem', '_getNextItem']) {
        if (source.includes(`function ${name}(`)) vm.runInContext(block(`function ${name}(`), context);
    }
    const content = { visible, children, visibleChildren: children.filter(c => c.visible) };
    return { context, content };
}
const item = (name, visible = true, enabled = true, focusable = true) => ({ name, visible, enabled, activeFocusOnTab: focusable });
for (const method of ['_getPreviousItem', '_getNextItem']) {
    test(`${method}: hidden entries plus visible decoration produce no focus target`, () => {
        const f = fixture([item('hidden', false), item('separator', true, true, false)]);
        assert.equal(f.context[method](f.content, 0), null);
    });
    test(`${method}: disabled entries are excluded`, () => {
        const f = fixture([item('disabled', true, false)]);
        assert.equal(f.context[method](f.content, 0), null);
    });
    test(`${method}: missing or hidden content produces no target`, () => {
        const f = fixture([item('button')], false);
        assert.equal(f.context[method](f.content, 0), null);
        assert.equal(f.context[method](null, 0), null);
    });
    test(`${method}: wrap skips hidden and disabled siblings`, () => {
        const target = item('target');
        const f = fixture([item('hidden', false), target, item('disabled', true, false)]);
        assert.equal(f.context[method](f.content, 1), target);
    });
}
for (const key of ['Key_Up', 'Key_Down']) {
    test(`keyboard ${key}: an empty popup never dereferences a missing target`, () => {
        const context = vm.createContext({ Qt: { Key_Up: 1, Key_Down: 2, TabFocusReason: 3, BacktabFocusReason: 4 },
            mainItem: { checked: true }, getPreviousItem: () => null, getNextItem: () => null });
        const body = block('Keys.onPressed: event =>').replace('Keys.onPressed: event =>', '');
        const handler = vm.runInContext(`(event => ${body})`, context);
        assert.doesNotThrow(() => handler({ key: context.Qt[key] }));
    });
}
