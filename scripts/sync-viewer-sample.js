'use strict';
const fs = require('node:fs');
const path = require('node:path');
const root = path.join(__dirname, '..');
const input = fs.readFileSync(path.join(root, 'examples/ver5-sample/results.json'), 'utf8');
require('../viewer/model.js').parse(input);
fs.writeFileSync(path.join(root, 'viewer/sample-data.js'), '// Generated from examples/ver5-sample/results.json.\nwindow.SCHEDULER_SAMPLE = ' + JSON.stringify(JSON.parse(input), null, 2) + ';\n');
console.log('Viewer sample updated.');
