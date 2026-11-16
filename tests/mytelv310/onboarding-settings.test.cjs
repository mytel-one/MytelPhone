const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const test = require('node:test');

const root = path.join(__dirname, '../..');
const login = fs.readFileSync(path.join(root, 'Linphone/view/Page/Form/Login/SIPLoginPage.qml'), 'utf8');
const settings = fs.readFileSync(path.join(root, 'Linphone/view/Page/Form/Settings/AbstractSettingsMenu.qml'), 'utf8');
const helpers = fs.readFileSync(path.join(root, 'Linphone/view/Control/Tool/Helper/utils.js'), 'utf8')
    .replace(/^\.(?:pragma|import).*$/gm, '');
const helperContext = vm.createContext({});
vm.runInContext(helpers, helperContext);
function method(source, name) {
    const start = source.indexOf('function ' + name + '(');
    assert.ok(start >= 0, 'Production method exists: ' + name);
    let end = source.indexOf('{', start) + 1, depth = 1;
    for (; depth && end < source.length; end++) {
        if (source[end] === '{') depth++;
        if (source[end] === '}') depth--;
    }
    return source.slice(start, end);
}
function loginFixture(values = {}, connecting = false) {
    const calls = [], focus = [];
    const mainItem = { connecting, validationError: '', validationField: '' };
    const inputs = {};
    for (const [name, defaultText] of Object.entries({
        usernameEdit: '301', passwordEdit: ' secret ', domainEdit: 'pbx.example.invalid:5060',
        displayName: 'Work', registrar: '', proxy: '', authenticationId: ''
    })) inputs[name] = { text: values[name] ?? defaultText, forceActiveFocus: () => focus.push(name) };
    const context = vm.createContext({ ...inputs, mainItem, transport: { currentValue: 'TCP' },
        Utils: helperContext, LinphoneEnums: { TransportType: { Udp: 6, Tcp: 7, Tls: 8 } },
        qsTr: text => text, LoginPageCpp: { login: (...args) => calls.push(args) } });
    for (const name of ['connecting', 'validationError', 'validationField']) Object.defineProperty(context, name, {
        get: () => mainItem[name], set: value => { mainItem[name] = value; }
    });
    vm.runInContext(method(login, 'connectAccount'), context);
    if (login.includes('function clearValidation(')) vm.runInContext(method(login, 'clearValidation'), context);
    return { context, mainItem, calls, focus };
}
for (const [field, missing, focusName] of [
    ['extension', { usernameEdit: '   ' }, 'usernameEdit'],
    ['password', { passwordEdit: '' }, 'passwordEdit'],
    ['server', { domainEdit: '   ' }, 'domainEdit']
]) test('Missing ' + field + ' focuses its input and prevents registration', () => {
    const f = loginFixture(missing);
    f.context.connectAccount();
    assert.deepEqual(f.calls, []);
    assert.deepEqual(f.focus, [focusName]);
    assert.equal(f.mainItem.validationField, field);
    assert.ok(f.mainItem.validationError.length > 0);
});
test('Valid credentials keep password bytes and configured transport untouched', () => {
    const f = loginFixture({ usernameEdit: ' 301 ', domainEdit: ' pbx.example.invalid:5060 ', displayName: ' Work ' });
    f.context.connectAccount();
    assert.deepEqual(f.calls, [['301', ' secret ', 'Work', 'pbx.example.invalid:5060', 7, '', '', '', false]]);
    assert.equal(f.mainItem.validationError, '');
});
test('Automatic setup carries its mode to the SDK boundary for each advanced route', () => {
    const f = loginFixture();
    f.context.transport.currentValue = 'Automatic';
    f.context.connectAccount();
    assert.equal(f.calls[0][8], true);
});
test('Connecting cannot submit a duplicate registration', () => {
    const f = loginFixture({}, true);
    f.context.connectAccount();
    assert.deepEqual(f.calls, []);
    assert.deepEqual(f.focus, []);
});
test('Correcting one field clears only its own inline validation', () => {
    const f = loginFixture();
    vm.runInContext(method(login, 'clearValidation'), f.context);
    f.mainItem.validationField = 'server'; f.mainItem.validationError = 'Missing server';
    f.context.clearValidation('extension');
    assert.equal(f.mainItem.validationError, 'Missing server');
    f.context.clearValidation('server');
    assert.equal(f.mainItem.validationError, '');
    assert.equal(f.mainItem.validationField, '');
});
function settingsFixture(families) {
    const operations = [];
    const mainItem = { families, layoutsPath: 'settings' };
    const familiesList = { selectedIndex: -1, currentIndex: -1 };
    const stack = { clear: () => operations.push('clear'), push: (...args) => operations.push(args) };
    const context = vm.createContext({ mainItem, familiesList, rightPanelStackView: stack });
    for (const name of ['layoutUrl', 'firstVisibleFamily', 'openFamily']) {
        vm.runInContext(method(settings, name), context);
        mainItem[name] = context[name];
    }
    return { context, mainItem, familiesList, operations };
}
test('Initial settings selection skips unavailable features', () => {
    const f = settingsFixture([{ title: 'Hidden', layout: 'Hidden', visible: false }, { title: 'Calls', layout: 'Calls' }]);
    assert.equal(f.context.firstVisibleFamily(0), 1);
    assert.equal(f.context.firstVisibleFamily(99), 1);
    assert.equal(f.context.firstVisibleFamily(-1), 1);
});
test('An available requested settings section remains selected', () => {
    const f = settingsFixture([{ layout: 'Calls' }, { layout: 'Appearance', visible: true }]);
    assert.equal(f.context.firstVisibleFamily(1), 1);
});
test('No settings family never pushes an undefined component', () => {
    for (const families of [[], [{ layout: 'Hidden', visible: false }]]) {
        const f = settingsFixture(families);
        assert.equal(f.context.firstVisibleFamily(0), -1);
        f.context.openFamily(-1); f.context.openFamily(0); f.context.openFamily(99);
        assert.deepEqual(f.operations, []);
    }
});
test('Opening an unavailable section preserves current settings content', () => {
    const f = settingsFixture([{ layout: 'Hidden', visible: false }, { title: 'Calls', layout: 'Calls' }]);
    f.context.openFamily(0);
    assert.deepEqual(f.operations, []);
});
test('Valid settings navigation loads the requested section and synchronizes keyboard selection', () => {
    const f = settingsFixture([{ title: 'Appearance', layout: 'DisplaySettingsLayout', model: ['a'] }]);
    f.context.openFamily(0);
    assert.equal(f.familiesList.selectedIndex, 0);
    assert.equal(f.familiesList.currentIndex, 0);
    assert.equal(f.operations[0], 'clear');
    assert.equal(f.operations[1][0], 'settings/DisplaySettingsLayout.qml');
    assert.equal(f.operations[1][1].titleText, 'Appearance');
    assert.equal(f.operations[1][1].container, f.context.rightPanelStackView);
});

