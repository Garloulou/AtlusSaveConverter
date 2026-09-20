package cmd

import (
	"fmt"
	"github.com/spf13/cobra"
	"os"
	"log"
)

// backupCmd represents the backup command
var patchCmd = &cobra.Command{
	Use:   "patch",
	Short: "Backup all Steam Persona 3 Reload save files and remove DLC flags.",
	Long: `Backup all Steam Persona 3 Reload save files to a folder named backup, then remove all DLC flags that may prevent the game from loading. No arguments required.`,
	Run: func(cmd *cobra.Command, args []string) {
		if len(args) > 0 {
			fmt.Println("Usage: P3RCleaner patch")
			os.Exit(1)
		}
		RunBackup();

		for i := 1; i <= 15; i++ {
			savefile := fmt.Sprintf("SaveData%03d.sav", i)			//Basegame saves
			log.Println(savefile, ":")
			data, err := os.ReadFile(savefile)				//Load save
			if err != nil {
				if os.IsNotExist(err) {
					log.Println("Does not exist. Skipping.")
					continue
				}
				log.Println("Error reading", savefile, ":", err)
				continue
			}
			if len(data) < minsize || len(data) > maxsize {
				log.Println(savefile, "does not seem to be a valid P3R save. Skipping.")
				continue
			}
			decryptedFile := decrypt(data, encKey)				//Decrypt save
			cleanDLCA(decryptedFile)					//Find and remove all three DLC flag sets.
			cleanDLCB(decryptedFile)
			cleanDLCC(decryptedFile)
			encryptedFile := encrypt(decryptedFile, encKey)			//Re-encrypt save
			err = os.WriteFile(savefile, encryptedFile, 0644)
			if err != nil {
				log.Println("Error writing", savefile, ":", err)
				continue
			}
		}
		for i := 1; i <= 15; i++ {				//There's a better way to do this but I don't feel like breaking something.
			savefile := fmt.Sprintf("SaveData1%03d.sav", i)			//Episode Aigis saves
			log.Println(savefile, ":")
			data, err := os.ReadFile(savefile)				//Load save
			if err != nil {
				if os.IsNotExist(err) {
					log.Println("Does not exist. Skipping.")
					continue
				}
				log.Println("Error reading", savefile, ":", err)
				continue
			}
			if len(data) < minsize || len(data) > maxsize {
				log.Println(savefile, "does not seem to be a valid P3R save. Skipping.")
				continue
			}
			decryptedFile := decrypt(data, encKey)				//Decrypt save
			cleanDLCA(decryptedFile)					//Find and remove all three DLC flag sets.
			cleanDLCB(decryptedFile)
			cleanDLCC(decryptedFile)
			encryptedFile := encrypt(decryptedFile, encKey)			//Re-encrypt save
			err = os.WriteFile(savefile, encryptedFile, 0644)
			if err != nil {
				log.Println("Error writing", savefile, ":", err)
				continue
			}
		}

	},
}

func init() {
	rootCmd.AddCommand(patchCmd)
}
