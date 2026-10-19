# Textos do jogo nos `*_text.afs` (SCRUM-63)

- **Versão-alvo:** instalação padrão (o formato dos `.afs` não depende do hash do executável).
- **Como foi obtido (2026-10-19):** leitura dos arquivos do jogo (`E:\...\dat\e_text.afs`), sem executar nada; ferramenta `tools/text/pes6text.py`.

## Idiomas instalados

`dat/` tem `0_*.afs` (comum) e `e_`, `f_`, `g_`, `i_`, `s_`, `p_` (`_text` e `_sound`) = inglês, francês, alemão, italiano, espanhol e **polonês** (o `p` é Polski, CONFIRMADO no seletor de idioma do jogo). **Não há português.**

## e_text.afs

- **AFS:** `AFS\0`, quantidade (421) e tabela `(offset, tamanho)`.
- **392 arquivos `00 02 00 00`:** contêineres de imagens (nomes internos como `uni_l_0127`, `jfa_flier3`). Texto desenhado em textura, fora do escopo.
- **29 arquivos `00 06 01 00`:** **tabelas de texto**, comprimidas com zlib, com **14.991 textos** no total (índices 1, 2, 3, 4, 6, 10–14, 127, 150, 154–156, 191, 283, 348, 352, 358, 363, 365, 370, 372–374, 379, 384, 416).

## Contêiner `00 06 01 00`

| Offset | Campo |
|---|---|
| `0x00` | assinatura `00 06 01 00` |
| `0x04` | tamanho comprimido |
| `0x08` | tamanho descomprimido |
| `0x20` | dados zlib (`78 9C`) |

## Tabela descomprimida (INFERIDO; validado pela ferramenta nas 29 tabelas)

- **Cabeçalho global:** `[u32 nº de seções][u32 8][u32 deslocamento de cada seção…]`.
- **Seção:**
  - `+0x00` = `0x400`;
  - `+0x04` e `+0x08` = tamanho dos dados;
  - dados a partir de `+0x20`: `[u32 quantidade][u32 ?]`, depois registros de 12 bytes `[u32 id][u16 tamanho com \0][u16 comprimento][u32 deslocamento]`, depois as strings terminadas em `\0`;
  - o deslocamento é relativo ao início dos dados (`seção + 0x20`);
  - a seção é alinhada a 16 bytes.
- **Particularidades:** strings repetidas são **compartilhadas** por mais de um registro, e a ordem nem sempre é a dos registros. Por isso a ferramenta não reconstrói "do zero": mantém os bytes originais e só acrescenta os textos alterados ao fim da área de textos da seção.
- **Codificação:** 1 byte por caractere (Latin-1, INFERIDO). Que a fonte do jogo tenha `ã`, `õ`, `ç` ainda está **A VERIFICAR** no jogo.

## Validação (ferramenta)

`python tools/text/pes6text.py selftest e_text.afs`: nas 29 tabelas, sem alteração a saída é idêntica; alterando o primeiro e o último texto de cada seção (com acentos), os textos lidos de volta batem e todos os outros ficam intactos. **OK** (2026-10-19).

## Instalação da tradução

Pelo **afs2fs** do Kitserver: `Kitserver\dat\e_text.afs\unnamed_<índice>.bin` (manual do Kitserver, seção 8). O afs2fs aceita várias pastas raiz no `afs2fs.cfg`, então a tradução pode ficar numa raiz própria e ser ligada ou desligada. Os textos extraídos e traduzidos são derivados do jogo e ficam **fora do repositório** (`D:\RE\text\`).
