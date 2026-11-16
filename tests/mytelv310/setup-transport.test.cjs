const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const source = fs.readFileSync(path.join(__dirname, '../../Linphone/view/Control/Tool/Helper/utils.js'), 'utf8')
    .replace(/^\.(?:pragma|import).*$/gm, '');
const context = vm.createContext({});
vm.runInContext(source, context);
const cases = [
    ['ordinary hostname', 'pbx.example.invalid', 'Automatic', 'UDP'],
    ['ordinary IPv4 and5060', '192.0.2.10:5060', 'Automatic', 'UDP'],
    ['ordinary hostname and5060', 'pbx.example.invalid:5060', 'Automatic', 'UDP'],
    ['blank selection', 'pbx.example.invalid', '', 'UDP'],
    ['unknown selection', 'pbx.example.invalid', 'Unknown', 'UDP'],
    ['secure URI', 'sips:pbx.example.invalid', 'Automatic', 'TLS'],
    ['mixed case secure URI', '  SiPs:pbx.example.invalid:5060  ', 'Automatic', 'TLS'],
    ['secure URI overrides explicit UDP', 'sips:pbx.example.invalid:5060', 'UDP', 'TLS'],
    ['secure URI overrides explicit TCP', 'sips:pbx.example.invalid', 'TCP', 'TLS'],
    ['ordinary secure port', 'pbx.example.invalid:5061', 'Automatic', 'TLS'],
    ['sip secure port', 'sip:pbx.example.invalid:5061', 'Automatic', 'TLS'],
    ['bracketed IPv6 secure port', '[2001:db8::10]:5061', 'Automatic', 'TLS'],
    ['SIP bracketed IPv6 secure port', 'sip:[2001:db8::10]:5061', 'Automatic', 'TLS'],
    ['IPv6 trailing digits are not a port', '2001:db8::5061', 'Automatic', 'UDP'],
    ['bracketed IPv6 without port', '[2001:db8::5061]', 'Automatic', 'UDP'],
    ['explicit TCP beats port inference', 'pbx.example.invalid:5061', 'TCP', 'TCP'],
    ['explicit UDP beats port inference', 'pbx.example.invalid:5061', 'UDP', 'UDP'],
    ['explicit TLS beats ordinary port', 'pbx.example.invalid:5060', 'TLS', 'TLS'],
    ['explicit choice is normalized', 'pbx.example.invalid:5060', ' tcp ', 'TCP'],
    ['URI TCP parameter', 'sip:pbx.example.invalid;transport=tcp', 'Automatic', 'TCP'],
    ['URI TLS parameter', 'sip:pbx.example.invalid;transport=TLS', 'Automatic', 'TLS'],
    ['URI UDP parameter beats secure port', 'sip:pbx.example.invalid:5061;transport=udp', 'Automatic', 'UDP'],
    ['explicit choice beats URI parameter', 'sip:pbx.example.invalid;transport=tcp', 'TLS', 'TLS'],
    ['secure URI beats conflicting parameter', 'sips:pbx.example.invalid;transport=udp', 'Automatic', 'TLS'],
    ['URI query cannot change transport', 'sip:pbx.example.invalid?Subject=hello;transport=tls', 'Automatic', 'UDP'],
    ['unrecognized parameter falls back to port', 'sip:pbx.example.invalid:5061;transport=invalid', 'Automatic', 'TLS']
];
for (const [name, server, selection, expected] of cases) test(name, () => {
    assert.equal(typeof context.resolveSetupTransport, 'function', 'Production transport resolver exists');
    assert.equal(context.resolveSetupTransport(server, selection), expected);
});

