// import type from 'direct2d_display'

import { createRequire } from 'module';
const require = createRequire(import.meta.url);
const { Direct2DDisplay } = require('#render2display_addon');
export { Direct2DDisplay }