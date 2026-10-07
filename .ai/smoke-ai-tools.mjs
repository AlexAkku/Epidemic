// Infrastructure-only smoke checks. No C++ build, full index completion or test suite.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { spawn, spawnSync } from 'node:child_process';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const sdk = path.join(root, '.tools/clangd-mcp-server/node_modules/@modelcontextprotocol/sdk/dist/esm');
const { Client } = await import(pathToFileURL(path.join(sdk, 'client/index.js')).href);
const { StdioClientTransport } = await import(pathToFileURL(path.join(sdk, 'client/stdio.js')).href);
const { LSPClient } = await import(pathToFileURL(path.join(root, '.tools/clangd-mcp-server/dist/lsp-client.js')).href);
const results = {};
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));
const reportPath = path.join(root, '.ai/smoke-report.json');

async function check(name, action) {
  if (process.argv.includes('--ms5') && /repomix/i.test(name)) {
    results[name] = { status: 'NOT_USED', reason: 'MS5 routing excludes Repomix' };
    return;
  }
  try {
    results[name] = { status: 'PASS', ...(await action()) };
    console.log(`${name}: PASS`);
  } catch (error) {
    results[name] = { status: 'FAIL', error: String(error) };
    console.error(`${name}: FAIL - ${error.message}`);
    process.exitCode = 1;
  }
}

function stopOwnedTree(pid) {
  // Only the PID returned by our own spawn/transport is used here.
  if (pid && process.platform === 'win32') {
    spawnSync('taskkill.exe', ['/PID', String(pid), '/T', '/F'], { windowsHide: true, stdio: 'ignore' });
  }
}

function boundedLog(current, chunk) {
  return (current + chunk.toString()).slice(-2 * 1024 * 1024);
}

const database = JSON.parse(fs.readFileSync(path.join(root, 'compile_commands.json'), 'utf8'));
const projectUnits = database.filter(unit => /^Engine(Base|Runtime|Framework)[\\/]/.test(path.relative(root, path.resolve(unit.directory, unit.file))));
const sample = projectUnits.find(unit => /memory_tracker\.cpp$/.test(unit.file)) ?? projectUnits[0];
assert(sample, 'compilation database contains no project sources');
const sampleFile = path.resolve(sample.directory, sample.file);

await check('clangd background index', async () => {
  const resolver = spawnSync('powershell.exe', ['-NoProfile', '-ExecutionPolicy', 'Bypass', '-Command',
    '. ./.ai/ai-tools-common.ps1; Resolve-AiClangd'], { cwd: root, encoding: 'utf8', windowsHide: true });
  assert.equal(resolver.status, 0, resolver.stderr);
  const clangd = spawn(resolver.stdout.trim(), ['--background-index', `--compile-commands-dir=${root}`, '--log=verbose', '-j=2'],
    { cwd: root, stdio: ['pipe', 'pipe', 'pipe'], windowsHide: true });
  let log = '';
  let spawnError;
  clangd.on('error', error => { spawnError = error; });
  clangd.stderr.on('data', data => { log = boundedLog(log, data); });
  const lsp = new LSPClient(clangd.stdin, clangd.stdout);
  try {
    const initialized = await lsp.request('initialize', {
      processId: process.pid, rootUri: pathToFileURL(root + path.sep).href,
      capabilities: { textDocument: { documentSymbol: { hierarchicalDocumentSymbolSupport: true } } }
    }, 30000);
    assert(initialized.capabilities, 'LSP initialize did not return capabilities');
    lsp.notify('initialized', {});
    const uri = pathToFileURL(sampleFile).href;
    lsp.notify('textDocument/didOpen', { textDocument: {
      uri, languageId: 'cpp', version: 1, text: fs.readFileSync(sampleFile, 'utf8')
    } });
    const symbols = await lsp.request('textDocument/documentSymbol', { textDocument: { uri } }, 30000);
    assert(symbols?.length > 0, 'clangd returned no symbols for a real project translation unit');
    const deadline = Date.now() + 30000;
    while (Date.now() < deadline && !(/Loaded compilation database/.test(log) &&
      /Enqueueing .*commands for indexing|BackgroundIndex:|Rebuilt background index/.test(log))) {
      if (spawnError) throw spawnError;
      assert(clangd.exitCode === null, 'clangd exited during indexing');
      await delay(200);
    }
    assert(/Loaded compilation database/.test(log), 'clangd did not load the compilation database');
    assert(/Enqueueing .*commands for indexing|BackgroundIndex:|Rebuilt background index/.test(log), 'no background-index start evidence');
    assert(!/Unknown command line argument|Failed to load compilation database|Error while parsing config|config error|Invalid URI/i.test(log), 'system configuration error in clangd log');
    return { translationUnits: database.length, projectSourceUnits: projectUnits.length,
      sample: path.relative(root, sampleFile), documentSymbols: symbols.length,
      evidence: log.split(/\r?\n/).filter(line => /Loaded compilation database|Enqueueing .*commands for indexing|BackgroundIndex:|Rebuilt background index/.test(line)).slice(0, 8) };
  } finally {
    fs.writeFileSync(path.join(root, '.ai/clangd-index-smoke.log'), log);
    try { await lsp.request('shutdown', null, 5000); lsp.notify('exit'); } catch { /* bounded cleanup below */ }
    lsp.close();
    await delay(250);
    if (clangd.exitCode === null) { stopOwnedTree(clangd.pid); clangd.kill(); }
  }
});

