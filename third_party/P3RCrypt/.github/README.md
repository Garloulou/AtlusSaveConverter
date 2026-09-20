# P3RCrypt

P3RCrypt is a simple tool to encrypt and decrypt Persona 3 Reload save files. Save files for Persona 3 Reload are unencrypted on the Microsoft Store version of the game, but encrypted on the Steam version. This tool allows you to encrypt or decrypt save files to facilitate cross-platform save file usage.

### Usage
1. Navigate to the [releases page](https://github.com/camohiddendj/P3RCrypt/releases) and download the latest release.
2. Open a command prompt and navigate to your downloads folder.
3. Run one of the following commands to encrypt or decrypt a save file:
   - `P3RCrypt.exe encrypt "path\to\savefile"`
   - `P3RCrypt.exe decrypt "path\to\savefile"`

### Save File Locations

Depending on your platform, your Persona 3 Reload save files can be found in the following locations:

- Microsoft Store: `%LOCALAPPDATA%\\Packages\\SEGAofAmericaInc.L0cb6b3aea_s751p9cej88mt\\SystemAppData\\wgs\\<user-id>\\`
- Steam: `%APPDATA%\\SEGA\\P3R\\Steam\\<user-id>\\`
- Steam Play (Linux): `<Steam-folder>/steamapps/compatdata/2161700/pfx/`

If you're having trouble finding your Microsoft Store save files, you can use a tool such as [GPSaveConverter](https://github.com/Fr33dan/GPSaveConverter) to make figuring out the proper names a bit easier. However, we are planning on implementing our own method in a future release.

### Built With
- [Go](https://golang.org/) - The programming language used
- [Cobra](https://github.com/spf13/cobra) - A library for creating powerful modern CLI applications

### License
This project is licensed under the MIT License - see the [LICENSE](../LICENSE) file for details.

### Acknowledgments
- [illusion0001](https://illusion0001.com/) for the original [P3R-Save-EnDecryptor](https://github.com/illusion0001/P3R-Save-EnDecryptor) tool 
- Atlus for creating such a great game