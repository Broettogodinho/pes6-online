// Lista quem referencia endereços de dados (leitura/escrita) ou código, agrupado por função.
// Uso (headless): -postScript XrefsTo.java <arquivo_saida.tsv> <endereco1,endereco2,...>
// Saída: alvo, tipo de referência, endereço de origem, função de origem. Fica fora do repo.
// @category PES6Online

import java.io.PrintWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;

public class XrefsTo extends GhidraScript {

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        String out = args.length > 0 ? args[0] : askString("Saída", "Arquivo TSV:");
        // O PowerShell quebra "a,b" em argumentos separados: junta tudo depois da saída.
        String list = args.length > 1 ? String.join(",", java.util.Arrays.copyOfRange(args, 1, args.length))
                : askString("Endereços", "Alvos (vírgula):");
        try (PrintWriter w = new PrintWriter(out, "UTF-8")) {
            w.println("target\tref_type\tfrom\tfunction\tfunction_entry");
            for (String a : list.split("[,;\\s]+")) {
                Address target = toAddr(a.trim());
                for (Reference r : getReferencesTo(target)) {
                    Function f = getFunctionContaining(r.getFromAddress());
                    w.println(target + "\t" + r.getReferenceType() + "\t" + r.getFromAddress() + "\t"
                            + (f == null ? "-" : f.getName()) + "\t" + (f == null ? "-" : f.getEntryPoint()));
                }
            }
        }
        println("XrefsTo: -> " + out);
    }
}
