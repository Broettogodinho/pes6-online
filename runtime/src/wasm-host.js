// Host WASM: instancia o módulo e liga as importações de I/O ao VFS.
//
// O problema central: o código do módulo é síncrono, mas o I/O no navegador
// é assíncrono (fetch). Duas estratégias, escolhidas em tempo de execução:
//
// 1. JSPI (JS Promise Integration, WebAssembly.Suspending/promising): a
//    importação "read" devolve uma Promise e o módulo fica suspenso até ela
//    resolver. É a solução definitiva, mas depende do runtime.
// 2. Pré-busca + nova tentativa (fallback): "read" é síncrona; se os blocos
//    não estão em memória, devolve EAGAIN e registra o trecho pendente. O
//    host busca o trecho e chama a exportação de novo. Só serve para
//    exportações idempotentes — é uma estratégia de PoC, não de produção.
//
// Importações expostas ao módulo (módulo "runtime"):
//   read(handle: i32, offset: i32, length: i32, dst: i32) -> i32
//     bytes lidos (>= 0), EAGAIN (-11) ou EBADF (-9)
//   size(handle: i32) -> i32

import { verifySha256 } from "./manifest.js";

export const EAGAIN = -11;
export const EBADF = -9;

export function jspiAvailable() {
  return typeof WebAssembly.Suspending === "function" && typeof WebAssembly.promising === "function";
}

export class WasmHost {
  /**
   * @param {import("./vfs.js").VirtualFS} vfs
   * @param {{ mode?: "auto" | "jspi" | "retry", maxRetries?: number }} [options]
   */
  constructor(vfs, options = {}) {
    this.vfs = vfs;
    const mode = options.mode ?? "auto";
    if (mode === "jspi" && !jspiAvailable()) throw new Error("JSPI indisponível neste runtime");
    this.mode = mode === "auto" ? (jspiAvailable() ? "jspi" : "retry") : mode;
    this.maxRetries = options.maxRetries ?? 64;
    this.instance = undefined;
    this.memory = undefined;
    /** @type {{ handle: number, offset: number, length: number } | undefined} */
    this.pending = undefined;
    this.stats = { calls: 0, retries: 0 };
  }

  /** Baixa, confere e instancia o módulo a partir de uma URL. */
  async loadFromUrl(url, { sha256, fetch: doFetch } = {}) {
    const f = doFetch ?? globalThis.fetch.bind(globalThis);
    const res = await f(String(url));
    if (!res.ok) throw new Error(`HTTP ${res.status} ao baixar ${url}`);
    if (!sha256 && res.headers.get("content-type")?.startsWith("application/wasm")) {
      // Sem hash para conferir: compila enquanto baixa.
      return this.instantiate(await WebAssembly.compileStreaming(res));
    }
    const bytes = new Uint8Array(await res.arrayBuffer());
    await verifySha256(bytes, sha256);
    return this.instantiate(await WebAssembly.compile(bytes));
  }

  /** @param {WebAssembly.Module} module */
  async instantiate(module, extraImports = {}) {
    const runtime = {
      size: (handle) => {
        const reader = this._reader(handle);
        return reader ? reader.totalSize : EBADF;
      },
      read: this.mode === "jspi"
        ? new WebAssembly.Suspending((handle, offset, length, dst) => this._readAsync(handle, offset, length, dst))
        : (handle, offset, length, dst) => this._readSync(handle, offset, length, dst),
    };
    this.instance = await WebAssembly.instantiate(module, { ...extraImports, runtime });
    this.memory = this.instance.exports.memory;
    return this.instance;
  }

  /** Chama uma exportação, resolvendo o I/O conforme o modo. */
  async call(name, ...args) {
    const fn = this.instance?.exports[name];
    if (typeof fn !== "function") throw new Error(`exportação inexistente: ${name}`);
    this.stats.calls++;
    if (this.mode === "jspi") return WebAssembly.promising(fn)(...args);
    for (let attempt = 0; ; attempt++) {
      this.pending = undefined;
      const result = fn(...args);
      if (!this.pending) return result;
      if (attempt >= this.maxRetries) throw new Error(`I/O não resolvido após ${attempt} tentativas`);
      const { handle, offset, length } = this.pending;
      await this.vfs.readerFor(handle).prefetch(offset, length);
      this.stats.retries++;
    }
  }

  _reader(handle) {
    try {
      return this.vfs.readerFor(handle);
    } catch {
      return undefined;
    }
  }

  _readSync(handle, offset, length, dst) {
    const reader = this._reader(handle);
    if (!reader) return EBADF;
    const bytes = reader.readSync(offset, length);
    if (bytes === null) {
      this.pending ??= { handle, offset, length };
      return EAGAIN;
    }
    new Uint8Array(this.memory.buffer, dst, bytes.length).set(bytes);
    return bytes.length;
  }

  async _readAsync(handle, offset, length, dst) {
    const reader = this._reader(handle);
    if (!reader) return EBADF;
    const bytes = await reader.read(offset, length);
    // Relê o buffer depois do await: a memória pode ter crescido.
    new Uint8Array(this.memory.buffer, dst, bytes.length).set(bytes);
    return bytes.length;
  }
}
