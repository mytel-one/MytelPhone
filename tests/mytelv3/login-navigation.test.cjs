const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const source = readFileSync(join(__dirname, '../../Linphone/view/Page/Window/Main/MainWindow.qml'), 'utf8');
function block(marker, from = source) {
    const start = from.indexOf(marker);
    assert.ok(start >= 0, `production block exists: ${marker}`);
    let end = from.indexOf('{', start) + 1, depth = 1;
    for (; depth; end++) { if (from[end] === '{') depth++; if (from[end] === '}') depth--; }
    return from.slice(start, end);
}
function fixture(haveAccount = true) {
    const main = { objectName: 'mainPage', selection: 'preserved-history' };
    const pages = haveAccount ? [main] : [{ objectName: 'sipLoginPage' }];
    const stack = {
        get currentItem() { return pages.at(-1); }, get depth() { return pages.length; },
        push(page) { pages.push({ objectName: page }); },
        replace(page) { pages.splice(-1, 1, { objectName: page }); },
        pop(target) {
            if (arguments.length && target && typeof target === 'object') pages.splice(pages.indexOf(target) + 1);
            else if (pages.length > 1) pages.pop();
        },
        find(predicate) { return [...pages].reverse().find(predicate); }
    };
    const context = vm.createContext({ mainStackViewLoader: { item: stack }, accountProxy: { haveAccount },
        StackView: { Immediate: 0 }, mainPage: 'mainPage', sipLoginPage: 'sipLoginPage',
        SettingsCpp: { assistantGoDirectlyToThirdPartySipAccountLogin: false } });
    for (const name of ['openMainPage', 'goToLogin']) vm.runInContext(block(`function ${name}(`), context);
    if (source.includes('function leaveSipLogin(')) vm.runInContext(block('function leaveSipLogin('), context);
    context.mainWindow = context;
    const sip = source.slice(source.indexOf('id: sipLoginPage'));
    const backLine = sip.match(/onGoBack:\s*([^\r\n]+)/)[1];
    const back = backLine.startsWith('{') ? block('onGoBack:', sip).replace('onGoBack:', '') : backLine;
    return { pages, main, context, stack, back: () => vm.runInContext(back, context) };
}
test('Add account retains the existing main page underneath the login form', () => {
    const f = fixture(); f.context.goToLogin();
    assert.equal(f.stack.depth, 2);
    assert.equal(f.pages[0], f.main);
    assert.equal(f.stack.currentItem.objectName, 'sipLoginPage');
});
test('Back after cancelled add-account registration restores prior selection', () => {
    const f = fixture(); f.context.goToLogin(); f.back();
    assert.equal(f.stack.currentItem, f.main);
    assert.equal(f.stack.currentItem.selection, 'preserved-history');
    assert.equal(f.stack.depth, 1);
});
test('successful add-account return reuses the existing main page', () => {
    const f = fixture(); f.context.goToLogin(); f.context.openMainPage();
    assert.equal(f.stack.currentItem, f.main);
    assert.equal(f.stack.depth, 1);
});
test('repeated Add account navigation does not push duplicate login pages', () => {
    const f = fixture(); f.context.goToLogin(); f.context.goToLogin();
    assert.equal(f.stack.depth, 2);
});
test('Back still recovers a legacy login-only stack when an account exists', () => {
    const f = fixture(); f.stack.replace('sipLoginPage'); f.back();
    assert.equal(f.stack.currentItem.objectName, 'mainPage');
});
test('a first profile starts with one editable login page', () => {
    const f = fixture(false); f.context.goToLogin();
    assert.equal(f.stack.depth, 1);
    assert.equal(f.stack.currentItem.objectName, 'sipLoginPage');
});
for (const [name, args] of [
    ['goToNewCall', []], ['goToCallHistory', []], ['displayContactPage', ['sip:301@pbx']],
    ['displayChatPage', ['sip:302@pbx']], ['openChat', [{ chat: 1 }]],
    ['createContact', ['Test contact', '301']], ['scheduleMeeting', ['Test', ['301', '302']]]
]) {
    test(`main page wrapper forwards ${name} to the retained layout`, () => {
        const calls = [];
        const context = vm.createContext({ mainLayout: { [name]: (...values) => calls.push(values) } });
        const wrapper = source.slice(source.indexOf('id: mainPage'));
        const method = block(`function ${name}(`, wrapper);
        vm.runInContext(method, context);
        context[name](...args);
        assert.deepEqual(calls, [args]);
    });
}
const layoutSource = readFileSync(join(__dirname, '../../Linphone/view/Page/Layout/Main/MainLayout.qml'), 'utf8');
for (const [name, from, marker, owner] of [
    ['account popup', layoutSource, 'onAddAccountRequest:', 'avatarButton'],
    ['main options popup', layoutSource.slice(layoutSource.indexOf('id: addAccountButton')), 'onClicked:', 'settingsMenuButton']
]) {
    test(`Add account closes ${name} before hiding the retained page`, () => {
        const expression = from.slice(from.indexOf(marker) + marker.length).trimStart();
        const body = expression.startsWith('{') ? block(marker, from).replace(marker, '') : expression.split(/\r?\n/)[0];
        const calls = [];
        vm.runInNewContext(body, { [owner]: { close: () => calls.push('close') },
            mainItem: { addAccountRequest: () => calls.push('navigate') } });
        assert.deepEqual(calls, ['close', 'navigate']);
    });
}
