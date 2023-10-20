/**
 * Represents a single element in an XML document
 */
export class XMLElement {
    /**
     * @type {string}
     */
    #name;

    #attributes;

    /**
     * @type {Array<XMLElement>}
     */
    #elements;

    /**
     * @type {string}
     */
    innerHTML;

    /**
     * @param {string} qualifiedName
     */
    constructor(qualifiedName) {
        this.#name = qualifiedName;
        this.#attributes = {};
        this.#elements = [];
    }

    get name() {
        return this.#name;
    }

    /**
     * Sets the value of an attribute
     * @param {string} qualifiedName
     * @param {string} value
     */
    setAttribute(qualifiedName, value) {
        this.#attributes[qualifiedName] = value;
    }

    /**
     * Creates and appends a new child element
     * @param {string} tagName
     * @returns {XMLElement}
     */
    createElement(tagName) {
        const element = new XMLElement(tagName);
        this.#elements.push(element);

        return element;
    }

    toString() {
        return this.stringify(0);
    }

    stringify(indent) {
        let result = '';

        result += ' '.repeat(indent * 4);
        result += `<${this.#name}`;

        for (let key in this.#attributes) {
            const /** @type {string} */ val = this.#attributes[key];
            result += ` ${key}="${val}"`;
        }

        if (this.innerHTML == null && this.#elements.length == 0) {
            result += '/>\n';
        } else {
            result += '>\n';

            if (this.innerHTML != null) {
                result += ' '.repeat((indent + 1) * 4);
                result += this.innerHTML + '\n';
            } else {
                for (let elem of this.#elements) {
                    result += elem.stringify(indent + 1);
                }
            }

            result += ' '.repeat(indent * 4);
            result += `</${this.#name}>\n`;
        }

        return result;
    }
}

/**
 * Represents an XML document
 */
export class XMLDocument extends XMLElement {
    /**
     * @param {string} qualifiedName
     */
    constructor(qualifiedName) {
        super(qualifiedName);
    }
}
