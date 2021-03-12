#!/usr/bin/env node
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.await_future = exports.await_function = exports.destroy = exports.initialize = exports.test = exports.load_from_package = exports.execution_path = exports.discover = exports.clear = exports.load_from_memory = exports.load_from_file = void 0;
const fs_1 = require("fs");
const Module = require("module");
const os_1 = require("os");
const path = require("path");
/** Native node require(TypeScript) */
const ts = require("typescript");
const patched_require = Module.prototype.require;
const node_cache = Module.prototype.node_cache || require.cache;
/** Unpatch metacall_require if present */
if (Module.prototype.node_require) {
    Module.prototype.require = Module.prototype.node_require;
const ts = Module.prototype.require(path.join(__dirname, "node_modules", "typescript"));
/** Patch metacall_require if present */
if (Module.prototype.node_require) {
    Module.prototype.require = patched_require;
}
const noop = () => ({});
const discoverTypes = new Map();
/** Logging util */
const log = process.env.METACALL_DEBUG ? console.log : noop;
/** Util: Wraps a function in try / catch and possibly logs */
const safe = (f, def) => (...args) => {
    try {
        log(f.name, "<-", ...args);
        const res = f(...args);
        log(f.name, "->", res);
        return res;
    }
    catch (err) {
        console.log(f.name, "ERR", err.message);
        console.log(err.stack);
        return def;
    }
};
const wrapFunctionExport = (e) => typeof e === "function" ? { [e.name]: e } : e;
const defaultCompilerOptions = {
    target: ts.ScriptTarget.ES2019,
    module: ts.ModuleKind.CommonJS,
};
const getCompilerOptions = () => {
    const configFile = ts.findConfigFile("./", ts.sys.fileExists, "tsconfig.json");
    if (!configFile) {
        return defaultCompilerOptions;
    }
    const { config, error } = ts.readConfigFile(configFile, ts.sys.readFile);
    if (error) {
        return defaultCompilerOptions;
    }
    const { errors, options } = ts.convertCompilerOptionsFromJson(config, "./", configFile);
    if (errors.length > 0) {
        return defaultCompilerOptions;
    }
    return options;
};
const getMetacallExportTypes = (p, cb = () => { }) => {
    const exportTypes = {};
    const sourceFiles = p.getRootFileNames().map((name) => [name, p.getSourceFile(name)]);
    for (const [fileName, sourceFile] of sourceFiles) {
        if (!sourceFile) {
            console.log(fileName, "not there");
            continue;
        }
        const c = p.getTypeChecker();
        const sym = c.getSymbolAtLocation(sourceFile);
        if (!sym) {
            console.log(fileName, "missing sym");
            continue;
        }
        const moduleExports = c.getExportsOfModule(sym);
        for (const e of moduleExports) {
            const metacallType = (exportTypes[e.name] = exportTypes[e.name] || {});
            const exportType = c.getTypeOfSymbolAtLocation(e, sourceFile);
            const callSignatures = exportType.getCallSignatures();
            if (callSignatures.length === 0) {
                continue;
            }
            for (const signature of callSignatures) {
                const parameters = signature.getParameters();
                metacallType.signature = parameters.map((p) => p.name);
                metacallType.types = parameters.map((p) => c.typeToString(c.getTypeOfSymbolAtLocation(p, p.valueDeclaration)));
                metacallType.ret = c.typeToString(signature.getReturnType());
                // TODO: figure out some nicer way to do this
                metacallType.async = metacallType.ret.startsWith("Promise<") &&
                    metacallType.ret.endsWith(">");
            }
        }
        cb(sourceFile, exportTypes);
    }
    return exportTypes;
};
/** Loads a TypeScript file from disk */
exports.load_from_file = safe(function load_from_file(paths) {
    const result = {};
    const p = ts.createProgram(paths, getCompilerOptions());
    getMetacallExportTypes(p, (sourceFile, exportTypes) => {
        p.emit(sourceFile, (fileName, data) => {
            var _a;
            const m = new Module(fileName);
            m._compile(data, fileName);
            const wrappedExports = wrapFunctionExport(m.exports);
            for (const [name, handle] of Object.entries(exportTypes)) {
                handle.ptr = wrappedExports[name];
            }
            discoverTypes.set(fileName, {
                ...((_a = discoverTypes.get(fileName)) !== null && _a !== void 0 ? _a : {}),
                ...exportTypes,
            });
            result[fileName] = { __esModule: true, ...wrappedExports };
        });
    });
    return result;
}, {});
/** Loads a TypeScript file from memory */
exports.load_from_memory = safe(function load_from_memory(name, data) {
    var _a;
    const compilerOptions = getCompilerOptions();
    const transpileOutput = ts.transpileModule(data, { compilerOptions });
    const extName = `${name}.ts`;
    const target = (_a = compilerOptions.target) !== null && _a !== void 0 ? _a : ts.ScriptTarget.ES2019;
    const p = ts.createProgram([extName], getCompilerOptions(), {
        fileExists: (fileName) => fileName === extName,
        getCanonicalFileName: (fileName) => fileName,
        getCurrentDirectory: ts.sys.getCurrentDirectory,
        getDefaultLibFileName: ts.getDefaultLibFileName,
        getNewLine: () => os_1.EOL,
        getSourceFile: (fileName) => {
            if (fileName === extName) {
                return ts.createSourceFile(fileName, data, target);
            }
            if (fileName.endsWith(".d.ts")) {
                try {
                    return ts.createSourceFile(fileName, fs_1.readFileSync(path.join(__dirname, "node_modules", "typescript", "lib", fileName), "utf8"), target);
                }
                catch (err) {
                    return ts.createSourceFile(fileName, fs_1.readFileSync(fileName, "utf8"), target);
                }
            }
        },
        readFile: (fileName) => fileName === extName ? data : undefined,
        useCaseSensitiveFileNames: () => true,
        writeFile: () => { },
    });
    const exportTypes = getMetacallExportTypes(p);
    const m = new Module(name);
    m._compile(transpileOutput.outputText, name);
    const result = {
        __esModule: true,
        [name]: wrapFunctionExport(m.exports),
    };
    for (const [n, handle] of Object.entries(exportTypes)) {
        handle.ptr = result[name][n];
    }
    discoverTypes.set(name, exportTypes);
    return result;
}, {});
/** Unloads a TypeScript file using handle returned from load_from_file / load_from_memory */
exports.clear = safe(function clear(handle) {
    for (const name of Object.keys(handle)) {
        const absolute = require.resolve(name);
        discoverTypes.delete(name);
        delete node_cache[absolute];
    }
}, undefined);
/** Returns type information about exported functions from a given handle */
exports.discover = safe(function discover(handle) {
    const result = Object.keys(handle)
        .reduce((acc, k) => { var _a; return ({ ...acc, ...(_a = discoverTypes.get(k)) !== null && _a !== void 0 ? _a : {} }); }, {});
    return result;
}, {});
/** Unimplemented */
exports.execution_path = noop;
/** Unimplemented */
exports.load_from_package = noop;
/** Unimplemented */
exports.test = noop;
/** Unimplemented */
exports.initialize = noop;
/** Unimplemented */
exports.destroy = noop;
/** Unimplemented */
exports.await_function = noop;
/** Unimplemented */
exports.await_future = noop;
