# Scripts do Ghidra

Scripts nossos para a engenharia reversa do `pes6.exe` (SCRUM-18/24). Rodam no Ghidra headless; nenhuma saída deles é versionada diretamente.

## Onde fica cada coisa

| O quê | Onde |
|---|---|
| Ghidra | `C:\Tools\ghidra_12.1.4_PUBLIC` |
| Projeto do Ghidra (contém o binário do jogo) | `D:\RE\pes6` — **fora do repositório** |
| Saídas brutas dos scripts | `D:\RE\out` — **fora do repositório** |
| Resultado curado (nomes, endereços, assinaturas, evidência) | `re/symbols/target.toml` e `re/notes/` |

## Scripts

| Script | Argumentos | Para quê |
|---|---|---|
| `ExportImports.java` | `<saida.tsv>` | APIs do Windows importadas pelo jogo e nº de referências (ex.: DirectInput para o SCRUM-56) |
| `FindStrings.java` | `<saida.tsv> <chave1,chave2,...>` | Strings com as palavras-chave e as funções que as usam: ponto de partida para Copa, Liga e menus |
| `MakeSignature.java` | `<saida.tsv> <endereco1,...>` | Assinatura curta e única de cada função, com `??` nos bytes que dependem de endereço |

## Exemplo

```powershell
$G = "C:\Tools\ghidra_12.1.4_PUBLIC\support\analyzeHeadless.bat"
& $G D:\RE\pes6 pes6 -process pes6.exe -noanalysis -readOnly `
  -scriptPath tools\ghidra -postScript FindStrings.java D:\RE\out\strings.tsv "cup,league,menu"
```

`-readOnly` evita salvar alterações no projeto; para gravar nomes de funções no projeto, rode sem ele.

## Erros no VS Code

A extensão Java do VS Code acusa `The import ghidra... cannot be resolved` porque não conhece as bibliotecas do Ghidra. O Ghidra fornece essas bibliotecas ao rodar o script, então os erros são só do editor. Para eliminá-los, adicione ao `.vscode/settings.json` local (não versionado):

```json
"java.project.sourcePaths": ["tools/ghidra"],
"java.project.referencedLibraries": [
  "C:/Tools/ghidra_12.1.4_PUBLIC/Ghidra/Framework/*/lib/*.jar",
  "C:/Tools/ghidra_12.1.4_PUBLIC/Ghidra/Features/Base/lib/*.jar",
  "C:/Tools/ghidra_12.1.4_PUBLIC/Ghidra/Features/Decompiler/lib/*.jar",
  "C:/Tools/ghidra_12.1.4_PUBLIC/Ghidra/Processors/x86/lib/*.jar"
]
```

## REA (servidor MCP)

O `npx rea-agents setup` não registra no Claude Code quando ele roda só pela extensão do VS Code (sem o comando `claude` no terminal). O registro fica no `.mcp.json` da raiz do repositório (`rea-agents@6.5.0 --mcp`, com `GHIDRA_INSTALL_DIR` apontando para o Ghidra). Depois de recarregar a janela, o VS Code pede para aprovar o servidor.
