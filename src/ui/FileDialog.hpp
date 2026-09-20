#pragma once

#include <string>

namespace Ps4Atlus {

class NativeDialogs {
public:
    static bool SelectFile(std::string& outPath, const wchar_t* title = L"Sélectionner un fichier de sauvegarde", const wchar_t* filterSpec = L"Fichiers de sauvegarde (*.dat;*.sav;*.bin)\0*.dat;*.sav;*.bin\0Tous les fichiers (*.*)\0*.*\0");
    static bool SelectFolder(std::string& outPath, const wchar_t* title = L"Sélectionner un dossier");
};

} // namespace Ps4Atlus
