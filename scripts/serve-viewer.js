'use strict';
// 로컬 미리보기용입니다. viewer 폴더의 정해진 파일만 루프백에서 제공합니다.
const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
const root = path.join(__dirname, '..', 'viewer');
const files = { '/': ['index.html', 'text/html'], '/index.html': ['index.html', 'text/html'],
  '/style.css': ['style.css', 'text/css'], '/model.js': ['model.js', 'text/javascript'],
  '/sample-data.js': ['sample-data.js', 'text/javascript'], '/app.js': ['app.js', 'text/javascript'] };
const port = Number(process.argv[2] ?? 8787);
if (!Number.isInteger(port) || port < 0 || port > 65535) throw new Error('Port must be 0..65535.');
const server = http.createServer((req, res) => {
  const entry = Object.hasOwn(files, req.url) ? files[req.url] : null;
  if (!entry || !['GET', 'HEAD'].includes(req.method)) { res.writeHead(404); res.end('Not found'); return; }
  fs.readFile(path.join(root, entry[0]), (error, data) => {
    if (error) { res.writeHead(500); res.end('Cannot read viewer asset'); return; }
    res.writeHead(200, { 'Content-Type': `${entry[1]}; charset=utf-8`, 'Cache-Control': 'no-store', 'X-Content-Type-Options': 'nosniff' });
    res.end(req.method === 'HEAD' ? undefined : data);
  });
});
server.on('error', error => { console.error(error.message); process.exitCode = 1; });
server.listen(port, '127.0.0.1', () => console.log(`Viewer: http://127.0.0.1:${server.address().port}/`));
