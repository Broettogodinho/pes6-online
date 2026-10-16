# runtime/ — runtime WebAssembly genérico (SCRUM-36, PoC)

Base da Fase 2: carrega um módulo WebAssembly e os pacotes de dados de que ele precisa, sob demanda, de uma URL remota ou de um arquivo local, e faz a ponte entre o código síncrono do módulo e o I/O assíncrono do navegador.

O runtime é genérico e não sabe que aplicação está rodando. Os testes e a demo usam só conteúdo gerado em memória (`tools/dummy-app.js`), e o repositório continua sem nenhum arquivo do jogo (AGENTS.md §1).

## Peças

| Arquivo | Papel |
|---|---|
| `src/sources.js` | Fontes de bytes: `HttpRangeSource` (URL/CDN com HTTP Range; sem suporte a Range funciona, mas baixa tudo e sinaliza), `BlobSource` (arquivo escolhido pelo usuário), `MemorySource` |
| `src/chunk-cache.js` | `ChunkedReader`: blocos fixos, cache LRU, agrupamento de blocos vizinhos numa única Range, requisições compartilhadas, `readSync` para chamadas síncronas, store persistente opcional |
| `src/opfs-store.js` | Store persistente no OPFS do navegador |
| `src/vfs.js` | Sistema de arquivos virtual: caminho → leitor; handles numéricos para o módulo |
| `src/manifest.js` | Manifesto da carga de trabalho (módulo + arquivos), URLs relativas, SHA-256 do módulo |
| `src/wasm-host.js` | Instancia o módulo e expõe `runtime.read` / `runtime.size` |
| `src/gpu.js` | Inicialização do WebGPU num `<canvas>` |
| `src/index.js` | `loadWorkload(manifestUrl)` monta tudo |

## Ponte de I/O (síncrono × assíncrono)

| Modo | Como funciona | Situação |
|---|---|---|
| `jspi` | `runtime.read` é uma importação `WebAssembly.Suspending`: o módulo fica suspenso até o `fetch` terminar | **CONFIRMADO** no Node 24 (teste). No navegador depende do suporte a JSPI (**A VERIFICAR** por navegador) |
| `retry` | `runtime.read` é síncrona; sem os blocos em memória devolve `EAGAIN`, o host busca e chama a exportação de novo | **CONFIRMADO** (testes). Só serve para exportações idempotentes: é fallback de PoC |

`mode: "auto"` usa JSPI quando o runtime oferece.

## Comandos

```sh
cd runtime
npm test        # 19 testes: fontes, cache, VFS, manifesto, host nos dois modos
npm run demo    # servidor local com carga de teste; abrir a URL no navegador
```

## O que esta PoC ainda não responde

- **Execução de um binário x86 no navegador:** emulação x86 em WASM ou recompilação estática. O host acima instancia módulos que já são WASM; é a próxima etapa do SCRUM-36.
- **Tradução gráfica Direct3D 8 → WebGPU (o `pes6.exe` importa `D3D8.DLL`):** aqui só há a inicialização do dispositivo.
- **Offsets acima de 2 GiB:** `runtime.read` usa `i32`; passar a `i64` quando um módulo real precisar.
- **Integridade por bloco dos pacotes de dados:** hoje só o módulo tem SHA-256.
