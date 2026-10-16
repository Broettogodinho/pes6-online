// Gera, em memória, as cargas de trabalho de teste. Nada aqui vai para o
// repositório como binário: o módulo WASM é montado byte a byte.
//
// dummy_app.wasm importa runtime.read e exporta:
//   memory
//   checksum(handle: i32, length: i32) -> i32
//     lê `length` bytes do início do arquivo para a memória (endereço 0)
//     e devolve a soma dos bytes; devolve -1 se a leitura falhar.

const I32 = 0x7f;

function leb(n) {
  const out = [];
  do {
    let byte = n & 0x7f;
    n >>>= 7;
    if (n !== 0) byte |= 0x80;
    out.push(byte);
  } while (n !== 0);
  return out;
}

function str(s) {
  const bytes = [...new TextEncoder().encode(s)];
  return [...leb(bytes.length), ...bytes];
}

function section(id, body) {
  return [id, ...leb(body.length), ...body];
}

function vec(items) {
  return [...leb(items.length), ...items.flat()];
}

export function buildDummyApp() {
  const types = vec([
    [0x60, 4, I32, I32, I32, I32, 1, I32], // t0: read(handle, offset, length, dst) -> i32
    [0x60, 2, I32, I32, 1, I32], //           t1: checksum(handle, length) -> i32
  ]);
  const imports = vec([[...str("runtime"), ...str("read"), 0x00, 0]]);
  const funcs = vec([[1]]);
  const memory = vec([[0x00, 16]]); // 16 páginas = 1 MiB
  const exports = vec([
    [...str("memory"), 0x02, 0],
    [...str("checksum"), 0x00, 1], // índice 0 é a importação
  ]);
  // locais: 2 = i, 3 = sum, 4 = n
  const code = [
    0x01, 0x03, I32,
    0x20, 0x00, 0x41, 0x00, 0x20, 0x01, 0x41, 0x00, // handle, offset 0, length, dst 0
    0x10, 0x00, //                                     call runtime.read
    0x22, 0x04, //                                     local.tee n
    0x41, 0x00, 0x48, //                               n < 0 ?
    0x04, 0x40, 0x41, 0x7f, 0x0f, 0x0b, //             if: return -1
    0x02, 0x40, 0x03, 0x40, //                         block / loop
    0x20, 0x02, 0x20, 0x04, 0x4f, 0x0d, 0x01, //       if i >= n: break
    0x20, 0x03, 0x20, 0x02, 0x2d, 0x00, 0x00, 0x6a, 0x21, 0x03, // sum += mem[i]
    0x20, 0x02, 0x41, 0x01, 0x6a, 0x21, 0x02, //       i++
    0x0c, 0x00, 0x0b, 0x0b, //                         continue / end loop / end block
    0x20, 0x03, 0x0b, //                               return sum
  ];
  const codeSection = vec([[...leb(code.length), ...code]]);
  return new Uint8Array([
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
    ...section(1, types),
    ...section(2, imports),
    ...section(3, funcs),
    ...section(5, memory),
    ...section(7, exports),
    ...section(10, codeSection),
  ]);
}

/** Pacote de dados determinístico (PRNG xorshift), para conferir leituras. */
export function buildDataBundle(size, seed = 0x2545f491) {
  const out = new Uint8Array(size);
  let x = seed >>> 0;
  for (let i = 0; i < size; i++) {
    x ^= x << 13; x >>>= 0;
    x ^= x >>> 17;
    x ^= x << 5; x >>>= 0;
    out[i] = x & 0xff;
  }
  return out;
}

export function byteSum(bytes) {
  let sum = 0;
  for (const b of bytes) sum = (sum + b) | 0;
  return sum;
}
