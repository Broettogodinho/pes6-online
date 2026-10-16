// Leitor em blocos com cache LRU sobre qualquer fonte (ver sources.js).
//
// - Divide o arquivo em blocos fixos (padrão 256 KiB).
// - read() busca só os blocos que faltam e agrupa blocos vizinhos numa única
//   requisição (uma Range por trecho contíguo).
// - Leituras simultâneas do mesmo bloco compartilham a mesma requisição.
// - readSync() devolve os bytes só se todos os blocos já estiverem em memória;
//   é o que o host WASM usa dentro de chamadas síncronas.
// - Um store persistente opcional (ex.: OPFS) guarda blocos entre sessões.

export const DEFAULT_CHUNK_SIZE = 256 * 1024;

export class ChunkedReader {
  /**
   * @param {{ size(): Promise<number>, read(o: number, l: number): Promise<Uint8Array> }} source
   * @param {{
   *   chunkSize?: number,
   *   maxBytes?: number,
   *   store?: { get(key: string): Promise<Uint8Array | undefined>, put(key: string, bytes: Uint8Array): Promise<void> },
   *   storeKey?: string,
   * }} [options]
   */
  constructor(source, options = {}) {
    this.source = source;
    this.chunkSize = options.chunkSize ?? DEFAULT_CHUNK_SIZE;
    this.maxBytes = options.maxBytes ?? 64 * 1024 * 1024;
    this.store = options.store;
    this.storeKey = options.storeKey ?? "default";
    /** @type {Map<number, Uint8Array>} índice do bloco -> bytes (ordem = LRU) */
    this.chunks = new Map();
    /** @type {Map<number, Promise<void>>} blocos sendo buscados */
    this.inflight = new Map();
    this.residentBytes = 0;
    this.totalSize = undefined;
    this.stats = { requests: 0, bytesFetched: 0, hits: 0, storeHits: 0 };
  }

  async size() {
    this.totalSize ??= await this.source.size();
    return this.totalSize;
  }

  /** Lê [offset, offset+length), limitado ao fim do arquivo. */
  async read(offset, length) {
    const [first, last, len] = await this._span(offset, length);
    if (len === 0) return new Uint8Array(0);
    await this._ensure(first, last);
    return this._copy(offset, len);
  }

  /** Igual a read(), mas síncrono: devolve null se algum bloco não estiver em memória. */
  readSync(offset, length) {
    if (this.totalSize === undefined) return null;
    const len = Math.max(0, Math.min(length, this.totalSize - offset));
    if (len === 0) return new Uint8Array(0);
    const first = Math.floor(offset / this.chunkSize);
    const last = Math.floor((offset + len - 1) / this.chunkSize);
    for (let i = first; i <= last; i++) if (!this.chunks.has(i)) return null;
    return this._copy(offset, len);
  }

  /** Garante em memória os blocos de [offset, offset+length) sem copiar os bytes. */
  async prefetch(offset, length) {
    const [first, last, len] = await this._span(offset, length);
    if (len > 0) await this._ensure(first, last);
  }

  async _span(offset, length) {
    const total = await this.size();
    if (offset < 0 || length < 0) throw new RangeError("offset/length negativos");
    const len = Math.max(0, Math.min(length, total - offset));
    if (len === 0) return [0, -1, 0];
    return [Math.floor(offset / this.chunkSize), Math.floor((offset + len - 1) / this.chunkSize), len];
  }

  async _ensure(first, last) {
    const waits = [];
    let runStart = -1;
    const flush = (end) => {
      if (runStart < 0) return;
      const start = runStart;
      const p = this._fetchRun(start, end);
      for (let i = start; i <= end; i++) this.inflight.set(i, p);
      p.finally(() => {
        for (let i = start; i <= end; i++) if (this.inflight.get(i) === p) this.inflight.delete(i);
      });
      waits.push(p);
      runStart = -1;
    };
    for (let i = first; i <= last; i++) {
      if (this.chunks.has(i)) {
        this._touch(i);
        this.stats.hits++;
        flush(i - 1);
      } else if (this.inflight.has(i)) {
        flush(i - 1);
        waits.push(this.inflight.get(i));
      } else if (runStart < 0) {
        runStart = i;
      }
    }
    flush(last);
    await Promise.all(waits);
  }

  async _fetchRun(first, last) {
    // Tenta o store persistente antes da rede, bloco a bloco.
    let missingFrom = first;
    if (this.store) {
      for (let i = first; i <= last; i++) {
        const cached = await this.store.get(this._key(i));
        if (!cached) break;
        this._put(i, cached);
        this.stats.storeHits++;
        missingFrom = i + 1;
      }
    }
    if (missingFrom > last) return;
    const offset = missingFrom * this.chunkSize;
    const end = Math.min((last + 1) * this.chunkSize, this.totalSize);
    const bytes = await this.source.read(offset, end - offset);
    this.stats.requests++;
    this.stats.bytesFetched += bytes.length;
    for (let i = missingFrom; i <= last; i++) {
      const start = (i - missingFrom) * this.chunkSize;
      const chunk = bytes.slice(start, start + this.chunkSize);
      this._put(i, chunk);
      if (this.store) await this.store.put(this._key(i), chunk);
    }
  }

  _key(index) {
    return `${this.storeKey}:${this.chunkSize}:${index}`;
  }

  _touch(index) {
    const chunk = this.chunks.get(index);
    this.chunks.delete(index);
    this.chunks.set(index, chunk);
  }

  _put(index, chunk) {
    if (this.chunks.has(index)) this.residentBytes -= this.chunks.get(index).length;
    this.chunks.delete(index);
    this.chunks.set(index, chunk);
    this.residentBytes += chunk.length;
    // Despeja os blocos menos usados; nunca o que acabou de entrar.
    for (const [i, c] of this.chunks) {
      if (this.residentBytes <= this.maxBytes || i === index) break;
      this.chunks.delete(i);
      this.residentBytes -= c.length;
    }
  }

  _copy(offset, len) {
    const out = new Uint8Array(len);
    let written = 0;
    while (written < len) {
      const pos = offset + written;
      const index = Math.floor(pos / this.chunkSize);
      const chunk = this.chunks.get(index);
      if (!chunk) throw new Error(`bloco ${index} despejado durante a cópia; aumente maxBytes`);
      const from = pos - index * this.chunkSize;
      const n = Math.min(chunk.length - from, len - written);
      out.set(chunk.subarray(from, from + n), written);
      written += n;
    }
    return out;
  }
}