async function withMcp(command, args, logName, action) {
  const transport = new StdioClientTransport({ command, args, cwd: root, stderr: 'pipe',
    env: Object.fromEntries(Object.entries(process.env).filter(([, value]) => value !== undefined)) });
  let log = '';
  transport.stderr?.on('data', data => { log = boundedLog(log, data); });
  const client = new Client({ name: 'epidemic-ai-infrastructure-smoke', version: '1.0.0' });
  try {
    await client.connect(transport, { timeout: 30000 });
    return await action(client, () => log);
  } finally {
    fs.writeFileSync(path.join(root, '.ai', logName), log);
    // Windows SIGTERM does not reliably propagate from PowerShell to Node/clangd.
    // Stop just this smoke test's process tree before closing the transport.
    stopOwnedTree(transport.pid);
    await client.close();
  }
}

await check('clangd MCP', () => withMcp('powershell.exe',
  ['-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', '.ai/run-clangd-mcp.ps1'],
  'clangd-mcp-smoke.log', async (client, log) => {
    const tools = await client.listTools();
    for (const name of ['find_definition', 'find_references', 'find_implementations', 'get_call_hierarchy', 'get_document_symbols']) {
      assert(tools.tools.some(tool => tool.name === name), `missing MCP tool: ${name}`);
    }
    const response = await client.callTool({ name: 'get_document_symbols', arguments: { file_path: sampleFile } }, undefined, { timeout: 90000 });
    assert(!response.isError, JSON.stringify(response));
    const symbols = JSON.parse(response.content.find(item => item.type === 'text').text);
    assert(symbols.found && symbols.count > 0, 'MCP failed to navigate a project translation unit');
    const sourceLines = fs.readFileSync(sampleFile, 'utf8').split(/\r?\n/);
    const symbolLine = sourceLines.findIndex(line => line.includes('MemoryTracker::'));
    let definitionCount;
    if (symbolLine >= 0) {
      const definition = await client.callTool({ name: 'find_definition', arguments: {
        file_path: sampleFile, line: symbolLine, column: sourceLines[symbolLine].indexOf('MemoryTracker')
      } }, undefined, { timeout: 30000 });
      assert(!definition.isError, 'MCP definition request failed');
      const locations = JSON.parse(definition.content.find(item => item.type === 'text').text);
      assert(locations.found && locations.count > 0, 'MCP returned no class definition');
      definitionCount = locations.count;
    }
    assert(!/Failed to start clangd|Unknown command line argument|Invalid URI/.test(log()), 'MCP clangd configuration error');
    return { tools: tools.tools.map(tool => tool.name), sample: path.relative(root, sampleFile), symbols: symbols.count, definitions: definitionCount };
  }));

await check('Repomix MCP sandbox', () => withMcp(process.execPath,
  [path.join(root, 'node_modules/repomix/bin/repomix.cjs'), '--mcp', '--sandbox', '.'],
  'repomix-mcp-smoke.log', async client => {
    const tools = await client.listTools();
    assert(tools.tools.some(tool => tool.name === 'pack_codebase'));
    assert(!tools.tools.some(tool => tool.name === 'pack_remote_repository'), 'unrestricted remote pack tool exposed');
    const read = await client.callTool({ name: 'file_system_read_file', arguments: { path: 'package.json' } });
    assert(!read.isError, 'sandbox cannot read a workspace-relative file');
    for (const forbidden of ['../package.json', path.join(root, 'package.json')]) {
      const rejected = await client.callTool({ name: 'file_system_read_file', arguments: { path: forbidden } });
      assert(rejected.isError, `sandbox accepted forbidden path: ${forbidden}`);
    }
    return { tools: tools.tools.map(tool => tool.name), workspaceRead: 'PASS', traversalRejected: true, absolutePathRejected: true };
  }));

fs.writeFileSync(reportPath, JSON.stringify({ checkedAt: new Date().toISOString(), results }, null, 2) + '\n');
console.log(`Report: ${path.relative(root, reportPath)}`);
