package cmd

import (
	"fmt"
	"github.com/spf13/cobra"
	"os"
	"log"
	"bytes"
	"path/filepath"
)

const minsize = 400 * 1024	//I don't believe it's possible to create a save file smaller than this.
const maxsize = 2000 * 1024	//I don't believe it's possible to create a save file larger than this.
//These size limits are used to double check that no other files are accidentally grabbed. Most saves are between 700KB and 1.1 MB, with the largest I've seen being about 1.4MB.
const searchlimit = 0x3000	//All DLC flags should be located at least a few hundred bytes before this point. This limit should prevent the searches for flag sets B and C from hitting false positives.

var dlcflagsetA = []byte{
	0x53, 0x61, 0x76, 0x65, 0x44, 0x61, 0x74, 0x61,
	0x41, 0x72, 0x65, 0x61, 0x00, 0x0F, 0x00, 0x00,
	0x00, 0x55, 0x49, 0x6E, 0x74, 0x33, 0x32, 0x50,
	0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x98, 0x01, 0x00, 0x00,
	0x00,
}
var dlcflagsetB = []byte{
	0x53, 0x61, 0x76, 0x65, 0x44, 0x61, 0x74, 0x61,
	0x41, 0x72, 0x65, 0x61, 0x00, 0x0F, 0x00, 0x00,
	0x00, 0x55, 0x49, 0x6E, 0x74, 0x33, 0x32, 0x50,
	0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x9B,
}
var dlcflagsetC = []byte{0x53, 0x61, 0x76, 0x65, 0x44, 0x61, 0x74, 0x61,
	0x41, 0x72, 0x65, 0x61, 0x00, 0x0F, 0x00, 0x00,
	0x00, 0x55, 0x49, 0x6E, 0x74, 0x33, 0x32, 0x50,
	0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x97, 0x01, 0x00, 0x00,
	0x00,
}
var dlcflagtail = []byte{
	0x80, 0x0D, 0x00, 0x00, 0x00}

// backupCmd represents the backup command
var msCmd = &cobra.Command{
	Use:   "ms",
	Short: "Backup a Microsoft Store/Gamepass Persona 3 Reload save file and remove DLC flags.",
	Long: `Backup a Microsoft Store/Gamepass Persona 3 Reload save file to a folder named backup, then remove all DLC flags that may prevent the game from loading. Requires filename.`,
	Run: func(cmd *cobra.Command, args []string) {
		if len(args) != 1 {
			fmt.Println("Usage: P3RCleaner ms <savefile>")
			os.Exit(1)
		}
		filename := args[0]
		data, err := os.ReadFile(filename)				//Load save
		if err != nil {
			log.Println("Error reading", filename, ":", err)
			os.Exit(1)
		}
		if len(data) < minsize || len(data) > maxsize {
			log.Println(filename, "does not seem to be a valid P3R save.")
			os.Exit(1)
		}
		err = os.MkdirAll("backup", 0755)
		if err != nil {
			log.Println("Could not create backup folder. Please check write permissions.")
			os.Exit(1)
		}
		backupfile := filepath.Join("backup", filepath.Base(filename))
		err = os.WriteFile(backupfile, data, 0644)
		if err != nil {
			log.Println("Error creating", backupfile, ":", err, ". Check write permissions.")
			os.Exit(1)
		}
		cleanDLCA(data)					//Find and remove all three DLC flag sets.
		cleanDLCB(data)
		cleanDLCC(data)
		err = os.WriteFile(filename, data, 0644)
		if err != nil {
			log.Println("Error writing", filename, ":", err, ". Check write permissions.")
		}
	},

}

func init() {
	rootCmd.AddCommand(msCmd)
}

func cleanDLCA(buf []byte) bool {
	limit := buf[:searchlimit]
	idx := bytes.Index(limit, dlcflagsetA)
	if idx == -1 {
		log.Println("First set of DLC flags not found. Skipping.")
		return false 	//DLC flag set A not present
	}
	pos := idx + len(dlcflagsetA)
	if buf[pos] == 0x00 && buf[pos+1] == 0x00 {
		log.Println("The location of the first set of DLC flags exists, but has already been cleared.")
		return false	//This program has already run on this save.
	}
	buf[pos] = 0x00		//Remove all flags associated with the first field.
	buf[pos+1] = 0x00
	log.Println("First set of DLC flags replaced.")
	return true
}

func cleanDLCB(buf []byte) bool {
	limit := buf[:searchlimit]
	idx := bytes.Index(limit, dlcflagsetB)
	if idx == -1 {
		log.Println("Second set of DLC flags not found. Skipping.")
		return false 	//DLC flag set B not present
	}
	pos := idx + len(dlcflagsetB)
	buf[pos-1] = 0x97	//Remove all DLC flags associated with the second field.
	log.Println("Second set of DLC flags replaced.")
	return true
}

func cleanDLCC(buf []byte) bool {			//Avoiding false positives is a huge pain in the ass.
	limit := buf[:searchlimit]
	idx := bytes.Index(limit, dlcflagsetC)
	if idx == -1 {
		log.Println("Final set of DLC flags not found. Skipping.")
		return false
	}

	tailPos := idx + len(dlcflagsetC) + 3
	if tailPos+len(dlcflagtail) > searchlimit {
		log.Println("Final set of DLC flags not found. Skipping.")
		return false
	}

	if bytes.Equal(buf[tailPos:tailPos+len(dlcflagtail)], dlcflagtail) {
		buf[tailPos] = 0x00
		log.Println("Final set of DLC flags replaced.")
		return true
	}
	log.Println("Final set of DLC flags not found. Skipping.")
	return false
}

