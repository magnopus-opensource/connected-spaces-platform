import { Common } from './olympus_foundation.js'


/**
 * Converts a Common.Array to a JS array
 * @template T
 * @param {!Common.Array<T>} array 
 * @returns {T[]}
 */
export function commonArrayToJSArray(array) {
    let _array = [];

    for (let i = 0; i < array.size(); i++)
        _array.push(array.get(i));
    
    return _array;
}


/**
 * @typedef {String} SupportedArrayTypes
 */


/**
 * @typedef {StringConstructor} SupportedArrayTypeConstructors
 */


/**
 * Converts a JS array to a Common.Array
 * @param {!SupportedArrayTypes[]} array
 * @param {SupportedArrayTypeConstructors} elementType
 * @returns {Common.Array<SupportedArrayTypes>}
 */
export function jsArrayToCommonArray(array, elementType) {
    if (array === null)
        return null;
        
    let /** @type {Common.Array<SupportedArrayTypes>} */ _array;

    if (elementType === String)
        _array = Common.Array.ofString_number(array.length);
    else
        throw new Error('Unsupported array type!');

    for (let i = 0; i < array.length; i++)
        _array.set(i, array[i]);

    return _array;
}