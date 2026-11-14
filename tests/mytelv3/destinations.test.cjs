const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');
const test = require('node:test');

const root = join(__dirname, '../..');
const helpers = readFileSync(join(root, 'Linphone/view/Control/Tool/Helper/utils.js'), 'utf8')
    .replace(/^\.(?:pragma|import).*$/gm, '');
const context = vm.createContext({});
vm.runInContext(helpers, context);
context.Utils = context;

// Execute each production entry point, including the QML wrapper. No copied
// normalizer lives in the test: this catches a forgotten call/chat entry point.
const entryPoints = [
    ['call page', 'Linphone/view/Page/Main/Call/CallPage.qml', 'mytelNormalizeDial'],
    ['call form', 'Linphone/view/Page/Form/Call/NewCallForm.qml', 'normalizeInput'],
    ['chat form', 'Linphone/view/Page/Form/Chat/NewChatForm.qml', 'normalizeInput']
];
const cases = [
    ['extension', '301', '301'],
    ['Persian extension', '۳۰۱', '301'],
    ['Arabic extension', '٣٠١', '301'],
    ['spaces', '  301  ', '301'],
    ['national mobile', '0912 123-4567', '09121234567'],
    ['international mobile', '+989121234567', '09121234567'],
    ['international Persian mobile', '+۹۸۹۱۲۱۲۳۴۵۶۷', '09121234567'],
    ['international mobile without plus', '989121234567', '09121234567'],
    ['international prefix', '00989121234567', '09121234567'],
    ['alternate prefix', '0989121234567', '09121234567'],
    ['short extension starting 098', '0981', '0981'],
    ['foreign number', '+442079460000', '+442079460000'],
    ['feature code', '*97', '*97'],
    ['SIP address', 'sip:alice@pbx.example.org:5060;transport=tcp', 'sip:alice@pbx.example.org:5060;transport=tcp'],
    ['secure SIP address', 'sips:alice@pbx.example.org', 'sips:alice@pbx.example.org'],
    ['address without scheme', '301@192.168.8.171', 'sip:301@192.168.8.171'],
    ['empty', '', ''],
    ['whitespace', '   ', ''],
    ['invalid word', 'not a number', ''],
    ['invalid URL', 'https://example.org', ''],
    ['SIP missing destination', 'sip:', ''],
    ['SIP missing domain', 'sip:301@', ''],
    ['header injection', 'sip:301@pbx\r\nVia: invalid', ''],
    ['invalid plus', '30+1', ''],
    ['invalid input', null, ''],
    ['long number', '3'.repeat(65), '']
];

for (const [label, path, name] of entryPoints) {
    const qml = readFileSync(join(root, path), 'utf8');
    const start = qml.indexOf(`function ${name}(`);
    assert.notEqual(start, -1, `production entry point missing: ${name}`);
    const bodyStart = qml.indexOf('{', start);
    let depth = 1;
    let end = bodyStart + 1;
    for (; depth && end < qml.length; end++) {
        if (qml[end] === '{') depth++;
        if (qml[end] === '}') depth--;
    }
    const normalize = vm.runInContext(`(${qml.slice(start, end)})`, context);
    for (const [description, input, expected] of cases) {
        test(`${label}: ${description}`, () => assert.equal(normalize(input), expected));
    }
}
