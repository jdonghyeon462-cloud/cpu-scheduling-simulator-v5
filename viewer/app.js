(function () {
  'use strict';
  const model = window.SchedulerViewer;
  const $ = id => document.getElementById(id);
  const colors = ['#66e3bd', '#ffbb70', '#afa4ff', '#77ccff', '#ff92ae', '#d0e885', '#efacff', '#79dedf'];
  const statuses = { pending: '미도착', waiting: '대기', running: '실행 중', complete: '완료' };
  let experiment, selected = 0, time = 0, playing = false, lastFrame = null;
  let laneViews = [], processViews = new Map(), importRequest = 0;
  const format = value => Number.isInteger(value) ? String(value) : value.toFixed(2);
  function element(tag, text, className) {
    const node = document.createElement(tag);
    if (text !== undefined) node.textContent = text;
    if (className) node.className = className;
    return node;
  }
  function color(id) { return colors[experiment.input.findIndex(p => p.id === id) % colors.length]; }
  function setPlaying(value) {
    playing = value; lastFrame = null;
    $('play').textContent = value ? '일시정지' : '재생';
    $('play').setAttribute('aria-pressed', String(value));
  }
  function drawLane(view) {
    const { canvas, run } = view;
    const width = Math.max(1, Math.round(canvas.getBoundingClientRect().width));
    const ratio = Math.min(window.devicePixelRatio || 1, 2);
    canvas.width = width * ratio; canvas.height = 46 * ratio;
    const ctx = canvas.getContext('2d');
    ctx.scale(ratio, ratio); ctx.clearRect(0, 0, width, 46);
    ctx.font = '600 13px Segoe UI, sans-serif'; ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
    for (const slice of run.timeline) {
      const x = slice.start / experiment.makespan * width;
      const end = slice.end / experiment.makespan * width;
      ctx.fillStyle = slice.process_id === null ? '#38455a' : color(slice.process_id);
      ctx.fillRect(x, 0, Math.max(1, end - x), 46);
      if (end - x > 32) {
        ctx.fillStyle = slice.process_id === null ? '#e5edfc' : '#0e2430';
        ctx.fillText(slice.process_id === null ? 'IDLE' : `P${slice.process_id}`, (x + end) / 2, 23);
      }
      if (end - x > 4) { ctx.fillStyle = '#0d1626'; ctx.fillRect(end - 1, 0, 1, 46); }
    }
  }
  function buildProcesses() {
    const run = experiment.runs[selected];
    $('active-label').textContent = run.algorithm;
    $('policy').textContent = run.algorithm === 'RR' ? `시간 할당량 ${run.quantum} ticks · 새 도착을 먼저 넣고 현재 작업 재삽입` :
      run.algorithm === 'SJF' ? '현재 도착한 작업 중 실행 시간 → 도착 시간 → 입력 순서로 선택' : '도착 시간 → 입력 순서로 선택';
    processViews = new Map();
    const rows = [...run.processes].sort((a, b) => a.id - b.id).map(p => {
      const row = element('tr');
      const name = element('td'); const label = element('span', undefined, 'job-label');
      const swatch = element('i', undefined, 'swatch'); swatch.style.background = color(p.id);
      label.append(swatch, document.createTextNode(`P${p.id}`)); name.append(label);
      const statusCell = element('td'), status = element('span', '', 'status'); statusCell.append(status);
      const executed = element('td'), remaining = element('td');
      row.append(name, element('td', p.arrival), element('td', p.burst), statusCell, executed, remaining);
      processViews.set(p.id, { row, status, executed, remaining });
      return row;
    });
    $('processes').replaceChildren(...rows);
    [...$('tabs').children].forEach((button, index) => button.setAttribute('aria-pressed', String(index === selected)));
    laneViews.forEach((view, index) => view.row.classList.toggle('selected', index === selected));
    [...$('summary').children].forEach((row, index) => row.classList.toggle('selected-summary', index === selected));
  }
  function render() {
    const run = experiment.runs[selected];
    $('clock').textContent = time.toFixed(2); $('seek').value = String(time);
    $('seek').setAttribute('aria-valuetext', `${time.toFixed(2)} ticks`);
    const percent = Math.max(0, Math.min(100, time / experiment.makespan * 100));
    for (const view of laneViews) {
      view.cursor.style.left = `calc(${percent}% - 1px)`;
      view.future.style.width = `${100 - percent}%`;
    }
    const states = model.stateAt(run, time);
    const visible = model.filterStates(states, $('status-filter').value, $('process-filter').value);
    const visibleIds = new Set(visible.map(p => p.id));
    $('filter-count').textContent = `${visible.length} / ${states.length}개 표시`;
    $('filter-empty').hidden = visible.length > 0;
    for (const p of states) {
      const view = processViews.get(p.id);
      view.row.hidden = !visibleIds.has(p.id);
      view.status.textContent = statuses[p.status]; view.status.className = `status ${p.status}`;
      view.executed.textContent = format(p.executed); view.remaining.textContent = format(p.remaining);
    }
    const slice = model.sliceAt(run, time);
    $('cpu').textContent = time >= run.summary.makespan ? '완료' : slice?.process_id == null ? 'IDLE' : `P${slice.process_id}`;
    $('cpu-note').textContent = time >= run.summary.makespan ? '모든 작업의 실행이 끝났습니다.' : slice ? `${format(slice.start)} → ${format(slice.end)} ticks` : '';
    $('waiting-count').textContent = states.filter(p => p.status === 'waiting').length;
    $('completed-count').textContent = states.filter(p => p.status === 'complete').length;
    $('pending-count').textContent = states.filter(p => p.status === 'pending').length;
    $('previous').disabled = time <= 0;
    $('next').disabled = time >= run.summary.makespan;
    $('reset').disabled = time <= 0 && !playing;
  }
  function load(data, name) {
    setPlaying(false); experiment = data; selected = 0; time = 0;
    $('status-filter').value = 'all'; $('process-filter').value = '';
    $('source-name').textContent = name;
    $('source-meta').textContent = `${data.input.length}개 작업 · ${data.runs.length}개 알고리즘 · 엔진 ${data.version}`;
    $('error').hidden = true; $('error').textContent = '';
    $('end-time').textContent = data.makespan; $('axis-end').textContent = data.makespan;
    $('axis-mid').textContent = format(data.makespan / 2); $('seek').max = data.makespan;
    $('tabs').replaceChildren(...data.runs.map((run, index) => {
      const button = element('button', run.algorithm); button.type = 'button';
      button.addEventListener('click', () => { selected = index; buildProcesses(); render(); });
      return button;
    }));
    laneViews = data.runs.map(run => {
      const row = element('div', undefined, 'timeline-row');
      const lane = element('div', undefined, 'lane');
      const canvas = element('canvas');
      canvas.setAttribute('role', 'img'); canvas.setAttribute('aria-label', `${run.algorithm} 타임라인: ${run.timeline.length}개 구간. 아래 시각 이동과 작업 상태 표로 확인할 수 있습니다.`);
      const future = element('div', undefined, 'future'), cursor = element('div', undefined, 'cursor');
      lane.append(canvas, future, cursor); row.append(element('span', run.algorithm, 'lane-label'), lane);
      return { run, row, canvas, future, cursor };
    });
    $('timeline-rows').replaceChildren(...laneViews.map(view => view.row));
    $('legend').replaceChildren(...data.input.map(p => {
      const item = element('span'); const swatch = element('i', undefined, 'swatch'); swatch.style.background = color(p.id);
      item.append(swatch, document.createTextNode(`P${p.id}`)); return item;
    }), element('span', '회색: IDLE'));
    $('summary').replaceChildren(...data.runs.map(run => {
      const row = element('tr');
      for (const value of [run.algorithm, run.quantum ?? '—', run.summary.average_waiting.toFixed(2), run.summary.average_turnaround.toFixed(2), run.summary.average_response.toFixed(2), run.summary.makespan]) row.append(element('td', value));
      return row;
    }));
    buildProcesses(); laneViews.forEach(drawLane); render();
  }
  $('sample').addEventListener('click', () => { importRequest++; $('file').value = ''; load(model.prepare(window.SCHEDULER_SAMPLE), '기본 예제 · RR q=2'); });
  $('file').addEventListener('change', async event => {
    const file = event.target.files[0]; if (!file) return;
    const request = ++importRequest; setPlaying(false);
    try {
      if (file.size > model.MAX_FILE_BYTES) throw new Error('파일 크기는 16 MB 이하여야 합니다.');
      const text = await file.text();
      if (request !== importRequest) return;
      const data = model.parse(text);
      load(data, file.name);
    } catch (error) {
      if (request === importRequest) { $('error').textContent = error.message; $('error').hidden = false; }
    } finally { if (request === importRequest) $('file').value = ''; }
  });
  $('play').addEventListener('click', () => {
    if (!playing && time >= experiment.makespan) time = 0;
    setPlaying(!playing); render();
  });
  $('reset').addEventListener('click', () => { setPlaying(false); time = 0; render(); });
  $('seek').addEventListener('input', event => { setPlaying(false); time = Number(event.target.value); render(); });
  $('previous').addEventListener('click', () => { setPlaying(false); time = model.stepTime(experiment.runs[selected], time, -1); render(); });
  $('status-filter').addEventListener('change', render);
  $('process-filter').addEventListener('input', render);
  $('next').addEventListener('click', () => { setPlaying(false); time = model.stepTime(experiment.runs[selected], time, 1); render(); });
  window.addEventListener('resize', () => laneViews.forEach(drawLane));
  document.addEventListener('visibilitychange', () => { if (document.hidden) setPlaying(false); });
  function animate(timestamp) {
    if (playing && experiment) {
      if (lastFrame !== null) {
        const delta = Math.min((timestamp - lastFrame) / 1000, 0.1);
        time = Math.min(experiment.makespan, time + delta * experiment.makespan / 12 * Number($('speed').value));
        if (time >= experiment.makespan) setPlaying(false);
        render();
      }
      lastFrame = timestamp;
    }
    requestAnimationFrame(animate);
  }
  load(model.prepare(window.SCHEDULER_SAMPLE), '기본 예제 · RR q=2');
  requestAnimationFrame(animate);
})();
