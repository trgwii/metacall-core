#!/usr/bin/env bash
./node_modules/.bin/tsc
mv bootstrap_src.js bootstrap.ts
sed -i '16s/.*/const ts = Module.prototype.require(path.join(__dirname, "node_modules", "typescript"));/' bootstrap.ts
