package cmd

import (
	"os"

	"github.com/spf13/cobra"
)

// encKey is a constant that defines the encryption/decryption key used for processing save files.
const encKey = "ae5zeitaix1joowooNgie3fahP5Ohph"

// rootCmd represents the base command when called without any subcommands
var rootCmd = &cobra.Command{
	Use:   "P3RCleaner",
	Short: "A simple tool to encrypt, decrypt, and remove DLC from Persona 3 Reload save files.",
	Long: `Save files for Persona 3 Reload are unencrypted on the Microsoft Store version of the game,
but encrypted on the Steam version. This tool allows you to encrypt or decrypt save files to facilitate
cross-platform save file usage, as well as remove DLC flags from your save files.`,
}

// Execute adds all child commands to the root command and sets flags appropriately.
// This is called by main.main(). It only needs to happen once to the rootCmd.
func Execute() {
	err := rootCmd.Execute()
	if err != nil {
		os.Exit(1)
	}
}

// check is a utility function to simplify error handling.
// If an error is encountered, it panics, effectively stopping the application and printing the error message.
func check(e error) {
	if e != nil {
		panic(e)
	}
}

// readFile is a utility function that reads the contents of a file specified by the filename parameter.
func readFile(filename string) ([]byte, error) {
	data, err := os.ReadFile(filename)
	return data, err
}

// writeFile is a utility function that writes data (byte slice) to a file specified by the filename parameter.
func writeFile(filename string, data []byte) error {
	err := os.WriteFile(filename, data, 0644)
	return err
}
