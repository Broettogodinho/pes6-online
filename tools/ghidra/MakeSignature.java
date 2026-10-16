// Gera a assinatura (padrão de bytes) curta e única de funções, para o patch validar
// antes de instalar um hook (AGENTS.md §3).
//
// Regras do padrão:
//   - começa na entrada da função e segue instrução a instrução;
//   - bytes que dependem de endereço viram "??": deslocamentos relativos de call/jmp
//     (rel32) e operandos de 4 bytes cujo valor cai dentro da imagem;
//   - cresce até ser único no executável (mínimo 16 bytes, máximo 64).
//
// Uso (headless): -postScript MakeSignature.java <arquivo_saida.tsv> <endereco1,endereco2,...>
// Saída: entrada, nome, tamanho do padrão, ocorrências, padrão.
// @category PES6Online

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.scalar.Scalar;

public class MakeSignature extends GhidraScript {

    private static final int MIN_LEN = 16;
    private static final int MAX_LEN = 64;

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        String out = args.length > 0 ? args[0] : askString("Saída", "Arquivo TSV:");
        // O PowerShell quebra "a,b" em argumentos separados: junta tudo depois da saída.
        String list = args.length > 1 ? String.join(",", java.util.Arrays.copyOfRange(args, 1, args.length))
                : askString("Endereços", "Entradas (vírgula):");
        String[] addrs = list.split("[,;\s]+");
        try (PrintWriter w = new PrintWriter(out, "UTF-8")) {
            w.println("entry\tname\tlength\tmatches\tpattern");
            for (String a : addrs) {
                Address entry = toAddr(a.trim());
                Function f = getFunctionAt(entry);
                String name = f == null ? "-" : f.getName();
                List<Integer> pattern = new ArrayList<>(); // -1 = curinga
                Instruction ins = getInstructionAt(entry);
                int matches = -1;
                while (ins != null && pattern.size() < MAX_LEN) {
                    appendMasked(ins, pattern);
                    if (pattern.size() >= MIN_LEN) {
                        matches = countMatches(pattern);
                        if (matches == 1) break;
                    }
                    ins = ins.getNext();
                }
                w.println(entry + "\t" + name + "\t" + pattern.size() + "\t" + matches + "\t" + format(pattern));
            }
        }
        println("MakeSignature: " + addrs.length + " funções -> " + out);
    }

    private void appendMasked(Instruction ins, List<Integer> pattern) throws Exception {
        byte[] bytes = ins.getBytes();
        boolean[] wild = new boolean[bytes.length];
        int op = bytes[0] & 0xff;
        // call/jmp rel32 (E8/E9) e jcc rel32 (0F 8x): deslocamento relativo
        if ((op == 0xE8 || op == 0xE9) && bytes.length == 5) mark(wild, 1, 4);
        if (op == 0x0F && bytes.length == 6 && (bytes[1] & 0xf0) == 0x80) mark(wild, 2, 4);
        // operandos de 4 bytes com valor dentro da imagem (endereços absolutos)
        long lo = currentProgram.getMinAddress().getOffset();
        long hi = currentProgram.getMaxAddress().getOffset();
        for (int i = 0; i + 4 <= bytes.length; i++) {
            long v = (bytes[i] & 0xffL) | (bytes[i + 1] & 0xffL) << 8 | (bytes[i + 2] & 0xffL) << 16
                    | (bytes[i + 3] & 0xffL) << 24;
            if (v >= lo && v <= hi && hasAddressOperand(ins, v)) mark(wild, i, 4);
        }
        for (int i = 0; i < bytes.length; i++) pattern.add(wild[i] ? -1 : bytes[i] & 0xff);
    }

    private boolean hasAddressOperand(Instruction ins, long value) {
        for (int i = 0; i < ins.getNumOperands(); i++) {
            for (Object o : ins.getOpObjects(i)) {
                if (o instanceof Address a && a.getOffset() == value) return true;
                if (o instanceof Scalar s && s.getUnsignedValue() == value) return true;
            }
        }
        return false;
    }

    private static void mark(boolean[] wild, int from, int n) {
        for (int i = from; i < from + n && i < wild.length; i++) wild[i] = true;
    }

    private int countMatches(List<Integer> pattern) throws Exception {
        byte[] bytes = new byte[pattern.size()];
        byte[] mask = new byte[pattern.size()];
        for (int i = 0; i < bytes.length; i++) {
            int b = pattern.get(i);
            bytes[i] = (byte) (b < 0 ? 0 : b);
            mask[i] = (byte) (b < 0 ? 0 : 0xff);
        }
        Memory mem = currentProgram.getMemory();
        int count = 0;
        Address start = currentProgram.getMinAddress();
        while (count < 2) {
            Address found = mem.findBytes(start, bytes, mask, true, monitor);
            if (found == null) break;
            count++;
            start = found.add(1);
        }
        return count;
    }

    private static String format(List<Integer> pattern) {
        StringBuilder sb = new StringBuilder();
        for (int b : pattern) {
            if (sb.length() > 0) sb.append(' ');
            sb.append(b < 0 ? "??" : String.format("%02X", b));
        }
        return sb.toString();
    }
}
