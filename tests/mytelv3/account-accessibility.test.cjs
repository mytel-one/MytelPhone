const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const source = readFileSync(join(__dirname, '../../Linphone/view/Control/Display/Contact/Contact.qml'), 'utf8');
const control = source.slice(source.indexOf('id: manageAccount'));
const nameStart = control.indexOf('Accessible.name:') + 'Accessible.name:'.length;
const value = control.slice(nameStart).trimStart();
let expression;
if (value.startsWith('{')) {
    let end = 1, depth = 1;
    for (; depth; end++) { if (value[end] === '{') depth++; if (value[end] === '}') depth--; }
    expression = value.slice(1, end - 1);
} else expression = `return ${value.split(/\r?\n/)[0]}`;
function accessibleName(account) {
    const context = vm.createContext({ mainItem: { account },
        UtilsCpp: { getUsername: value => value.replace(/^sips?:/, '').split('@')[0] } });
    vm.runInContext(`
        function qsTr(key) { return key === 'account_settings_name_accessible_name' ? 'Account settings of %1' : key; }
        String.prototype.arg = function(value) { return this.replace('%1', value); };
    `, context);
    return vm.runInContext(`(function() { ${expression} })()`, context);
}
for (const [name, account, expected] of [
    ['extension account', { core: { displayName: '302', identityAddress: 'sip:302@pbx' } }, 'Account settings of 302'],
    ['display name', { core: { displayName: '  Saman  ', identityAddress: 'sip:302@pbx' } }, 'Account settings of Saman'],
    ['username fallback', { core: { displayName: ' ', identityAddress: 'sip:302@pbx' } }, 'Account settings of 302'],
    ['unnamed account', { core: { displayName: '', identityAddress: '' } }, 'Account settings'],
    ['missing core', {}, 'Account settings'],
    ['missing account', null, 'Account settings']
]) {
    test(`gear accessible name: ${name}`, () => assert.equal(accessibleName(account), expected));
}
