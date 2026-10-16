// Fontes de dados: de onde o runtime lê bytes. Toda fonte expõe
//   size(): Promise<number>
//   read(offset, length): Promise<Uint8Array>
// O resto do runtime (cache, VFS, host) só conhece essa interface.

export class SourceError extends Error {}

/** Lê de uma URL remota (CDN ou servidor local) com requisições HTTP Range. */
export class HttpRangeSource {
  /**
   * @param {string | URL} url
   * @param {{ fetch?: typeof fetch, headers?: Record<string, string> }} [options]
   */
  constructor(url, options = {}) {
    this.url = String(url);
    this.fetch = options.fetch ?? globalThis.fetch.bind(globalThis);
    this.headers = options.headers ?? {};
    this._size = undefined;
    /** true quando o servidor ignorou Range e devolveu o arquivo inteiro */
    this.rangeUnsupported = false;
  }

  async size() {
    if (this._size !== undefined) return this._size;
    const head = await this.fetch(this.url, { method: "HEAD", headers: this.headers });
    const len = head.ok ? Number(head.headers.get("content-length")) : NaN;
    if (Number.isFinite(len) && len >= 0) return (this._size = len);
    // Alguns servidores/CDNs não respondem HEAD: pede 1 byte e lê o total do Content-Range.
    const probe = await this.fetch(this.url, { headers: { ...this.headers, Range: "bytes=0-0" } });
    const total = parseContentRangeTotal(probe.headers.get("content-range"));
    await probe.body?.cancel();
    if (total === undefined) throw new SourceError(`tamanho desconhecido: ${this.url}`);
    return (this._size = total);
  }

  async read(offset, length) {
    if (length === 0) return new Uint8Array(0);
    const end = offset + length - 1;
    const res = await this.fetch(this.url, {
      headers: { ...this.headers, Range: `bytes=${offset}-${end}` },
    });
    if (res.status === 206) {
      const bytes = new Uint8Array(await res.arrayBuffer());
      if (bytes.length !== length) {
        throw new SourceError(`resposta parcial com ${bytes.length} bytes, esperado ${length}: ${this.url}`);
      }
      return bytes;
    }
    if (res.status === 200) {
      // Servidor sem suporte a Range: funciona, mas baixa tudo a cada leitura.
      this.rangeUnsupported = true;
      const all = new Uint8Array(await res.arrayBuffer());
      this._size ??= all.length;
      return all.slice(offset, offset + length);
    }
    throw new SourceError(`HTTP ${res.status} em ${this.url} (bytes ${offset}-${end})`);
  }
}

/** Lê de um Blob/File (ex.: arquivo escolhido pelo usuário na tela de seleção). */
export class BlobSource {
  /** @param {Blob} blob */
  constructor(blob) {
    this.blob = blob;
  }

  async size() {
    return this.blob.size;
  }

  async read(offset, length) {
    return new Uint8Array(await this.blob.slice(offset, offset + length).arrayBuffer());
  }
}

/** Fonte em memória, para testes e dados gerados. */
export class MemorySource {
  /** @param {Uint8Array} bytes */
  constructor(bytes) {
    this.bytes = bytes;
  }

  async size() {
    return this.bytes.length;
  }

  async read(offset, length) {
    return this.bytes.slice(offset, offset + length);
  }
}

/** "bytes 0-0/12345" -> 12345 */
export function parseContentRangeTotal(header) {
  const m = /\/(\d+)\s*$/.exec(header ?? "");
  return m ? Number(m[1]) : undefined;
}
