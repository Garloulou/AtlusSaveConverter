# P3RCleaner

P3RCleaner is a simple command-line tool to remove DLC flags from any Persona 3 Reload save file. It is forked from and retains all functionality of [P3RCrypt](https://github.com/camohiddendj/P3RCrypt) by camohiddendj.

### Usage
1. Navigate to the [releases page](https://github.com/CLGZFGFrankerZ123/P3RCleaner/releases) and download the latest release.
2. Move move the downloaded P3RCleaner prgram to your P3R save folder. (See below for details.)
3. Open your command prompt (Windows) or terminal (Linux) in the save folder.
4. Run one of the following commands:
   - `P3RCleaner.exe patch` (Steam, Windows)
   - `P3RCleaner.exe ms <savefile>` (Microsoft Store/Gamepass)
   - `./P3RCleaner patch` (Steam, Linux)

### Save File Locations

Depending on your platform, your Persona 3 Reload save files can be found in the following locations:

- Microsoft Store: `%LOCALAPPDATA%\\Packages\\SEGAofAmericaInc.L0cb6b3aea_s751p9cej88mt\\SystemAppData\\wgs\\<user-id>\\`
- Steam: `%APPDATA%\\SEGA\\P3R\\Steam\\<user-id>\\`
- Steam Play (Linux): `<Steam-folder>/steamapps/compatdata/2161700/pfx/`

If you're having trouble finding your Microsoft Store save files, you can use a tool such as [GPSaveConverter](https://github.com/Fr33dan/GPSaveConverter) to make figuring out the proper names a bit easier.

### Built With
- [Go](https://golang.org/) - The programming language used
- [Cobra](https://github.com/spf13/cobra) - A library for creating powerful modern CLI applications

### License
This project is licensed under the MIT License - see the [LICENSE](../LICENSE) file for details.

### Acknowledgments
- [illusion0001](https://illusion0001.com/) for the original [P3R-Save-EnDecryptor](https://github.com/illusion0001/P3R-Save-EnDecryptor) tool 
- [camohiddendj](https://github.com/camohiddendj) for [P3RCrypt](https://github.com/camohiddendj/P3RCrypt), which this program is forked from.
- Atlus for creating such a great game
