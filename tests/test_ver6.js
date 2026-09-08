'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
const root = path.join(__dirname, '..');
const binary = path.join(root, 'build', process.platform === 'win32' ? 'benchmark.exe' : 'benchmark');
const model = require('../viewer/model.js');
let checks = 0;
function check(value, description) { assert.ok(value, description); checks++; }
function invoke(args, status = 0) {
  const result = spawnSync(binary, args, { encoding: 'utf8', timeout: 30000 });
  assert.ifError(result.error); assert.equal(result.status, status, result.stderr);
  return result;
}
function readCsv(text) {
  const [header, ...lines] = text.trim().split(/\r?\n/);
  const columns = header.split(',');
  return lines.map(line => Object.fromEntries(line.split(',').map((value, i) => [columns[i], value])));
}
const opts = ['--cases', '25', '--processes', '100', '--seed', '42'];
for (const pattern of ['mixed', 'burst', 'sparse']) {
  const rows = readCsv(invoke([...opts, '--pattern', pattern]).stdout);
  const again = readCsv(invoke([...opts, '--pattern', pattern]).stdout);
  check(rows.length === 7, 'Seven benchmark configurations');
  check(new Set(rows.map(r => r.input_checksum)).size === 1, 'Paired input fingerprints');
  for (const row of rows) {
    check(row.pattern === pattern && row.cases === '25' && row.processes === '100' && row.seed === '42', 'Case metadata');
    check(Number(row.batch_elapsed_ms) >= 0 && Number(row.elapsed_ms_per_case) >= 0 && row.timer === 'timespec_get_TIME_UTC', 'Timer metadata');
  }
  const scan = rows.find(r => r.algorithm === 'SJF_SCAN'), heap = rows.find(r => r.algorithm === 'SJF_HEAP');
  for (const field of ['average_waiting', 'average_turnaround', 'average_response', 'average_makespan']) check(scan[field] === heap[field], 'Scan and heap agree');
  rows.forEach((row, i) => {
    for (const field of Object.keys(row).filter(k => !['batch_elapsed_ms', 'elapsed_ms_per_case'].includes(k))) {
      assert.equal(row[field], again[i][field]);
    }
  });
  check(true, 'Seed replay is deterministic except elapsed timing');
  if (pattern === 'sparse') check(rows.every(r => Number(r.average_waiting) === 0), 'Sparse workload has no waiting');
}
const changed = readCsv(invoke(['--cases', '25', '--processes', '100', '--seed', '43']).stdout);
check(changed[0].input_checksum !== readCsv(invoke(opts).stdout)[0].input_checksum, 'Different seed changes input');
check(invoke(['--help']).stdout.includes('--cases'), 'Benchmark help');
for (const flags of [['--cases', '0'], ['--cases', '10001'], ['--processes', '101'], ['--seed', '-1'], ['--seed', '4294967296'], ['--pattern', 'bad'], ['--cases'], ['--cases', '1', '--cases', '2'], ['--wat', '1']]) check(invoke(flags, 1).stderr.includes('Invalid benchmark arguments'), 'Bad arguments rejected');
const directory = fs.mkdtempSync(path.join(root, 'build', 'ver6-test-'));
const file = path.join(directory, 'result with spaces.csv');
invoke([...opts, '--csv', file]);
const original = fs.readFileSync(file, 'utf8');
check(readCsv(original).length === 7, 'CSV file output');
check(invoke([...opts, '--csv', file], 1).stderr.includes('Cannot create CSV'), 'Existing result rejected');
check(fs.readFileSync(file, 'utf8') === original, 'Existing result preserved');
const sample = model.parse(fs.readFileSync(path.join(root, 'examples/ver5-sample/results.json'), 'utf8'));
const rr = sample.runs.find(r => r.algorithm === 'RR');
const states = model.stateAt(rr, 4);
check(model.filterStates(states).length === 3, 'Default filter');
check(model.filterStates(states, 'running')[0].id === 3, 'Running filter');
check(model.filterStates(states, 'waiting').length === 2, 'Waiting filter');
check(model.filterStates(states, 'all', ' p1 ')[0].id === 1, 'ID case and whitespace');
check(model.filterStates(states, 'running', 'P1').length === 0, 'Combined filters');
check(model.filterStates(states, 'all', 'P99').length === 0, 'Empty result');
check(model.filterStates(model.stateAt(rr, 9), 'complete').length === 3, 'Time-dependent filter');
check(states.length === 3 && states[0].status !== 'complete', 'Filter keeps source unchanged');
console.log(`PASS: ${checks} ver6 benchmark/filter checks.`);
