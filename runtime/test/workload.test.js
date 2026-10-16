import { test, after, before } from "node:test";
import assert from "node:assert/strict";
import { createHash } from "node:crypto";

import { VirtualFS, normalize } from "../src/vfs.js";
import { MemorySource } from "../src/sources.js";
import { parseManifest, ManifestError } from "../src/manifest.js";
import { WasmHost, jspiAvailable, EBADF } from "../src/wasm-host.js";
import { loadWorkload } from "../src/index.js";
import { buildDummyApp, buildDataBundle, byteSum } from "../tools/dummy-app.js";
import { startStaticServer } from "../tools/static-server.js";

const app = buildDummyApp();
const data = buildDataBundle(200_000);
const appSha = createHash("sha256").update(app).digest("hex");
let srv;

function manifest(extra = {}) {
  return new TextEncoder().encode(JSON.stringify({
    version: 1,
    app: { url: "dummy_app.wasm", sha256: appSha, ...extra },
    files: [{ path: "/data/bundle.bin", url: "data_bundle.bin", size: data.length }],
  }));
}

before(async () => {
  srv = await startStaticServer({
    files: new Map([
      ["/w/manifest.json", manifest()],
      ["/w/manifest-bad-hash.json", manifest({ sha256: "00".repeat(32) })],
      ["/w/dummy_app.wasm", app],
      ["/w/data_bundle.bin", data],
    ]),
  });
});
after(() => srv.close());

test("o módulo de teste é WASM válido", () => {
  assert.ok(WebAssembly.validate(app));
});

test("normalize resolve caminhos virtuais", () => {
  assert.equal(normalize("data\\x/../bundle.bin"), "/data/bundle.bin");
  assert.equal(normalize("/a/./b//c"), "/a/b/c");
});

test("parseManifest resolve URLs relativas e valida campos", () => {
  const m = parseManifest(
    { version: 1, app: { url: "a.wasm" }, files: [{ path: "/d", url: "d.bin" }] },
    "https://cdn.example/w/manifest.json",
  );
  assert.equal(m.app.url, "https://cdn.example/w/a.wasm");
  assert.equal(m.files[0].url, "https://cdn.example/w/d.bin");
  assert.throws(() => parseManifest({ version: 2 }, "https://x/"), ManifestError);
  assert.throws(() => parseManifest({ version: 1, app: { url: "a" }, files: [{}] }, "https://x/"), ManifestError);
});

test("modo retry: lê do VFS remoto via pré-busca + nova tentativa", async () => {
  const { vfs, host } = await loadWorkload(`${srv.url}/w/manifest.json`, {
    mode: "retry",
    reader: { chunkSize: 16_384 },
  });
  const handle = await vfs.open("/data/bundle.bin");
  const len = 100_000;
  assert.equal(await host.call("checksum", handle, len), byteSum(data.subarray(0, len)));
  assert.equal(host.stats.retries, 1); // 1ª tentativa sem dados, 2ª com os blocos em memória
  assert.equal(await host.call("checksum", handle, len), byteSum(data.subarray(0, len)));
  assert.equal(host.stats.retries, 1); // agora tudo vem do cache
});

test("SHA-256 errado no manifesto bloqueia o carregamento", async () => {
  await assert.rejects(loadWorkload(`${srv.url}/w/manifest-bad-hash.json`, { mode: "retry" }), /SHA-256/);
});

test("handle inválido devolve EBADF ao módulo (checksum = -1)", async () => {
  const host = new WasmHost(new VirtualFS(), { mode: "retry" });
  await host.instantiate(await WebAssembly.compile(app));
  assert.equal(await host.call("checksum", 42, 10), -1);
  assert.equal(EBADF, -9);
});

test("modo retry com fonte local (BlobSource/MemorySource) usa a mesma ponte", async () => {
  const vfs = new VirtualFS({ chunkSize: 4096 });
  vfs.mount("/data/bundle.bin", new MemorySource(data));
  const host = new WasmHost(vfs, { mode: "retry" });
  await host.instantiate(await WebAssembly.compile(app));
  const handle = await vfs.open("/data/bundle.bin");
  assert.equal(await host.call("checksum", handle, 5000), byteSum(data.subarray(0, 5000)));
});

test("modo JSPI: o módulo suspende durante o fetch", { skip: !jspiAvailable() && "JSPI indisponível neste Node" }, async () => {
  const { vfs, host } = await loadWorkload(`${srv.url}/w/manifest.json`, { mode: "jspi" });
  const handle = await vfs.open("/data/bundle.bin");
  assert.equal(await host.call("checksum", handle, 100_000), byteSum(data.subarray(0, 100_000)));
  assert.equal(host.stats.retries, 0);
});
