// Procura strings definidas que contenham alguma palavra-chave e lista quem as usa.
// Uso (headless): -postScript FindStrings.java <arquivo_saida.tsv> <palavra1,palavra2,...>
// Saída: endereço da string, texto (cortado em 80 caracteres), endereço da referência,
// função que referencia (nome e entrada). Fica fora do repositório (ex.: D:\RE\out).
// @category PES6Online

import java.io.PrintWriter;
import java.util.Locale;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;

public class FindStrings extends GhidraScript {

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        String out = args.length > 0 ? args[0] : askString("Saída", "Arquivo TSV:");
        // O PowerShell quebra "a,b" em argumentos separados: junta tudo depois da saída.
        String list = args.length > 1 ? String.join(",", java.util.Arrays.copyOfRange(args, 1, args.length))
                : askString("Chaves", "Palavras (vírgula):");
        String[] keys = list.toLowerCase(Locale.ROOT).split(",");
        int hits = 0;
        try (PrintWriter w = new PrintWriter(out, "UTF-8")) {
            w.println("string_addr\tkey\ttext\tref_addr\tfunction\tfunction_entry");
            DataIterator it = currentProgram.getListing().getDefinedData(true);
            while (it.hasNext() && !monitor.isCancelled()) {
                Data d = it.next();
                if (!d.hasStringValue()) continue;
                String text = String.valueOf(d.getValue());
                String low = text.toLowerCase(Locale.ROOT);
                for (String key : keys) {
                    if (key.isBlank() || !low.contains(key.trim())) continue;
                    String shown = text.replaceAll("[\\t\\r\\n]", " ");
                    if (shown.length() > 80) shown = shown.substring(0, 80) + "…";
                    Reference[] refs = getReferencesTo(d.getAddress());
                    if (refs.length == 0) {
                        w.println(d.getAddress() + "\t" + key.trim() + "\t" + shown + "\t-\t-\t-");
                    }
                    for (Reference r : refs) {
                        Function f = getFunctionContaining(r.getFromAddress());
                        w.println(d.getAddress() + "\t" + key.trim() + "\t" + shown + "\t" + r.getFromAddress()
                                + "\t" + (f == null ? "-" : f.getName()) + "\t" + (f == null ? "-" : f.getEntryPoint()));
                    }
                    hits++;
                    break;
                }
            }
        }
        println("FindStrings: " + hits + " strings -> " + out);
    }
}
