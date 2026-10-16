import { test, after, before } from "node:test";
import assert from "node:assert/strict";

import { HttpRangeSource, MemorySource, parseContentRangeTotal } from "../src/sources.js";
import { ChunkedReader } from "../src/chunk-cache.js";
import { buildDataBundle } from "../tools/dummy-app.js";
import { startStaticServer } from "../tools/static-server.js";

const SIZE = 300_000;
const data = buildDataBundle(SIZE);
let srv;
let noRange;

before(async () => {
  srv = await startStaticServer({ files: new Map([["/data_bundle.bin", data]]) });
  noRange = await startStaticServer({ files: new Map([["/data_bundle.bin", data]]), rangeSupport: false });
});
after(async () => {
  await srv.close();
  await noRange.close();
});

test("HttpRangeSource lê o tamanho e trechos com 206", async () => {
  const src = new HttpRangeSource(`${srv.url}/data_bundle.bin`);
  assert.equal(await src.size(), SIZE);
  assert.deepEqual(await src.read(1000, 50), data.slice(1000, 1050));
  assert.equal(src.rangeUnsupported, false);
});

test("HttpRangeSource funciona com servidor sem Range (200) e sinaliza", async () => {
  const src = new HttpRangeSource(`${noRange.url}/data_bundle.bin`);
  assert.deepEqual(await src.read(10, 20), data.slice(10, 30));
  assert.equal(src.rangeUnsupported, true);
});

test("HttpRangeSource falha com 404", async () => {
  const src = new HttpRangeSource(`${srv.url}/nao-existe.bin`);
  await assert.rejects(src.read(0, 10), /HTTP 404/);
});

test("parseContentRangeTotal", () => {
  assert.equal(parseContentRangeTotal("bytes 0-0/12345"), 12345);
  assert.equal(parseContentRangeTotal("bytes */99"), 99);
  assert.equal(parseContentRangeTotal(null), undefined);
});

test("ChunkedReader devolve os bytes certos atravessando blocos", async () => {
  const reader = new ChunkedReader(new MemorySource(data), { chunkSize: 4096 });
  for (const [o, l] of [[0, 10], [4090, 20], [8191, 1], [123_456, 50_000], [SIZE - 5, 100]]) {
    assert.deepEqual(await reader.read(o, l), data.slice(o, Math.min(o + l, SIZE)), `trecho ${o}+${l}`);
  }
});

test("ChunkedReader agrupa blocos contíguos numa única requisição Range", async () => {
  const src = new HttpRangeSource(`${srv.url}/data_bundle.bin`);
  const reader = new ChunkedReader(src, { chunkSize: 4096 });
  await reader.size();
  srv.log.length = 0;
  await reader.read(0, 4096 * 5); // 5 blocos
  const gets = srv.log.filter((r) => r.method === "GET");
  assert.equal(gets.length, 1);
  assert.equal(gets[0].range, `bytes=0-${4096 * 5 - 1}`);
});

test("ChunkedReader não busca de novo o que está em cache e só busca o que falta", async () => {
  const src = new HttpRangeSource(`${srv.url}/data_bundle.bin`);
  const reader = new ChunkedReader(src, { chunkSize: 4096 });
  await reader.read(4096, 4096); // bloco 1
  srv.log.length = 0;
  await reader.read(0, 4096 * 3); // blocos 0, 1 (cache) e 2
  // as duas buscas saem em paralelo: a ordem de chegada no servidor varia
  const ranges = srv.log.filter((r) => r.method === "GET").map((r) => r.range).sort();
  assert.deepEqual(ranges, ["bytes=0-4095", "bytes=8192-12287"]);
  assert.ok(reader.stats.hits >= 1);
});

test("leituras simultâneas do mesmo bloco compartilham a requisição", async () => {
  const src = new HttpRangeSource(`${srv.url}/data_bundle.bin`);
  const reader = new ChunkedReader(src, { chunkSize: 4096 });
  await reader.size();
  srv.log.length = 0;
  const [a, b] = await Promise.all([reader.read(100, 10), reader.read(200, 10)]);
  assert.deepEqual(a, data.slice(100, 110));
  assert.deepEqual(b, data.slice(200, 210));
  assert.equal(srv.log.filter((r) => r.method === "GET").length, 1);
});

test("readSync devolve null até o bloco estar em memória", async () => {
  const reader = new ChunkedReader(new MemorySource(data), { chunkSize: 4096 });
  await reader.size();
  assert.equal(reader.readSync(5000, 10), null);
  await reader.prefetch(5000, 10);
  assert.deepEqual(reader.readSync(5000, 10), data.slice(5000, 5010));
});

test("LRU respeita maxBytes despejando os blocos menos usados", async () => {
  const reader = new ChunkedReader(new MemorySource(data), { chunkSize: 4096, maxBytes: 4096 * 3 });
  for (let i = 0; i < 10; i++) await reader.read(i * 4096, 4096);
  assert.ok(reader.residentBytes <= 4096 * 3);
  assert.deepEqual([...reader.chunks.keys()], [7, 8, 9]);
});

test("store persistente evita nova ida à rede", async () => {
  const mem = new Map();
  const store = { get: async (k) => mem.get(k), put: async (k, v) => void mem.set(k, v) };
  const src = new HttpRangeSource(`${srv.url}/data_bundle.bin`);
  await new ChunkedReader(src, { chunkSize: 4096, store }).read(0, 8192);
  srv.log.length = 0;
  const second = new ChunkedReader(src, { chunkSize: 4096, store });
  assert.deepEqual(await second.read(0, 8192), data.slice(0, 8192));
  assert.equal(srv.log.filter((r) => r.method === "GET").length, 0);
  assert.equal(second.stats.storeHits, 2);
});
