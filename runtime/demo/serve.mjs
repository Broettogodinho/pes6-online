// Demo local: serve o runtime e uma carga de trabalho de teste gerada em memória.
// Uso: npm run demo  ->  abrir a URL impressa no navegador.

import { createHash } from "node:crypto";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

import { buildDummyApp, buildDataBundle } from "../tools/dummy-app.js";
import { startStaticServer } from "../tools/static-server.js";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const app = buildDummyApp();
const data = buildDataBundle(4 * 1024 * 1024);
const manifest = {
  version: 1,
  app: { url: "dummy_app.wasm", sha256: createHash("sha256").update(app).digest("hex") },
  files: [{ path: "/data/bundle.bin", url: "data_bundle.bin", size: data.length }],
};

const srv = await startStaticServer({
  files: new Map([
    ["/workload/manifest.json", new TextEncoder().encode(JSON.stringify(manifest, null, 2))],
    ["/workload/dummy_app.wasm", app],
    ["/workload/data_bundle.bin", data],
  ]),
  dirs: { "/src/": join(root, "src"), "/": join(root, "demo") },
});
console.log(`Demo do runtime em ${srv.url}/`);
