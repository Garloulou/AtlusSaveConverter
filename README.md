# AtlusSaveConverter

Application C++ native moderne (Win32 + DirectX 11 + Dear ImGui) pour exporter, convertir et migrer les sauvegardes PlayStation 4 des jeux Atlus vers leurs versions PC (Steam & Microsoft Store / Xbox Game Pass).

---

## 🎮 Jeux Supportés

- **Persona 5 Tactica** (P5T)
  - Identifiants PS4 : `CUSA43147` (US), `CUSA43148` (EU), `CUSA35966` (JP), `CUSA43440` (Asia)
  - Structure PC : Répertoires par emplacement (`100`, `200`, `300`...), contenant `SaveData.dat` (MessagePack/GZIP) et `SaveInfo.dat` (.NET BinaryFormatter sérialisé)
  - Nettoyage du stream GZIP, extraction des métadonnées (nom complet, niveau, temps de jeu, complétion NG+) et protection des paramètres graphiques PC.
  - Thème graphique : **Revolutionary Scarlet** (`#E60026`)

- **Persona 5 Royal** (P5R)
  - Identifiants PS4 : `CUSA17416` (US), `CUSA17419` (EU), `CUSA08216` (JP), `CUSA17418` (Asia)
  - Format : `DATA0001.DAT` ... `DATA0016.DAT`, `SYSTEM.DAT` (Chiffrement AES-256-CBC + compression zlib)
  - Thème graphique : **Phantom Crimson Red** (`#E60012`)

- **Persona 5 Strikers** (P5S)
  - Identifiants PS4 : `CUSA19641` (US), `CUSA19642` (EU), `CUSA19643` (JP), `CUSA19644` / `CUSA19645` (Asia)
  - Format : `SAVEDATA.BIN` (Conteneur multi-emplacements, chiffrement LCG lié au SteamID 64-bit)
  - Thème graphique : **Strikers Hot Red** (`#FF1E27`)

- **Persona 3 Reload** (P3R)
  - Identifiants PS4 : `CUSA43887` (US), `CUSA43888` (EU), `CUSA43885` (JP), `CUSA43886` (Asia)
  - Format : `SaveData0001.sav` ... `SaveData0016.sav`, `SystemData.sav` (GVAS / UE4)
  - Support de l'Épisode Aigis et nettoyage automatique des drapeaux DLC.
  - Thème graphique : **Tartarus Moonlight Blue** (`#00A0E9`)

- **Persona 4 Golden** (P4G)
  - Identifiants PS4 : `CUSA33887` (US), `CUSA33888` (EU), `CUSA33886` (JP), `CUSA33889` (Asia)
  - Format : `data0001.bin` ... `data0016.bin`, compagnon `.binslot` avec signature MD5
  - Thème graphique : **Inaba Sunset Yellow** (`#FFD200`)

- **Persona 3 Portable** (P3P)
  - Identifiants PS4 : `CUSA33877` (US), `CUSA33878` (EU), `CUSA33876` (JP)
  - Format : `data0001.bin` ... `data0016.bin`, `systemsave.bin`
  - Thème graphique : **Portable Cyan** (`#00C3FF`)

- **Shin Megami Tensei V: Vengeance** (SMT5V)
  - Identifiants PS4 : `CUSA43877` (US), `CUSA43878` (EU), `CUSA43876` (JP)
  - Format : `GameSave*.sav`, `SysSave.sav` (AES-256-ECB + Checksum SHA-1)
  - Thème graphique : **Nahobino Cerulean Blue** (`#0066FF`)

- **Soul Hackers 2** (SH2)
  - Identifiants PS4 : `CUSA27525` (US), `CUSA27526` (EU), `CUSA27524` (JP)
  - Format : Répertoires `SaveData0001` ... `SaveData0016`, `SysSave`
  - Thème graphique : **Aion Neon Mint** (`#00FF88`)

---

## 🚀 Fonctionnalités du Front-End

- **Sélecteur Dynamique de Jeux** : Adaptation automatique de l'interface, des thèmes graphiques, des dossiers attendus et des pipelines de conversion.
- **Sélecteurs Natifs Windows Shell** : Intégration de l'API moderne `IFileDialog` / `IFileOpenDialog` pour parcourir vos dossiers et fichiers confortablement.
- **Auto-Détection des Comptes et Chemins Steam** : Analyse instantanée des répertoires d'installation Steam et sélection automatique du SteamID actif.
- **Inspection des Métadonnées** : Affichage en direct du nom du protagoniste, niveau d'équipe, temps de jeu formaté, statut de fin de jeu (NG+ / Terminé) et difficulté.
- **Remappage d'Emplacements** : Choix libre du slot cible sur PC (ex: Slot PS4 02 ➔ Slot PC 05) avec copies de sauvegarde de sécurité `.bak`.
- **Export en Chaîne** : Détection et conversion automatique de l'ensemble d'un dossier de sauvegardes en un seul clic.
- **Console de Journalisation Temps Réel** : Logs détaillés avec code couleur (INFO, SUCCÈS, AVERTISSEMENT, ERREUR).

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
