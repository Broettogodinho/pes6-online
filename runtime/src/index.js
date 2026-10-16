// Ponto de entrada do runtime.

export { HttpRangeSource, BlobSource, MemorySource, SourceError } from "./sources.js";
export { ChunkedReader, DEFAULT_CHUNK_SIZE } from "./chunk-cache.js";
export { OpfsChunkStore } from "./opfs-store.js";
export { VirtualFS, VfsError, normalize } from "./vfs.js";
export { fetchManifest, parseManifest, mountManifestFiles, verifySha256, ManifestError } from "./manifest.js";
export { WasmHost, EAGAIN, EBADF, jspiAvailable } from "./wasm-host.js";
export { initWebGPU, webgpuAvailable } from "./gpu.js";

import { fetchManifest, mountManifestFiles } from "./manifest.js";
import { VirtualFS } from "./vfs.js";
import { WasmHost } from "./wasm-host.js";

/**
 * Carrega uma carga de trabalho completa a partir do manifesto:
 * monta os pacotes de dados no VFS e instancia o módulo WASM.
 *
 * @param {string | URL} manifestUrl
 * @param {{
 *   fetch?: typeof fetch,
 *   mode?: "auto" | "jspi" | "retry",
 *   reader?: ConstructorParameters<typeof VirtualFS>[0],
 * }} [options]
 */
export async function loadWorkload(manifestUrl, options = {}) {
  const manifest = await fetchManifest(manifestUrl, options);
  const vfs = new VirtualFS(options.reader);
  mountManifestFiles(vfs, manifest, options);
  const host = new WasmHost(vfs, { mode: options.mode });
  await host.loadFromUrl(manifest.app.url, { sha256: manifest.app.sha256, fetch: options.fetch });
  return { manifest, vfs, host };
}
