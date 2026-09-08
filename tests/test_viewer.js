'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const viewer = require('../viewer/model.js');

const root = path.join(__dirname, '..');
const jsonPath = path.join(root, 'examples/ver5-sample/results.json');
const data = viewer.parse(fs.readFileSync(jsonPath, 'utf8'));
let checks = 0;
function check(condition, message) {
  assert.ok(condition, message);
  checks += 1;
}

check(data.version === '5.0.0', 'V5 sample version');
check(data.input.length === 3 && data.runs.length === 3, 'sample shape');
check(data.makespan === 9, 'sample makespan');
const rr = data.runs.find(run => run.algorithm === 'RR');
check(rr.quantum === 2 && rr.timeline.length === 6, 'RR metadata');
check(viewer.sliceAt(rr, 0).process_id === 1, 'slice at beginning');
check(viewer.sliceAt(rr, 2).process_id === 2, 'slice at boundary');
check(viewer.sliceAt(rr, 8.99).process_id === 1, 'slice before end');
check(viewer.sliceAt(rr, 9) === null, 'slice at completion');
check(viewer.stepTime(rr, 0, 1) === 2, 'next boundary');
check(viewer.stepTime(rr, 2, 1) === 4, 'next second boundary');
check(viewer.stepTime(rr, 2, -1) === 0, 'previous boundary');
check(viewer.stepTime(rr, 2.5, -1) === 2, 'previous from middle');
const start = viewer.stateAt(rr, 0);
check(start.find(p => p.id === 1).status === 'running', 'initial running state');
check(start.find(p => p.id === 2).status === 'pending', 'initial pending state');
const middle = viewer.stateAt(rr, 4);
check(middle.find(p => p.id === 3).status === 'running', 'middle running state');
check(middle.find(p => p.id === 1).executed === 2, 'RR accumulated execution');
check(middle.find(p => p.id === 1).remaining === 3, 'RR remaining time');
const done = viewer.stateAt(rr, 9);
check(done.every(p => p.status === 'complete' && p.remaining === 0), 'completed state');
const invalidJson = [
  '{',
  JSON.stringify({ ...JSON.parse(fs.readFileSync(jsonPath, 'utf8')), schema_version: 2 }),
  JSON.stringify({ ...JSON.parse(fs.readFileSync(jsonPath, 'utf8')), assumptions: { cpu_count: 2, context_switch_cost: 0, io_wait: false } }),
  JSON.stringify({ ...JSON.parse(fs.readFileSync(jsonPath, 'utf8')), runs: [] }),
  JSON.stringify({ ...JSON.parse(fs.readFileSync(jsonPath, 'utf8')), simulator_version: 'bad' })
];
for (const text of invalidJson) {
  assert.throws(() => viewer.parse(text), /지원|JSON|CPU|알고리즘|버전/);
  checks += 1;
}
const tooLarge = ' '.repeat(viewer.MAX_FILE_BYTES + 1);
assert.throws(() => viewer.parse(tooLarge), /16 MB/);
checks += 1;
console.log(`PASS: ${checks} viewer model checks.`);
