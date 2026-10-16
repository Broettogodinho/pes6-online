// Store persistente de blocos no Origin Private File System (OPFS) do navegador.
// Usado pelo ChunkedReader para não baixar de novo o que já veio em outra sessão.
// Só existe no navegador; em Node, use o ChunkedReader sem store.

export class OpfsChunkStore {
  /** @param {FileSystemDirectoryHandle} dir */
  constructor(dir) {
    this.dir = dir;
  }

  /** Abre (ou cria) a pasta do cache dentro do OPFS. */
  static async open(name = "runtime-cache") {
    if (!globalThis.navigator?.storage?.getDirectory) {
      throw new Error("OPFS indisponível neste ambiente");
    }
    const root = await navigator.storage.getDirectory();
    return new OpfsChunkStore(await root.getDirectoryHandle(name, { create: true }));
  }

  async get(key) {
    try {
      const handle = await this.dir.getFileHandle(fileName(key));
      return new Uint8Array(await (await handle.getFile()).arrayBuffer());
    } catch (err) {
      if (err?.name === "NotFoundError") return undefined;
      throw err;
    }
  }

  async put(key, bytes) {
    const handle = await this.dir.getFileHandle(fileName(key), { create: true });
    const writable = await handle.createWritable();
    await writable.write(bytes);
    await writable.close();
  }
}

function fileName(key) {
  return encodeURIComponent(key);
}
