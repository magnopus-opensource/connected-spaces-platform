import { test, assert } from '../test_framework.js';

import { CSPFoundation, Common } from '../olympus_foundation.js';


test('MiscellaneousTests', 'GetVersionTest', async function() {
    const version = CSPFoundation.getVersion();

    assert.areEqual(typeof version, 'string');
});


test('MiscellaneousTests', 'StringMapTest', async function() {
    const map = Common.Map.ofStringAndString();
    map.set('asd', '123');
    map.set('fgh', '456');

    assert.areEqual(map.size(), 2);
    assert.isTrue(map.hasKey('asd'));
    assert.isTrue(map.hasKey('fgh'));
    assert.areEqual(map.get('asd'), '123');
    assert.areEqual(map.get('fgh'), '456');
    
    const keys = map.keys();

    assert.areEqual(keys.size(), 2);
    assert.areEqual(keys.get(0), 'asd');
    assert.areEqual(keys.get(1), 'fgh');

    keys.delete();

    const values = map.values();

    assert.areEqual(values.size(), 2);
    assert.areEqual(values.get(0), '123');
    assert.areEqual(values.get(1), '456');

    values.delete();
    map.delete();
});