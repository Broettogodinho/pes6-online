import { loadWorkload, initWebGPU, webgpuAvailable, OpfsChunkStore } from "/src/index.js";

const out = document.getElementById("out");
const log = (msg) => (out.textContent += `\n${msg}`);

let gpu;
if (webgpuAvailable()) {
  try {
    gpu = await initWebGPU(document.getElementById("gpu"));
    gpu.clear(0.1, 0.1, 0.15);
  } catch (err) {
    log(`WebGPU: ${err.message}`);
  }
} else {
  log("WebGPU indisponível neste navegador (o carregador funciona mesmo assim)");
}

let store;
try {
  store = await OpfsChunkStore.open("runtime-demo");
} catch {
  log("OPFS indisponível: cache só em memória");
}

document.getElementById("run").addEventListener("click", async () => {
  out.textContent = "carregando…";
  const t0 = performance.now();
  try {
    const { vfs, host } = await loadWorkload("/workload/manifest.json", { reader: { store } });
    const handle = await vfs.open("/data/bundle.bin");
    const len = Number(document.getElementById("len").value);
    const sum = await host.call("checksum", handle, len);
    const reader = vfs.readerFor(handle);
    log(`modo de I/O: ${host.mode}`);
    log(`checksum(${len}) = ${sum}`);
    log(`requisições: ${reader.stats.requests} · bytes baixados: ${reader.stats.bytesFetched} · blocos do OPFS: ${reader.stats.storeHits}`);
    log(`tempo: ${(performance.now() - t0).toFixed(1)} ms`);
    gpu?.clear(0.1, 0.45, 0.25);
  } catch (err) {
    log(`erro: ${err.message}`);
    gpu?.clear(0.6, 0.1, 0.1);
  }
});
