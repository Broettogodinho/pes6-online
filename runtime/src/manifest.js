// Manifesto de uma carga de trabalho: qual módulo WASM rodar e quais
// pacotes de dados montar no VFS. Formato (JSON):
//
// {
//   "version": 1,
//   "app":   { "url": "dummy_app.wasm", "sha256": "<hex, opcional>" },
//   "files": [ { "path": "/data/bundle.bin", "url": "data_bundle.bin", "size": 1048576 } ]
// }
//
// URLs relativas são resolvidas a partir da URL do manifesto.

import { HttpRangeSource } from "./sources.js";

export class ManifestError extends Error {}

/**
 * @param {string | URL} manifestUrl
 * @param {{ fetch?: typeof fetch }} [options]
 */
export async function fetchManifest(manifestUrl, options = {}) {
  const doFetch = options.fetch ?? globalThis.fetch.bind(globalThis);
  const res = await doFetch(String(manifestUrl));
  if (!res.ok) throw new ManifestError(`HTTP ${res.status} ao baixar o manifesto ${manifestUrl}`);
  return parseManifest(await res.json(), manifestUrl);
}

export function parseManifest(json, baseUrl) {
  if (json?.version !== 1) throw new ManifestError("versão de manifesto não suportada");
  if (typeof json.app?.url !== "string") throw new ManifestError("manifesto sem app.url");
  const files = Array.isArray(json.files) ? json.files : [];
  for (const f of files) {
    if (typeof f.path !== "string" || typeof f.url !== "string") {
      throw new ManifestError("cada arquivo precisa de path e url");
    }
  }
  return {
    app: { url: new URL(json.app.url, baseUrl).href, sha256: json.app.sha256 },
    files: files.map((f) => ({ path: f.path, url: new URL(f.url, baseUrl).href, size: f.size })),
  };
}

/** Monta os arquivos do manifesto no VFS como fontes HTTP Range. */
export function mountManifestFiles(vfs, manifest, options = {}) {
  for (const f of manifest.files) {
    const source = new HttpRangeSource(f.url, options);
    if (Number.isFinite(f.size)) source._size = f.size; // evita um HEAD por arquivo
    vfs.mount(f.path, source);
  }
}

/** Confere o SHA-256 (hex) de bytes já baixados. */
export async function verifySha256(bytes, expectedHex) {
  if (!expectedHex) return;
  const digest = new Uint8Array(await crypto.subtle.digest("SHA-256", bytes));
  const hex = [...digest].map((b) => b.toString(16).padStart(2, "0")).join("");
  if (hex !== expectedHex.toLowerCase()) {
    throw new ManifestError(`SHA-256 não confere: esperado ${expectedHex}, obtido ${hex}`);
  }
}
