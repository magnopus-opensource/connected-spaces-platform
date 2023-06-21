# Example project initialising Foundation with JavaScript

## Setup and Running examples

This example is set up as a small Node project. Using Yarn (or NPM) the following commands will set up dependencies and start the project

### Install and start steps

- `yarn install` - install dependencies.
- `yarn start` - serve this directory with Vercel's `serve`.

## Files

- index.js - this is the main body of JavaScript that is responsible for talking to and using Foundation

- helpers.js - this contains some useful code functions that assist with these hello world examples, either with data formatting, or basic UI functionality.

## Foundation requirements (in a browser environment)

Due to the nature of the Foundation WASM binary requiring access to SharedArrayBuffer, an application that utilises Foundation in a browser environment must supply the following headers for http responses:

```
      'Cross-Origin-Embedder-Policy': 'require-corp',
      'Cross-Origin-Opener-Policy': 'same-origin',
```

A simple way for setting up these headers is to use something like [vercel serve], which we have included within the package.json, serve.json facilitates the setting up of the above headers.
