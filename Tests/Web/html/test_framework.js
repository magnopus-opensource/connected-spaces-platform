import { C, CONSOLE_RED, CONSOLE_GREEN, CHS_ENDPOINT_BASE_URI } from './test_helpers.js'

import { ClientUserAgent, CSPFoundation, Systems } from './connected_spaces_platform.js';


/**
 * Represents an assert failure
 */
export class AssertionError extends Error {
    condition = "";

    /**
     * @param {string} condition
     * @param {any[]} params
     */
    constructor(condition, ...params) {
        super(...params)

        //@ts-ignore
        if (Error.captureStackTrace) {
            //@ts-ignore
            Error.captureStackTrace(this, AssertionError);
        }

        this.name = 'AssertionError';
        this.condition = condition;
    }
}


/**
 * Represents a fatal error that should result in process termination
 */
export class FatalError extends Error {
    /**
     * @param {string} message
     */
    constructor(message) {
        super(message)

        //@ts-ignore
        if (Error.captureStackTrace) {
            //@ts-ignore
            Error.captureStackTrace(this, FatalError);
        }

        this.name = 'FatalError';
    }
}


/**
 * @typedef {function(): Promise<void>} CleanupFunction
 */


/**
 * @typedef {function(): Promise<void>} TestBody
 */


/**
 * @typedef {object} Test
 * @property {string} name
 * @property {TestBody} body
 */


/**
 * @typedef {object} TestSuite
 * @property {string} name
 * @property {Array<Test>} tests
 */


let /** @type {Array<TestSuite>} */ testsCollection = []
let /** @type {Array<CleanupFunction>} */ cleanupFunctions = []


/**
 * Defines a test
 * @param {string} namespace 
 * @param {string} name 
 * @param {TestBody} body 
 */
export function test(namespace, name, body) {
    let suite = testsCollection.find(s => s.name === namespace);

    if (suite === undefined) {
        suite = { name: namespace, tests: [] }
        testsCollection.push(suite);
    }

    suite.tests.push({ name, body });
}


/**
 * Pushes a callback to call once this test has finished
 * @param {CleanupFunction} body 
 */
export function pushCleanupFunction(body) {
    cleanupFunctions.push(body);
}


/**
 * Runs all tests
 * @param {Array<string>} includedTests
 * @returns {Promise<XMLDocument>}
 */
