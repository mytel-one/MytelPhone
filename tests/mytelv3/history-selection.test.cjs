const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const read = file => readFileSync(join(__dirname, '../../Linphone/view', file), 'utf8');
const listSource = read('Control/Display/Call/CallHistoryListView.qml');
const pageSource = read('Page/Main/Call/CallPage.qml').split('id: historyListView')[1];
function block(source, marker) {
    const start = source.indexOf(marker);
    if (start < 0) return null;
    const value = source.slice(start + marker.length).trimStart();
    if (!value.startsWith('{')) return value.split(/\r?\n/)[0];
    let end = 1, depth = 1;
    for (; depth; end++) { if (value[end] === '{') depth++; if (value[end] === '}') depth--; }
    return value.slice(1, end - 1);
}
const row = peer => ({ core: { remoteAddress: peer } });
function fixture(peers = ['302', '*43', '301'], index = 2, pageSize = 20) {
    let rows = peers.map(row), limit = pageSize, selected = index;
    const page = { selectedRowHistoryGui: rows[index] }, queue = [], calls = [];
    const model = { get count() { return Math.min(limit, rows.length); },
        get haveMore() { return limit < rows.length; },
        filterText: 'manual-filter', getAt: i => i >= 0 && i < model.count ? rows[i] : null,
        displayMore: () => { limit += pageSize; } };
    const context = vm.createContext({ model, loading: false, selectionResetPending: false, selectionPeerAddress: '',
        atYBeginning: false, positionViewAtBeginning() {}, searchText: 'manual-filter',
        Qt: { callLater: fn => { if (!queue.includes(fn)) queue.push(fn); } },
        UtilsCpp: { createCall: value => calls.push(value) } });
    context.mainItem = context;
    Object.defineProperty(context, 'count', { get: () => model.count });
    const pageContext = vm.createContext({ mainItem: page, model });
    Object.defineProperty(pageContext, 'currentIndex', { get: () => selected });
    Object.defineProperty(pageContext, 'loading', { get: () => context.loading });
    const pageHandler = name => {
        const body = block(pageSource, `${name}:`);
        if (body) vm.runInContext(body, pageContext);
    };
    Object.defineProperty(context, 'currentIndex', { get: () => selected, set: value => {
        if (selected !== value) { selected = value; pageHandler('onCurrentIndexChanged'); }
    }});
    for (const name of ['rememberSelection', 'restoreSelection']) {
        const body = block(listSource, `function ${name}()`);
        if (body) vm.runInContext(`function ${name}() { ${body} }`, context);
    }
    context.resultsReceived = () => {
        vm.runInContext(block(listSource, 'onResultsReceived:'), context);
        pageHandler('onResultsReceived');
    };
    const handler = name => vm.runInContext(block(listSource, `${name}:`), context);
    const reset = (values, flush = true) => {
        handler('onModelAboutToBeReset');
        const previousIndex = selected;
        rows = [];
        context.currentIndex = -1;
        vm.runInContext(block(listSource, 'onCountChanged:'), context);
        pageHandler('onCountChanged');
        rows = values.map(row);
        context.currentIndex = Math.min(previousIndex, model.count - 1);
        vm.runInContext(block(listSource, 'onCountChanged:'), context);
        pageHandler('onCountChanged');
        handler('onModelReset');
        if (flush) flushQueue();
    };
    const flushQueue = () => { while (queue.length) queue.shift()(); };
    return { context, model, page, reset, flushQueue, calls, get peer() { return page.selectedRowHistoryGui?.core?.remoteAddress; } };
}
test('inserting a new call before the selected peer retains that peer', () => {
    const f = fixture(); f.reset(['301', '302', '*43', '301']);
    assert.equal(f.peer, '301');
    assert.equal(f.model.getAt(f.context.currentIndex).core.remoteAddress, '301');
    assert.equal(f.calls.length, 0);
});
test('a different peer new call also retains the existing target', () => {
    const f = fixture(); f.reset(['3099', '302', '*43', '301']);
    assert.equal(f.peer, '301'); assert.equal(f.context.currentIndex, 3);
});
test('selected peer updates with replacement GUI objects rather than keeping a stale object', () => {
    const f = fixture(), before = f.page.selectedRowHistoryGui;
    f.reset(['302', '*43', '301']);
    assert.equal(f.peer, '301'); assert.notEqual(f.page.selectedRowHistoryGui, before);
});
test('deleting the selected peer clears selection instead of selecting an unrelated row', () => {
    const f = fixture(); f.reset(['302', '*43']);
    assert.equal(f.context.currentIndex, -1); assert.equal(f.peer, undefined);
});
test('empty history clears selection safely', () => {
    const f = fixture(); f.reset([]);
    assert.equal(f.context.currentIndex, -1); assert.equal(f.peer, undefined);
});
test('first load still selects the first result', () => {
    const f = fixture([], -1); f.reset(['301', '302']);
    assert.equal(f.context.currentIndex, 0); assert.equal(f.peer, '301');
});
test('manual filter values survive history refresh', () => {
    const f = fixture(); f.reset(['302', '*43', '301']);
    assert.equal(f.model.filterText, 'manual-filter'); assert.equal(f.context.searchText, 'manual-filter');
});
test('overlapping resets preserve the original peer until deferred restoration', () => {
    const f = fixture(); f.reset(['3099', '302', '*43', '301'], false);
    f.reset(['300', '3099', '302', '*43', '301'], false); f.flushQueue();
    assert.equal(f.peer, '301'); assert.equal(f.calls.length, 0);
});
test('a selected peer pushed past the display limit is restored by paging', () => {
    const f = fixture(['302', '*43', '301'], 2, 3); f.reset(['3099', '302', '*43', '301']);
    assert.equal(f.peer, '301'); assert.equal(f.context.currentIndex, 3);
});
test('a stalled paging source cannot loop forever while restoring a removed peer', () => {
    const f = fixture(['302', '*43', '301'], 2, 3); f.model.displayMore = () => {};
    f.reset(['3099', '302', '*43', '300']);
    assert.equal(f.context.currentIndex, -1); assert.equal(f.peer, undefined);
});
