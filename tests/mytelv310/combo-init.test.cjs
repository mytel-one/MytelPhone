const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const source = fs.readFileSync(path.join(__dirname, '../../Linphone/view/Control/Button/ComboBox.qml'), 'utf8');
const start = source.indexOf('onCurrentIndexChanged:');
const begin = source.indexOf('{', start);
let end = begin + 1, depth = 1;
for (; depth; end++) { if (source[end] === '{') depth++; if (source[end] === '}') depth--; }
const handler = '(function()' + source.slice(begin, end) + ')()';
function fixture(model, index) {
    return { model, currentIndex:index, constantImageSource:'', mainItem:{textRole:'text',flagRole:'flag'},
        selectedItemText:{text:'unchanged'}, selectedItemFlag:{text:'unchanged'}, selectedItemImg:{imageSource:'unchanged'} };
}
for (const [name, model, index] of [
    ['undefined model',undefined,0], ['null model',null,0], ['unselected index',[],-1],
    ['empty array',[],0], ['past-end array',[{text:'UDP'}],9]
]) test('ComboBox tolerates ' + name + ' while its model initializes', () => {
    const f=fixture(model,index);
    assert.doesNotThrow(()=>vm.runInNewContext(handler,f));
    assert.equal(f.selectedItemText.text,'unchanged');
});
test('ComboBox still renders array model text, flag and icon',()=>{
    const f=fixture([{text:'Automatic',flag:'',img:'demo.svg'}],0);
    vm.runInNewContext(handler,f);
    assert.equal(f.selectedItemText.text,'Automatic');
    assert.equal(f.selectedItemFlag.text,'');
    assert.equal(f.selectedItemImg.imageSource,'demo.svg');
});
test('ComboBox still supports application models exposing getAt',()=>{
    const calls=[];
    const f=fixture({getAt:index=>{calls.push(index);return {text:'TLS'};}},0);
    vm.runInNewContext(handler,f);
    assert.deepEqual(calls,[0]);
    assert.equal(f.selectedItemText.text,'TLS');
});

