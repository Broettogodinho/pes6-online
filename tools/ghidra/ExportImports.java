// Lista as funções importadas (DLL!função) e quantas referências cada uma tem.
// Uso (headless): -postScript ExportImports.java <arquivo_saida.tsv>
// A saída fica fora do repositório (ex.: D:\RE\out). Só nomes de API: nada do jogo.
// @category PES6Online

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.symbol.ExternalLocation;
import ghidra.program.model.symbol.ExternalLocationIterator;
import ghidra.program.model.symbol.ExternalManager;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.Symbol;

public class ExportImports extends GhidraScript {

    @Override
    protected void run() throws Exception {
        String out = getScriptArgs().length > 0 ? getScriptArgs()[0] : askString("Saída", "Arquivo TSV:");
        ExternalManager ext = currentProgram.getExternalManager();
        List<String> lines = new ArrayList<>();
        for (String lib : ext.getExternalLibraryNames()) {
            ExternalLocationIterator it = ext.getExternalLocations(lib);
            while (it.hasNext()) {
                ExternalLocation loc = it.next();
                Symbol sym = loc.getSymbol();
                Reference[] refs = getReferencesTo(sym.getAddress());
                lines.add(lib + "\t" + loc.getLabel() + "\t" + refs.length + "\t" + callers(sym));
            }
        }
        lines.sort(String::compareToIgnoreCase);
        try (PrintWriter w = new PrintWriter(out, "UTF-8")) {
            w.println("dll\tfunction\trefs\tcallers");
            lines.forEach(w::println);
        }
        println("ExportImports: " + lines.size() + " importações -> " + out);
    }

    // Funções que usam a importação: refs diretas ao símbolo externo e às
    // entradas da IAT (thunks), até 8 por API.
    private String callers(Symbol sym) {
        java.util.Set<String> found = new java.util.LinkedHashSet<>();
        java.util.List<ghidra.program.model.address.Address> targets = new java.util.ArrayList<>();
        targets.add(sym.getAddress());
        for (Reference r : getReferencesTo(sym.getAddress())) targets.add(r.getFromAddress());
        for (ghidra.program.model.address.Address t : targets) {
            for (Reference r : getReferencesTo(t)) {
                ghidra.program.model.listing.Function f = getFunctionContaining(r.getFromAddress());
                if (f != null) found.add(f.getName() + "@" + f.getEntryPoint());
                if (found.size() >= 8) return String.join(",", found);
            }
        }
        return found.isEmpty() ? "-" : String.join(",", found);
    }
}
