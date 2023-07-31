# Example project using Connected Spaces Platform (CSP) with JavaScript

## Setup and Running examples

This example is set up as a small Node project. Using Yarn (or NPM) the following commands will set up dependencies and start the project

### Before running the Yarn steps

- Update `EMAIL` and `PASSWORD` constants in `index.js:26-27` with your email, and desired password.

### Install and start steps

- `yarn install` - install dependencies.
- `yarn start` - serve this directory with Vercel's `serve`.

## Files

- index.js - this is the main body of JavaScript that is responsible for talking to and using CSP. We have a few sections relating to the following areas of CSP, initialisation, authentication, spaces, assets, and multiplayer.

- helpers.js - this contains some useful code functions that assist with these hello world examples, either with data formatting, or basic UI functionality.

## CSP requirements (in a browser environment)

Due to the nature of the CSP WASM binary requiring access to SharedArrayBuffer, an application that utilises CSP in a browser environment must supply the following headers for http responses:

```
      'Cross-Origin-Embedder-Policy': 'require-corp',
      'Cross-Origin-Opener-Policy': 'same-origin',
```

A simple way for setting up these headers is to use something like [vercel serve], which we have included within the package.json, serve.json facilitates the setting up of the above headers.