export async function runTests(includedTests) {
    let /** @type {XMLDocument} */ results = document.implementation.createDocument(null, 'testsuites');
    let /** @type {HTMLElement} */ resultsRoot = results.documentElement;
    let /** @type {Array<TestSuite>} */ suites = [];

    if (includedTests.length == 0) {
        suites = testsCollection;
    }
    else {
        for (const suite of testsCollection) {
            if (includedTests.includes(suite.name)) {
                suites.push(suite);
            }
            else {
                let /** @type {TestSuite} */ filteredSuite = {
                    name: suite.name,
                    tests: []
                };

                for (const test of suite.tests) {
                    if (includedTests.includes(`${suite.name}.${test.name}`)) {
                        filteredSuite.tests.push(test);
                    }
                }

                if (filteredSuite.tests.length > 0)
                    suites.push(filteredSuite);
            }
        }
    }

    const suiteCount = suites.length;
    const totalTestCount = suites.reduce((c, s) => c + s.tests.length, 0);
    console.log(`${C('[==========]', CONSOLE_GREEN)} Running ${totalTestCount} test${(totalTestCount > 1 ? 's' : '')} from ${suiteCount} test suite${(suiteCount > 1 ? 's' : '')}`);

    resultsRoot.setAttribute('name', 'AllTests');
    resultsRoot.setAttribute('tests', totalTestCount.toString());
    resultsRoot.setAttribute('disabled', '0');
    resultsRoot.setAttribute('errors', '0');
    resultsRoot.setAttribute('timestamp', (new Date()).toISOString());

    let totalElapsed = 0;
    let passedCount = 0;
    let /** @type {Array<string>} */ failed = [];

    for (const suite of suites) {
        let /** @type {HTMLElement} */ suiteResults = results.createElement('testsuite');
        let suiteElapsed = 0;
        let suiteFailedCount = 0;
        
        const testCount = suite.tests.length;
        console.log(`${C('[----------]', CONSOLE_GREEN)} ${testCount} test${(testCount > 1 ? 's' : '')} from ${suite.name}`);

        suiteResults.setAttribute('name', suite.name);
        suiteResults.setAttribute('tests', testCount.toString());
        suiteResults.setAttribute('disabled', '0');
        suiteResults.setAttribute('skipped', '0');
        suiteResults.setAttribute('errors', '0');
        suiteResults.setAttribute('timestamp', (new Date()).toISOString());

        for (const test of suite.tests) {        
            let /** @type {HTMLElement} */ testResult = results.createElement('testcase');

            console.log(`${C('[ RUN      ]', CONSOLE_GREEN)} ${suite.name}.${test.name}`);

            testResult.setAttribute('name', test.name);
            testResult.setAttribute('classname', suite.name);
            testResult.setAttribute('status', 'run');
            testResult.setAttribute('result', 'completed');
            testResult.setAttribute('timestamp', (new Date()).toISOString());

            cleanupFunctions.length = 0;
            CSPFoundation.initialise(CHS_ENDPOINT_BASE_URI, "OKO_TESTS");

            let userAgent = ClientUserAgent.create();
            userAgent.cSPVersion  = CSPFoundation.getVersion(),
            userAgent.clientOS		  = "WASMTestsOS",
            userAgent.clientSKU		  = "WASMTest",
            userAgent.clientVersion	  = CSPFoundation.getVersion(),
            userAgent.clientEnvironment = "ODev",
            userAgent.cHSEnvironment  = "oDev"

            CSPFoundation.setClientUserAgentInfo(userAgent);

            const startTime = performance.now();
            let passed = false;
            
            try {
                await test.body();
                passed = true;
            }
            catch(e) {
                if (e instanceof AssertionError) {
                    console.log(`${C('[     FAIL ]', CONSOLE_RED)} Assert failed (${e.condition})\n${e.stack}`);
                } else {
                    console.log(`${C('[     FAIL ]', CONSOLE_RED)} ${e}\n${e.stack}`);

                    throw new FatalError(e.message);
                }

                let /** @type {HTMLElement} */ testFailure = results.createElement('failure');
                testFailure.setAttribute('message', 'TODO: Print real error message.');
                testFailure.setAttribute('type', '');
                testFailure.innerHTML = '<![CDATA[TODO: Print real error message.]]>';
                testResult.appendChild(testFailure);
            }
            
            while (cleanupFunctions.length)
            {
                const func = cleanupFunctions.pop();
                await func();
            }

            const endTime = performance.now();
            const elapsed = endTime - startTime;
            
            if (passed) {
                console.log(`${C('[       OK ]', CONSOLE_GREEN)} ${suite.name}.${test.name} (${elapsed} ms)`);
                passedCount++;
            }
            else {
                console.log(`${C('[     FAIL ]', CONSOLE_RED)} ${suite.name}.${test.name} (${elapsed} ms)`);
                failed.push(`${suite.name}.${test.name}`);
                suiteFailedCount++;
            }

            suiteElapsed += elapsed;

            CSPFoundation.shutdown();

            testResult.setAttribute('time', (elapsed / 1000.0).toFixed(3).toString());

            suiteResults.appendChild(testResult);
        }

        totalElapsed += suiteElapsed;
        console.log(`${C('[----------]', CONSOLE_GREEN)} ${testCount} test${(testCount > 1 ? 's' : '')} from ${suite.name} (${suiteElapsed} ms total)\n`);

        suiteResults.setAttribute('failures', suiteFailedCount.toString());
        suiteResults.setAttribute('time', (suiteElapsed / 1000.0).toFixed(3).toString());

        resultsRoot.appendChild(suiteResults);
    }

    console.log();
    console.log(`${C('[==========]', CONSOLE_GREEN)} ${totalTestCount} test${(totalTestCount > 1 ? 's' : '')} from test suite${(suiteCount > 1 ? 's' : '')} ran (${totalElapsed} ms total)`);

    if (passedCount > 0)
        console.log(`${C('[  PASSED  ]', CONSOLE_GREEN)} ${passedCount} test${(passedCount > 1 ? 's' : '')}`);
    
    if (failed.length > 0) {
        console.log(`${C('[  FAILED  ]', CONSOLE_RED)} ${failed.length} test${(failed.length > 1 ? 's' : '')}, listed below:`);

        for (var name of failed)
            console.log(`${C('[  FAILED  ]', CONSOLE_RED)} ${name}`);
    }

    resultsRoot.setAttribute('time', (totalElapsed / 1000.0).toFixed(3).toString());
    resultsRoot.setAttribute('failures', failed.length.toString());

    return results;
}


