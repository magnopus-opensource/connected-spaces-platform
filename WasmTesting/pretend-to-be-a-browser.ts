// Whist we build WASM in node mode, some of our generated code still expects us to be a browser
// This simulates that, more or less.

// This one is actually important, lets us do web requests whilst in node. (Polyfill via xhr)
import XHR from 'xhr2';
(globalThis as any).XMLHttpRequest = XHR;

if (typeof globalThis.navigator === 'undefined') {
  Object.defineProperty(globalThis, 'navigator', {
    value: { userAgent: 'Node.js' },
    configurable: true
  });
}

if (typeof globalThis.localStorage === 'undefined') {
  globalThis.localStorage = {
    store: new Map<string, string>(),
    getItem(key: string) {
      return this.store.get(key) ?? null;
    },
    setItem(key: string, value: string) {
      this.store.set(key, value);
    },
    removeItem(key: string) {
      this.store.delete(key);
    },
    clear() {
      this.store.clear();
    }
  };
}

if (typeof globalThis.window === 'undefined') {
  (globalThis as any).window = {
    navigator: globalThis.navigator,
    localStorage: globalThis.localStorage,
  };
}