import { C, CONSOLE_RED, CONSOLE_GREEN } from './test_helpers.js';
// eslint-disable-next-line no-unused-vars
import { XMLDocument, XMLElement } from './xml.js';

/**
 * Represents an assert failure
 */
export class AssertionError extends Error {
    condition = '';

    /**
     * @param {string} condition
     * @param {any[]} params
     */
    constructor(condition, ...params) {
        super(...params);

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
        super(message);

        //@ts-ignore
        if (Error.captureStackTrace) {
            //@ts-ignore
            Error.captureStackTrace(this, FatalError);
        }

        this.name = 'FatalError';
    }
}

/**
 * @typedef {function(): Promise<void>} TestBody
 */

/**
 * @typedef {object} Test
 * @property {string} name
 * @property {TestBody} body
 */

let /** @type {Array<Test>} */ testsCollection = [];

/**
 * Defines a test
 * @param {string} name
 * @param {TestBody} body
 */
export function test(name, body) {
    testsCollection.push({ name, body });
}

/**
 * Runs all tests
 * @returns {Promise<XMLDocument>}
 */
export async function runTests() {
    const /** @type {XMLDocument} */ results = new XMLDocument('testsuites');

    const totalTestCount = testsCollection.length;
    console.log(
        `${C('[==========]', CONSOLE_GREEN)} Running ${totalTestCount} test${
            totalTestCount > 1 ? 's' : ''
        }`
    );

    results.setAttribute('name', 'AllTests');
    results.setAttribute('tests', totalTestCount.toString());
    results.setAttribute('disabled', '0');
    results.setAttribute('errors', '0');
    results.setAttribute('timestamp', new Date().toISOString());

    let totalElapsed = 0;
    let passedCount = 0;
    const /** @type {Array<string>} */ failed = [];

    const /** @type {XMLElement} */ suiteResults =
            results.createElement('testsuite');
    let suiteElapsed = 0;
    let suiteFailedCount = 0;

    suiteResults.setAttribute('name', 'AllTests');
    suiteResults.setAttribute('tests', totalTestCount.toString());
    suiteResults.setAttribute('disabled', '0');
    suiteResults.setAttribute('skipped', '0');
    suiteResults.setAttribute('errors', '0');
    suiteResults.setAttribute('timestamp', new Date().toISOString());

    for (const test of testsCollection) {
        const /** @type {XMLElement} */ testResult =
                suiteResults.createElement('testcase');

        console.log(`${C('[ RUN      ]', CONSOLE_GREEN)} ${test.name}`);

        testResult.setAttribute('name', test.name);
        testResult.setAttribute('classname', 'AllTests');
        testResult.setAttribute('status', 'run');
        testResult.setAttribute('result', 'completed');
        testResult.setAttribute('timestamp', new Date().toISOString());

        const startTime = performance.now();
        let passed = false;

        try {
            await test.body();
            passed = true;
        } catch (e) {
            if (e instanceof AssertionError) {
                console.log(
                    `${C('[     FAIL ]', CONSOLE_RED)} Assert failed (${
                        e.condition
                    })\n${e.stack}`
                );
            } else {
                console.log(
                    `${C('[     FAIL ]', CONSOLE_RED)} ${e}\n${e.stack}`
                );

                throw new FatalError(e.message);
            }

            const /** @type {XMLElement} */ testFailure =
                    testResult.createElement('failure');
            testFailure.setAttribute(
                'message',
                'TODO: Print real error message.'
            );
            testFailure.setAttribute('type', '');
            testFailure.innerHTML =
                '<![CDATA[TODO: Print real error message.]]>';
        }

        const endTime = performance.now();
        const elapsed = endTime - startTime;

        if (passed) {
            console.log(
                `${C('[       OK ]', CONSOLE_GREEN)} ${
                    test.name
                } (${elapsed} ms)`
            );
            passedCount++;
        } else {
            console.log(
                `${C('[     FAIL ]', CONSOLE_RED)} ${test.name} (${elapsed} ms)`
            );
            failed.push(`${test.name}`);
            suiteFailedCount++;
        }

        suiteElapsed += elapsed;

        testResult.setAttribute(
            'time',
            (elapsed / 1000.0).toFixed(3).toString()
        );
    }

    totalElapsed += suiteElapsed;

    suiteResults.setAttribute('failures', suiteFailedCount.toString());
    suiteResults.setAttribute(
        'time',
        (suiteElapsed / 1000.0).toFixed(3).toString()
    );

    console.log();
    console.log(
        `${C('[==========]', CONSOLE_GREEN)} ${totalTestCount} test${
            totalTestCount > 1 ? 's' : ''
        } ran (${totalElapsed} ms total)`
    );

    if (passedCount > 0)
        console.log(
            `${C('[  PASSED  ]', CONSOLE_GREEN)} ${passedCount} test${
                passedCount > 1 ? 's' : ''
            }`
        );

    if (failed.length > 0) {
        console.log(
            `${C('[  FAILED  ]', CONSOLE_RED)} ${failed.length} test${
                failed.length > 1 ? 's' : ''
            }, listed below:`
        );

        for (var name of failed)
            console.log(`${C('[  FAILED  ]', CONSOLE_RED)} ${name}`);
    }

    results.setAttribute('time', (totalElapsed / 1000.0).toFixed(3).toString());
    results.setAttribute('failures', failed.length.toString());

    return results;
}

