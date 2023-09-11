import { test, assert } from '../test_framework.js';

import { Systems } from '../connected_spaces_platform.js';

function logCallback(message) {

}

test('LogSystemTests', 'BasicLogTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const logSystem = systemsManager.getLogSystem();

    let messageLogged = false;

    logSystem.setLogCallback(function (message) {
        console.log(message);
        messageLogged = true;
    });

    logSystem.logTestMessage();

    assert.isTrue(messageLogged);
});

test('LogSystemTests', 'LogLevelTest', async function() {
    const systemsManager = Systems.SystemsManager.get();
    const logSystem = systemsManager.getLogSystem();

    logSystem.setSystemLevel(Systems.LogLevel.All);
    assert.areEqual(logSystem.getSystemLevel(), Systems.LogLevel.All);

    logSystem.setSystemLevel(Systems.LogLevel.VeryVerbose);
    assert.areEqual(logSystem.getSystemLevel(), Systems.LogLevel.VeryVerbose);

    logSystem.setSystemLevel(Systems.LogLevel.Verbose);
    assert.areEqual(logSystem.getSystemLevel(), Systems.LogLevel.Verbose);

    logSystem.setSystemLevel(Systems.LogLevel.Log);
    assert.areEqual(logSystem.getSystemLevel(), Systems.LogLevel.Log);

    logSystem.setSystemLevel(Systems.LogLevel.Display);
    assert.areEqual(logSystem.getSystemLevel(), Systems.LogLevel.Display);

    logSystem.setSystemLevel(Systems.LogLevel.Warning);
    assert.areEqual(logSystem.getSystemLevel(), Systems.LogLevel.Warning);

    logSystem.setSystemLevel(Systems.LogLevel.Error);
    assert.areEqual(logSystem.getSystemLevel(), Systems.LogLevel.Error);

    logSystem.setSystemLevel(Systems.LogLevel.Fatal);
    assert.areEqual(logSystem.getSystemLevel(), Systems.LogLevel.Fatal);

    logSystem.setSystemLevel(Systems.LogLevel.NoLogging);
    assert.areEqual(logSystem.getSystemLevel(), Systems.LogLevel.NoLogging);
});



