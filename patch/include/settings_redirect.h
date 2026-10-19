#pragma once

// Atribuição automática de controles (SCRUM-56/57): quando o pes6.exe abre o
// settings.dat só para leitura, a abertura é redirecionada para uma cópia
// montada na hora (settings_builder.h) com os controles conectados em P1, P2…
// O settings.dat real não é alterado, e o settings.exe (que também carrega a
// nossa dinput8.dll) não é afetado: lá o redirecionamento fica desligado.

namespace pes6online::settings_redirect {

// Instala os hooks de CreateFileA/W. Deve ser chamado cedo (DllMain), antes de o
// jogo ler o arquivo. Não faz nada se o processo não for o pes6.exe.
void install();

}  // namespace pes6online::settings_redirect
