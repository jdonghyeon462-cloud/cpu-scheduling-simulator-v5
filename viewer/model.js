/* 브라우저와 Node 테스트가 같은 시간 계산 코드를 사용합니다. */
(function (root, factory) {
  const api = factory();
  if (typeof module === 'object' && module.exports) module.exports = api;
  else root.SchedulerViewer = api;
})(typeof globalThis !== 'undefined' ? globalThis : this, function () {
  'use strict';
  const MAX_FILE_BYTES = 16 * 1024 * 1024;
  const MAX_CLOCK = 101000000;
  const policies = { FCFS: 'arrival_then_input', SJF: 'burst_then_arrival_then_input', RR: 'arrivals_before_requeue' };
  function requireValue(condition, message) {
    if (!condition) throw new Error(message);
  }
  function integer(value, min, max, label) {
    requireValue(Number.isSafeInteger(value) && value >= min && value <= max, `${label}: 정수 범위를 확인하세요.`);
  }
  function near(a, b) { return Number.isFinite(a) && Math.abs(a - b) <= 1e-8 * Math.max(1, Math.abs(b)); }
  function upperBound(items, value, key) {
    let lo = 0, hi = items.length;
    while (lo < hi) {
      const middle = (lo + hi) >>> 1;
      if (key(items[middle]) <= value) lo = middle + 1;
      else hi = middle;
    }
    return lo;
  }
  function prepare(data) {
    requireValue(data && typeof data === 'object' && data.schema_version === 1, '지원하는 JSON schema_version은 1입니다.');
    requireValue(typeof data.simulator_version === 'string' && /^\d+\.\d+\.\d+$/.test(data.simulator_version), '프로그램 버전이 올바르지 않습니다.');
    requireValue(data.time_unit === 'ticks' && data.assumptions?.cpu_count === 1 &&
      data.assumptions.context_switch_cost === 0 && data.assumptions.io_wait === false,
      '이 뷰어는 단일 CPU·문맥 교환 비용 0·I/O 없음 모델을 지원합니다.');
    requireValue(Array.isArray(data.input) && data.input.length >= 1 && data.input.length <= 100, '입력 작업 수는 1~100개여야 합니다.');
    const input = data.input.map(p => {
      requireValue(p && typeof p === 'object', '입력 작업이 올바르지 않습니다.');
      integer(p.id, 1, 100, '작업 ID'); integer(p.arrival, 0, 1000000, '도착'); integer(p.burst, 1, 1000000, '실행 시간');
      return { id: p.id, arrival: p.arrival, burst: p.burst };
    });
    const byId = new Map(input.map(p => [p.id, p]));
    requireValue(byId.size === input.length, '작업 ID가 중복됩니다.');
    requireValue(Array.isArray(data.runs) && data.runs.length >= 1 && data.runs.length <= 3, '알고리즘 결과는 1~3개여야 합니다.');
    const seen = new Set();
    const runs = data.runs.map(run => {
      requireValue(run && Object.hasOwn(policies, run.algorithm) && !seen.has(run.algorithm), '알고리즘이 지원되지 않거나 중복됩니다.');
      seen.add(run.algorithm);
      requireValue(run.policy === policies[run.algorithm], '알고리즘의 policy가 올바르지 않습니다.');
      if (run.algorithm === 'RR') integer(run.quantum, 1, 1000000, '시간 할당량');
      else requireValue(run.quantum === null, 'FCFS·SJF의 quantum은 null이어야 합니다.');
      requireValue(Array.isArray(run.processes) && run.processes.length === input.length, '작업별 결과 개수가 다릅니다.');
      const processIds = new Set();
      const processes = run.processes.map(p => {
        requireValue(p && byId.has(p.id) && !processIds.has(p.id), '결과의 작업 ID를 확인하세요.');
        processIds.add(p.id);
        const original = byId.get(p.id);
        requireValue(p.arrival === original.arrival && p.burst === original.burst, '원본 입력과 작업 결과가 다릅니다.');
        for (const field of ['first_start', 'completion', 'waiting', 'turnaround', 'response']) integer(p[field], 0, MAX_CLOCK, field);
        requireValue(p.first_start >= p.arrival && p.completion > p.first_start &&
          p.turnaround === p.completion - p.arrival && p.waiting === p.turnaround - p.burst &&
          p.response === p.first_start - p.arrival, '작업별 시간 계산이 일치하지 않습니다.');
        return { ...original, first_start: p.first_start, completion: p.completion, waiting: p.waiting, turnaround: p.turnaround, response: p.response };
      });
      requireValue(Array.isArray(run.timeline) && run.timeline.length > 0 && run.timeline.length <= 100100, '타임라인 구간 수가 올바르지 않습니다.');
      const tracks = new Map(input.map(p => [p.id, { segments: [], executed: 0 }]));
      let end = 0, executionCount = 0;
      const timeline = run.timeline.map(s => {
        requireValue(s && typeof s === 'object', '타임라인 구간이 올바르지 않습니다.');
        integer(s.start, 0, MAX_CLOCK, '구간 시작'); integer(s.end, 1, MAX_CLOCK, '구간 끝');
        requireValue(s.start === end && s.end > s.start, '타임라인에 겹침·누락·역순 구간이 있습니다.');
        end = s.end;
        if (s.process_id === null) {
          requireValue(input.every(p => p.arrival >= s.end || tracks.get(p.id).executed === p.burst), '실행 가능한 작업이 있는데 IDLE로 기록되었습니다.');
        } else {
          requireValue(byId.has(s.process_id), '타임라인에 알 수 없는 작업 ID가 있습니다.');
          const p = byId.get(s.process_id), track = tracks.get(p.id);
          requireValue(s.start >= p.arrival, '작업이 도착하기 전에 실행되었습니다.');
          if (run.algorithm === 'RR') requireValue(s.end - s.start <= run.quantum, 'RR 실행 구간이 시간 할당량을 넘습니다.');
          else requireValue(track.segments.length === 0, '비선점 알고리즘의 작업이 여러 구간으로 나뉘었습니다.');
          track.segments.push({ start: s.start, end: s.end, before: track.executed });
          track.executed += s.end - s.start;
          executionCount++;
        }
        return { process_id: s.process_id, start: s.start, end: s.end };
      });
      if (run.algorithm === 'RR') requireValue(executionCount <= 100000, 'RR 실행 구간이 100,000개를 넘습니다.');
      for (const p of processes) {
        const track = tracks.get(p.id);
        requireValue(track.executed === p.burst && track.segments[0]?.start === p.first_start &&
          track.segments.at(-1)?.end === p.completion, '실행 기록과 작업별 합계가 일치하지 않습니다.');
      }
      const summary = {
        average_waiting: processes.reduce((n, p) => n + p.waiting, 0) / input.length,
        average_turnaround: processes.reduce((n, p) => n + p.turnaround, 0) / input.length,
        average_response: processes.reduce((n, p) => n + p.response, 0) / input.length,
        makespan: Math.max(...processes.map(p => p.completion))
      };
      requireValue(summary.makespan === end && run.summary && Object.keys(summary).every(k => near(run.summary[k], summary[k])), '요약 통계와 실행 기록이 일치하지 않습니다.');
      return { algorithm: run.algorithm, quantum: run.quantum, policy: run.policy, processes, timeline, summary, tracks, boundaries: [0, ...timeline.map(s => s.end)] };
    });
    return { version: data.simulator_version, input, runs, makespan: Math.max(...runs.map(r => r.summary.makespan)) };
  }
  function parse(text) {
    requireValue(typeof text === 'string' && text.length <= MAX_FILE_BYTES, '파일 크기는 16 MB 이하여야 합니다.');
    let data;
    try { data = JSON.parse(text.replace(/^\uFEFF/, '')); }
    catch { throw new Error('JSON을 읽을 수 없습니다. C 프로그램에서 내보낸 results.json을 선택하세요.'); }
    return prepare(data);
  }
  function sliceAt(run, time) {
    const index = upperBound(run.timeline, time, s => s.start) - 1;
    const slice = run.timeline[index];
    return slice && time < slice.end ? slice : null;
  }
  function stateAt(run, time) {
    const t = Math.max(0, Math.min(Number.isFinite(time) ? time : 0, run.summary.makespan));
    const active = sliceAt(run, t);
    return run.processes.map(p => {
      const segments = run.tracks.get(p.id).segments;
      const index = upperBound(segments, t, s => s.start) - 1;
      const segment = segments[index];
      const executed = segment ? segment.before + Math.min(t - segment.start, segment.end - segment.start) : 0;
      const status = t < p.arrival ? 'pending' : t >= p.completion ? 'complete' : active?.process_id === p.id ? 'running' : 'waiting';
      return { ...p, executed, remaining: Math.max(0, p.burst - executed), status };
    });
  }
  function stepTime(run, time, direction) {
    if (direction > 0) return run.boundaries[upperBound(run.boundaries, time, x => x)] ?? run.summary.makespan;
    const index = upperBound(run.boundaries, time, x => x) - 1;
    return run.boundaries[Math.max(0, run.boundaries[index] === time ? index - 1 : index)];
  }
  function filterStates(states, status = 'all', query = '') {
    const term = String(query).trim().toUpperCase();
    return states.filter(p => (status === 'all' || p.status === status) && (!term || `P${p.id}`.includes(term)));
  }
  return { MAX_FILE_BYTES, prepare, parse, sliceAt, stateAt, stepTime, filterStates };
});
