// Servidor HTTP mínimo com suporte a HEAD e Range (206), para testes e demo.
// Serve arquivos em memória (Map caminho -> bytes) e, opcionalmente, uma pasta.

import { createServer } from "node:http";
import { readFile } from "node:fs/promises";
import { extname, join, normalize } from "node:path";

const TYPES = {
  ".wasm": "application/wasm",
  ".json": "application/json",
  ".html": "text/html; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".mjs": "text/javascript; charset=utf-8",
  ".bin": "application/octet-stream",
};

/**
 * @param {{
 *   files?: Map<string, Uint8Array>,
 *   dirs?: Record<string, string>,
 *   rangeSupport?: boolean,
 * }} options
 */
export function startStaticServer({ files = new Map(), dirs = {}, rangeSupport = true } = {}) {
  /** registro das requisições, para os testes conferirem agrupamento/cache */
  const log = [];
  const server = createServer(async (req, res) => {
    const path = decodeURIComponent(new URL(req.url, "http://x").pathname);
    log.push({ method: req.method, path, range: req.headers.range });
    const body = files.get(path) ?? (await fromDirs(dirs, path));
    if (!body) {
      res.writeHead(404).end();
      return;
    }
    const headers = {
      "Content-Type": TYPES[extname(path)] ?? "application/octet-stream",
      "Accept-Ranges": rangeSupport ? "bytes" : "none",
      "Cache-Control": "no-store",
    };
    const range = rangeSupport ? /^bytes=(\d+)-(\d*)$/.exec(req.headers.range ?? "") : null;
    if (range) {
      const start = Number(range[1]);
      const end = Math.min(range[2] ? Number(range[2]) : body.length - 1, body.length - 1);
      if (start > end) {
        res.writeHead(416, { "Content-Range": `bytes */${body.length}` }).end();
        return;
      }
      res.writeHead(206, {
        ...headers,
        "Content-Length": end - start + 1,
        "Content-Range": `bytes ${start}-${end}/${body.length}`,
      });
      res.end(req.method === "HEAD" ? undefined : body.subarray(start, end + 1));
      return;
    }
    res.writeHead(200, { ...headers, "Content-Length": body.length });
    res.end(req.method === "HEAD" ? undefined : body);
  });
  return new Promise((resolve) => {
    server.listen(0, "127.0.0.1", () => {
      const { port } = server.address();
      resolve({
        url: `http://127.0.0.1:${port}`,
        log,
        close: () => new Promise((r) => server.close(r)),
        server,
      });
    });
  });
}

async function fromDirs(dirs, path) {
  for (const [prefix, dir] of Object.entries(dirs)) {
    if (!path.startsWith(prefix)) continue;
    const rel = normalize(path.slice(prefix.length) || ".").replace(/^([/\\])+/, "");
    if (rel.startsWith("..")) return undefined;
    try {
      return new Uint8Array(await readFile(join(dir, rel === "." ? "index.html" : rel)));
    } catch {
      return undefined;
    }
  }
  return undefined;
}
