import { dirname, resolve as _resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

// Resolves a path relative to this file
export function resolve(...args) {
    return _resolve(dirname(fileURLToPath(import.meta.url)), ...args);
}

