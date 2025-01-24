import typescript from '@rollup/plugin-typescript';
import { nodeResolve } from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';
import alias from '@rollup/plugin-alias';

import { format } from 'path';
import { fileURLToPath } from 'node:url';

const cppaddonPath = fileURLToPath(
    new URL(
        'build/Release/render2display.node',
        import.meta.url
    )
)

export default {
    input: 'src/main.ts', // Entry point of your TypeScript code
    output: {
        file: 'dist/main.cjs', // Output file
        format: 'commonjs', // Output format
        sourcemap: true, // Generate source maps
    },

    plugins: [
        nodeResolve(), // Resolve Node.js modules
        // commonjs(), // Convert CommonJS modules to ES6
        typescript({ tsconfig: './tsconfig.json' }), // Compile TypeScript
        alias({
            entries: [
                { find: 'render2display_addon', replacement: cppaddonPath },
            ]
        }),
    ],
    external: ['fs', 'path', cppaddonPath,], // Specify Node.js built-in modules as external
};

