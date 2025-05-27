// Whist we build WASM in node mode, some of our generated code still expects us to be a browser
// This simulates that, more or less.

// This one is actually important, lets us do web requests whilst in node. (Polyfill via xhr)
// @ts-ignore (Don't want to write type bindings for XHR)
import XHR from 'xhr2';
(globalThis as any).XMLHttpRequest = XHR;

if (typeof globalThis.navigator === 'undefined') {
  Object.defineProperty(globalThis, 'navigator', {
    value: { userAgent: 'Node.js' },
    configurable: true
  });
}

if (typeof globalThis.localStorage === 'undefined') {
  const store = new Map<string, string>();

  globalThis.localStorage = {
    store: new Map<string, string>(),
    getItem(key: string) {
      return store.get(key) ?? null;
    },
    setItem(key: string, value: string) {
      store.set(key, value);
    },
    removeItem(key: string) {
      store.delete(key);
    },
    clear() {
      store.clear();
    },
    get length() {
      return store.size;
    },
    key(index: number) {
      return Array.from(store.keys())[index] ?? null;
    }
  } as Storage;
}

if (typeof globalThis.window === 'undefined') {
  (globalThis as any).window = {
    navigator: globalThis.navigator,
    localStorage: globalThis.localStorage,
  };
}