export const assert = {
    /**
     * Asserts that two values are equal
     * @param {*} a
     * @param {*} b
     */
    areEqual(a, b) {
        if (a != b) throw new AssertionError(`${a} == ${b}`);
    },

    /**
     * Asserts that two numbers are approximately equal using a dynamic tolerance value
     * @param {number} a
     * @param {number} b
     */
    areApproximatelyEqual(a, b) {
        if (a === undefined || b === undefined)
            throw new AssertionError('One of input values was undefined!');

        const tolerance = Math.max(Math.abs(a), Math.abs(b)) * 1e-8;

        if (Math.abs(a - b) > tolerance)
            throw new AssertionError(`${a} ~= ${b}`);
    },

    /**
     * Asserts that two values are not equal
     * @param {*} a
     * @param {*} b
     */
    areNotEqual(a, b) {
        if (a == b) throw new AssertionError(`${a} != ${b}`);
    },

    /**
     * Asserts that a boolean value is `true`
     * @param {boolean} a
     */
    isTrue(a) {
        if (a !== true) throw new AssertionError(`${a} is truthy`);
    },

    /**
     * Asserts that a boolean value is `false`
     * @param {boolean} a
     */
    isFalse(a) {
        if (a !== false) throw new AssertionError(`${a} is falsy`);
    },

    /**
     * Asserts that a number is less than another number
     * @param {number} a
     * @param {number} b
     */
    isLessThan(a, b) {
        if (a === undefined || b === undefined)
            throw new AssertionError('One of input values was undefined!');

        if (a >= b) throw new AssertionError(`${a} < ${b}`);
    },

    /**
     * Asserts that a number is less than or equal to another number
     * @param {number} a
     * @param {number} b
     */
    isLessThanOrEqual(a, b) {
        if (a === undefined || b === undefined)
            throw new AssertionError('One of input values was undefined!');

        if (a > b) throw new AssertionError(`${a} <= ${b}`);
    },

    /**
     * Asserts that a number is greater than another number
     * @param {number} a
     * @param {number} b
     */
    isGreaterThan(a, b) {
        if (a === undefined || b === undefined)
            throw new AssertionError('One of input values was undefined!');

        if (a <= b) throw new AssertionError(`${a} > ${b}`);
    },

    /**
     * Asserts that a number is greater than or equal to another number
     * @param {number} a
     * @param {number} b
     */
    isGreaterOrEqualThan(a, b) {
        if (a === undefined || b === undefined)
            throw new AssertionError('One of input values was undefined!');

        if (a < b) throw new AssertionError(`${a} >= ${b}`);
    },

    /**
     * Throws an AssertionError
     * @param {string} message The assertion message
     */
    fail(message) {
        throw new AssertionError(message);
    },
};
