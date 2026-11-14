const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const source = readFileSync(join(__dirname, '../../Linphone/view/Page/Layout/Main/MainLayout.qml'), 'utf8');
const pill = source.slice(source.indexOf('id: currentCallNotif'));
const expression = pill.match(/\n\s*text:\s*([\s\S]*?)\n\s*onClicked:/)[1];
for (const [name, currentCall, remoteName, expected] of [
    ['active extension', { core: {} }, '301', 'Call in progress: 301'],
    ['Persian display name', { core: {} }, 'سامان (301)', 'Call in progress: سامان (301)'],
    ['literal formatting characters', { core: {} }, '301 %1 $&', 'Call in progress: 301 %1 $&'],
    ['meeting subject', { core: { conference: { core: { subject: 'Team sync' } } } }, '', 'Meeting in progress: Team sync'],
    ['no active call fallback', null, '', 'Call in progress']
]) {
    test(`ongoing call pill: ${name}`, () => {
        const context = vm.createContext({ currentCallNotif: { currentCall, remoteName } });
        vm.runInContext(`function qsTr(value) { return value; }
            String.prototype.arg = function(value) { return this.replace('%1', () => String(value)); };`, context);
        assert.equal(vm.runInContext(expression, context), expected);
    });
}
