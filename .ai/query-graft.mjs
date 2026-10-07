import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { spawnSync } from 'node:child_process';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const sdk = path.join(root, '.tools/clangd-mcp-server/node_modules/@modelcontextprotocol/sdk/dist/esm');
const { Client } = await import(pathToFileURL(path.join(sdk, 'client/index.js')));
const { StdioClientTransport } = await import(pathToFileURL(path.join(sdk, 'client/stdio.js')));
const npm = spawnSync(process.env.ComSpec || 'cmd.exe', ['/d', '/s', '/c', 'npm.cmd root -g'], { encoding: 'utf8', windowsHide: true });
if (npm.status !== 0) throw new Error('Cannot locate installed Graft');
const entry = path.join(npm.stdout.trim(), '@nanonets/graft/dist/cli.js');
const transport = new StdioClientTransport({ command: process.execPath, args: [entry, 'mcp', root], cwd: root, stderr: 'pipe' });
const client = new Client({ name: 'epidemic-local-navigation', version: '1.0.0' });
try {
    await client.connect(transport, { timeout: 60000 });
    if (process.argv[2] === 'tools') console.log(JSON.stringify(await client.listTools()));
    else for (const request of JSON.parse(process.argv[2])) {
        const result = await client.callTool(request, undefined, { timeout: 120000 });
        // Graph orientation must not flood the model with complete function bodies.
        for (const content of result.content ?? []) if (content.type === 'text') {
            content.text = content.text.replace(/^\[graft\] tokens saved.*\n/gm, '');
            if (content.text.length > 4500) { content.text = content.text.slice(0, 4500) + '\n[bounded output truncated; use clangd for the known symbol]'; result.output_truncated = true; }
        }
        console.log(JSON.stringify({ request, result }));
    }
} finally {
    if (transport.pid) spawnSync('taskkill.exe', ['/PID', String(transport.pid), '/T', '/F'], { windowsHide: true, stdio: 'ignore' });
    await client.close();
}
