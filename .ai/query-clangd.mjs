import path from 'node:path';
import { pathToFileURL, fileURLToPath } from 'node:url';
import { spawnSync } from 'node:child_process';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const sdk = path.join(root, '.tools/clangd-mcp-server/node_modules/@modelcontextprotocol/sdk/dist/esm');
const { Client } = await import(pathToFileURL(path.join(sdk, 'client/index.js')));
const { StdioClientTransport } = await import(pathToFileURL(path.join(sdk, 'client/stdio.js')));
const client = new Client({ name: 'epidemic-ms5-navigation', version: '1.0.0' });

const MAX_ITEMS = 12;
const MAX_STRING = 4000;

function compact(value, depth = 0) {
  if (value == null) return value;
  if (depth > 8) return '[max depth reached]';

  if (typeof value === 'string') {
    return value.length > MAX_STRING
      ? value.slice(0, MAX_STRING) + `...[truncated ${value.length - MAX_STRING} chars]`
      : value;
  }

  if (Array.isArray(value)) {
    const items = value.slice(0, MAX_ITEMS).map(v => compact(v, depth + 1));
    if (value.length > MAX_ITEMS) {
      items.push({ truncated_items: value.length - MAX_ITEMS });
    }
    return items;
  }

  if (typeof value === 'object') {
    return Object.fromEntries(
      Object.entries(value).map(([k, v]) => [k, compact(v, depth + 1)])
    );
  }

  return value;
}

const transport = new StdioClientTransport({
  command: 'powershell.exe',
  args: [
    '-NoProfile',
    '-ExecutionPolicy', 'Bypass',
    '-File', '.ai/run-clangd-mcp.ps1'
  ],
  cwd: root,
  stderr: 'pipe',
  env: {
    ...Object.fromEntries(
      Object.entries(process.env)
        .filter(([, value]) => value !== undefined)
    ),
    AI_CLANGD_EPHEMERAL: '1'
  }
});

try {
  await client.connect(transport, { timeout: 30000 });
  if (process.argv[2] === 'tools') console.log(JSON.stringify(await client.listTools()));
  else {
    const requests = JSON.parse(process.argv[2]);
    for (const entry of requests) {
      const { filter, ...request } = entry;
      if (request.arguments?.file_path) request.arguments.file_path = path.resolve(root, request.arguments.file_path);
      const result = await client.callTool(request, undefined, { timeout: 90000 });
      let output = result;
      if (request.name === 'get_diagnostics') {
        // The installed MCP cache returns [] on its five-second timeout. Require
        // a completed clangd check before describing diagnostics as verified.
        const executable = process.env.CLANGD_PATH || path.join(process.env.ProgramFiles || 'C:/Program Files', 'LLVM/bin/clangd.exe');
        const check = spawnSync(executable, ['--check=' + request.arguments.file_path, '--compile-commands-dir=' + root, '--tweaks=none', '--log=error'],
          { cwd: root, windowsHide: true, encoding: 'utf8', timeout: 60000 });
        output = {
          mcp_response: result,
          completed_clangd_check: {
            verified: check.status === 0 && !check.error,
            exit_code: check.status,
            error: check.error?.message,
            diagnostics: check.stderr || ''
          }
        };
      }
      if (request.name === 'get_document_symbols' && !result.isError) {
        const data = JSON.parse(result.content.find(item => item.type === 'text').text);
        const symbols = [];
        const visit = items => { for (const item of items ?? []) {
          if (!filter || new RegExp(filter).test(item.name)) {
            const { children, ...location } = item;
            if (symbols.length < MAX_ITEMS) {
              symbols.push(location);
            }
          }
          visit(item.children);
        } };
        visit(data.symbols);
        output = {
          found: data.found,
          count: data.count,
          returned: symbols.length,
          truncated: data.count > symbols.length,
          symbols
        };
      }
      console.log(JSON.stringify(compact(output)));
    }
  }
} finally {
  const pid = transport.pid;

  try {
    await client.close();
  } catch {
    // Fallback cleanup below.
  }

  if (pid) {
    spawnSync(
      'taskkill.exe',
      ['/PID', String(pid), '/T', '/F'],
      {
        windowsHide: true,
        stdio: 'ignore',
        timeout: 5000
      }
    );
  }
}
