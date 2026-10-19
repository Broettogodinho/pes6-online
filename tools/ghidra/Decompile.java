// Decompila funções e grava o pseudocódigo, as chamadas e as strings usadas, para
// leitura e anotação. A saída contém código derivado do jogo: fica SEMPRE fora do
// repositório (ex.: D:\RE\out). Em re/notes vai só o resumo escrito por nós.
// Uso (headless): -postScript Decompile.java <arquivo_saida.txt> <endereco1,endereco2,...>
// @category PES6Online

import java.io.PrintWriter;
import java.util.Set;
import java.util.TreeSet;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.symbol.Reference;

public class Decompile extends GhidraScript {

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        String out = args.length > 0 ? args[0] : askString("Saída", "Arquivo:");
        // O PowerShell quebra "a,b" em argumentos separados: junta tudo depois da saída.
        String list = args.length > 1 ? String.join(",", java.util.Arrays.copyOfRange(args, 1, args.length))
                : askString("Endereços", "Entradas (vírgula):");
        DecompInterface dec = new DecompInterface();
        dec.openProgram(currentProgram);
        try (PrintWriter w = new PrintWriter(out, "UTF-8")) {
            for (String a : list.split("[,;\\s]+")) {
                Address addr = toAddr(a.trim());
                Function f = getFunctionContaining(addr);
                if (f == null) {
                    // Métodos chamados só por tabela virtual podem não ter virado função na
                    // análise automática: desmonta e cria a função na hora (sem salvar com -readOnly).
                    disassemble(addr);
                    f = createFunction(addr, null);
                    if (f == null) {
                        w.println("=== " + a + ": sem função");
                        continue;
                    }
                    w.println("// função criada pelo script em " + addr);
                }
                w.println("=== " + f.getName() + " @ " + f.getEntryPoint() + " (" + f.getBody().getNumAddresses()
                        + " bytes)");
                Set<String> callers = new TreeSet<>();
                for (Function c : f.getCallingFunctions(monitor)) callers.add(c.getName() + "@" + c.getEntryPoint());
                Set<String> callees = new TreeSet<>();
                for (Function c : f.getCalledFunctions(monitor)) callees.add(c.getName() + "@" + c.getEntryPoint());
                w.println("callers: " + callers);
                w.println("callees: " + callees);
                Set<String> strings = new TreeSet<>();
                for (Instruction ins : currentProgram.getListing().getInstructions(f.getBody(), true)) {
                    for (Reference r : ins.getReferencesFrom()) {
                        Data d = getDataAt(r.getToAddress());
                        if (d != null && d.hasStringValue()) strings.add(r.getToAddress() + "=" + d.getValue());
                    }
                }
                w.println("strings: " + strings);
                DecompileResults res = dec.decompileFunction(f, 60, monitor);
                w.println(res.decompileCompleted() ? res.getDecompiledFunction().getC()
                        : "// decompilação falhou: " + res.getErrorMessage());
                w.println();
            }
        } finally {
            dec.dispose();
        }
        println("Decompile: -> " + out);
    }
}
