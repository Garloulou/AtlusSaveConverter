# AtlusSaveConverter

Une application pour convertire les save des different jeu atlus sur pc

---

## 🎮 Jeux Supportés
# **Importent !** Uniquement les version eu on etait testé
- **Persona 5 Tactica** (P5T)
  - Identifiants PS4 : `CUSA43147` (US), `CUSA43148` (EU)

- **Persona 5 Royal** (P5R)
  - Identifiants PS4 : `CUSA17416` (US), `CUSA17419` (EU)

- **Persona 5 Strikers** (P5S)
  - Identifiants PS4 : `CUSA19641` (US), `CUSA19642` (EU)
- **Persona 3 Reload** (P3R)
  - Identifiants PS4 : `CUSA43887` (US), `CUSA43888` (EU)
  - 
- **Persona 4 Golden** (P4G)
  - Identifiants PS4 : `CUSA33887` (US), `CUSA33888` (EU)

- **Persona 3 Portable** (P3P)
  - Identifiants PS4 : `CUSA33877` (US), `CUSA33878` (EU)


- **Shin Megami Tensei V: Vengeance** (SMT5V)
  - Identifiants PS4 : `CUSA43877` (US), `CUSA43878` (EU)


- **Soul Hackers 2** (SH2)
  - Identifiants PS4 : `CUSA27525` (US), `CUSA27526` (EU)
 
- **Metaphor: ReFantazio** (MF) **TODO**
  - Identifiants PS4 : `CUSA-47037` (US), ` CUSA-47038` (EU)

---

## 🛠️ Compilation

### Prérequis
- Windows 10 ou 11 (64-bit)
- Visual Studio 2022 (avec le composant « Développement Desktop en C++ »)
- CMake 3.20 ou supérieur

### Compiler avec le script PowerShell :
```powershell
.\build.ps1 -Run
```

### Ou manuellement avec CMake :
```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\AtlusSaveConverter.exe
```

---

## 📜 Licence

Projet open source distribué sous licence MIT.
