// Sistema de arquivos virtual: mapeia caminhos para leitores em blocos.
// O módulo WASM enxerga arquivos por handle (inteiro); o VFS resolve
// handle -> ChunkedReader e faz a leitura assíncrona ou síncrona.

import { ChunkedReader } from "./chunk-cache.js";

export class VfsError extends Error {}

export class VirtualFS {
  /** @param {ConstructorParameters<typeof ChunkedReader>[1]} [readerOptions] */
  constructor(readerOptions = {}) {
    this.readerOptions = readerOptions;
    /** @type {Map<string, ChunkedReader>} */
    this.files = new Map();
    /** @type {Map<number, string>} */
    this.handles = new Map();
    this.nextHandle = 3; // 0-2 reservados (stdin/stdout/stderr)
  }

  /**
   * Monta uma fonte num caminho virtual.
   * @param {string} path
   * @param {Parameters<typeof ChunkedReader>[0]} source
   */
  mount(path, source) {
    const p = normalize(path);
    this.files.set(p, new ChunkedReader(source, { ...this.readerOptions, storeKey: p }));
    return p;
  }

  list() {
    return [...this.files.keys()];
  }

  async stat(path) {
    const reader = this._reader(path);
    return { path: normalize(path), size: await reader.size() };
  }

  /** Abre um arquivo e devolve o handle numérico usado pelo módulo WASM. */
  async open(path) {
    const p = normalize(path);
    await this._reader(p).size(); // carrega o tamanho para o readSync funcionar
    const handle = this.nextHandle++;
    this.handles.set(handle, p);
    return handle;
  }

  close(handle) {
    this.handles.delete(handle);
  }

  async read(path, offset, length) {
    return this._reader(path).read(offset, length);
  }

  readerFor(handle) {
    const path = this.handles.get(handle);
    if (path === undefined) throw new VfsError(`handle inválido: ${handle}`);
    return this.files.get(path);
  }

  _reader(path) {
    const reader = this.files.get(normalize(path));
    if (!reader) throw new VfsError(`arquivo não montado: ${path}`);
    return reader;
  }
}

export function normalize(path) {
  const parts = [];
  for (const part of String(path).replaceAll("\\", "/").split("/")) {
    if (!part || part === ".") continue;
    if (part === "..") parts.pop();
    else parts.push(part);
  }
  return "/" + parts.join("/");
}
