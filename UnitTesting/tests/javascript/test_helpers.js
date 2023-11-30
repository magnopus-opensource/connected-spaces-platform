/**
 * Sleeps for the specified number of milliseconds
 * @param {number} ms The number of milliseconds to sleep for
 * @returns {Promise<void>} A promise that can be awaited in order to sleep on the current thread
 */
export function sleep(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

/**
 * Creates a deep copy of an object by converting it to and then from JSON
 * @template T
 * @param {T} object The object to create a deep copy of
 * @returns {T} The deep copy of the provided object
 */
export function deepCopy(object) {
    return JSON.parse(JSON.stringify(object));
}

/**
 * Generates a random hex string of a given length
 * @param {number} length
 * @returns {string}
 */
export function getUniqueHexString(length = 8) {
    var arr = new Array(length);

    for (var i = 0; i < length; i++) {
        arr[i] = Math.floor(Math.random() * 16).toString(16);
    }

    return arr.join('');
}

/**
 * Appends a random 64-bit hex string to a given string
 * @param {string} rootString The string to append to
 * @returns {string}
 */
export function generateUniqueString(rootString) {
    return `${rootString}-${getUniqueHexString()}`;
}

export const CONSOLE_RED = '\x1b[31m';
export const CONSOLE_GREEN = '\x1b[32m';
export const CONSOLE_RESET = '\x1b[0m';

/**
 * Returns a string coloured using ANSI escape sequences
 * @param {string} message
 * @param {string} colour
 * @returns
 */
export function C(message, colour) {
    return colour + message + CONSOLE_RESET;
}

export const CHS_ENDPOINT_BASE_URI = 'https://ogs-odev-internal.magnoboard.com';