export const assert = {
    /**
     * Asserts that two values are equal
     * @param {*} a 
     * @param {*} b 
     */
    areEqual(a, b) {
        if (a !== b)
            throw new AssertionError(`${a} === ${b}`);
    },

    /**
     * Asserts that two numbers are approximately equal with a tolerance of `Number.EPSILON`
     * @param {number} a 
     * @param {number} b 
     */
    areApproximatelyEqual(a, b) {
        if (Math.abs(a - b) > Number.EPSILON)
            throw new AssertionError(`${a} ~= ${b}`);
    },

    /**
     * Asserts that two values are not equal
     * @param {*} a 
     * @param {*} b 
     */
    areNotEqual(a, b) {
        if (a === b)
            throw new AssertionError(`${a} !== ${b}`);
    },

    /**
     * Asserts that a boolean value is `true`
     * @param {boolean} a 
     */
    isTrue(a) {
        if (a !== true)
            throw new AssertionError(`${a} is truthy`);
    },

    /**
     * Asserts that a boolean value is `false`
     * @param {boolean} a 
     */
    isFalse(a) {
        if (a !== false)
            throw new AssertionError(`${a} is falsy`);
    },

    /**
     * Asserts that a number is less than another number
     * @param {number} a 
     * @param {number} b 
     */
    isLessThan(a, b) {
        if (a >= b)
            throw new AssertionError(`${a} < ${b}`);
    },

    /**
     * Asserts that a number is less than or equal to another number
     * @param {number} a 
     * @param {number} b 
     */
    isLessThanOrEqual(a, b) {
        if (a > b)
            throw new AssertionError(`${a} <= ${b}`);
    },

    /**
     * Asserts that a number is greater than another number
     * @param {number} a 
     * @param {number} b 
     */
    isGreaterThan(a, b) {
        if (a <= b)
            throw new AssertionError(`${a} > ${b}`);
    },

    /**
     * Asserts that a number is greater than or equal to another number
     * @param {number} a 
     * @param {number} b 
     */
    isGreaterOrEqualThan(a, b) {
        if (a < b)
            throw new AssertionError(`${a} >= ${b}`);
    },

    /**
     * Asserts that a ResultBase instance contains the expected ResultCode value
     * @param {!Systems.ResultBase} a 
     * @param {Systems.EResultCode} [expectedResultCode] 
     */
    succeeded(a, expectedResultCode = Systems.EResultCode.Success) {
        const resCode = a.getResultCode();

        if (resCode !== expectedResultCode)
            throw new AssertionError(`${resCode} === ${expectedResultCode}`)
    },

    /**
     * Asserts that a ResultBase instance contains the expected ResultCode value
     * @param {!Systems.ResultBase} a 
     * @param {Systems.EResultCode} [expectedResultCode] 
     */
    failed(a, expectedResultCode = Systems.EResultCode.Success) {
        const resCode = a.getResultCode();

        if (resCode === expectedResultCode)
            throw new AssertionError(`${resCode} !== ${expectedResultCode}`)
    }
}