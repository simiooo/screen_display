import typescript from '@rollup/plugin-typescript';
import { nodeResolve } from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';
import alias from '@rollup/plugin-alias';
import clear from 'rollup-plugin-clear'

import { format } from 'path';
import { fileURLToPath } from 'node:url';

const cppaddonPath = fileURLToPath(
    new URL(
        'build/Release/render2display.node',
        import.meta.url
    )
)
function createConfig(output = {
    file: 'dist/main.cjs', // Output file
    format: 'cjs', // Output format
    sourcemap: true, // Generate source maps
}) {
    return {
        input: 'src/main.ts', // Entry point of your TypeScript code
        output,
        plugins: [
            nodeResolve(), // Resolve Node.js modules
            // commonjs(), // Convert CommonJS modules to ES6
            typescript({ tsconfig: './tsconfig.json' }), // Compile TypeScript
            alias({
                entries: [
                    { find: 'render2display_addon', replacement: cppaddonPath },
                ]
            }),
            clear({
                // required, point out which directories should be clear.
                targets: ['dist'],
            })
        ],
        external: ['fs', 'path', cppaddonPath,], // Specify Node.js built-in modules as external
    }
}
export default [
    createConfig(),
    createConfig({
        file: 'dist/main.mjs',
        format: 'esm',
        sourcemap: true,
    }),
];

