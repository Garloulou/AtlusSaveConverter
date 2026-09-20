package cmd

import (
	"fmt"
	"github.com/spf13/cobra"
	"os"
	"log"
	"path/filepath"
)

// backupCmd represents the backup command
var backupCmd = &cobra.Command{
	Use:   "backup",
	Short: "Backup all Steam Persona 3 Reload save files.",
	Long: `Backup all Steam Persona 3 Reload save files to a folder named backup. No arguments required. Manual backups are still recommended.`,
	Run: func(cmd *cobra.Command, args []string) {
		if len(args) > 0 {
			fmt.Println("Usage: P3RCleaner backup")
			os.Exit(1)
		}
		if err := RunBackup(); err !=nil {
			log.Fatal(err)

		}
	},
}
func RunBackup() error {
			err := os.MkdirAll("backup", 0755)
			if err != nil {
				return err
			}
		for i := 1; i <= 15; i++ {
			savefile := fmt.Sprintf("SaveData%03d.sav", i)	//Basegame saves
			data, err := os.ReadFile(savefile)
			if err != nil {
				if os.IsNotExist(err) {
					continue
				}
				log.Println("Error reading", savefile, ":", err)
				continue
			}
			backupfile := filepath.Join("backup", fmt.Sprintf("SaveData%03d.sav", i))
			err = os.WriteFile(backupfile, data, 0644)
			if err != nil {
			log.Println("Error creating", backupfile, ":", err)
			continue
			}
		}
		for i := 1; i <= 15; i++ {
			savefile := fmt.Sprintf("SaveData1%03d.sav", i)	//Episode Aigis saves
			data, err := os.ReadFile(savefile)
			if err != nil {
				if os.IsNotExist(err) {
					continue
				}
				log.Println("Error reading", savefile, ":", err)
				continue
			}
			backupfile := filepath.Join("backup", fmt.Sprintf("SaveData1%03d.sav", i))
			err = os.WriteFile(backupfile, data, 0644)
			if err != nil {
				log.Println("Error creating", backupfile, ":", err)
				continue
			}
		}
	return nil
}

// init adds the backup command to the root command
func init() {
	rootCmd.AddCommand(backupCmd)
}
