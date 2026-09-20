package cmd

import (
	"fmt"
	"github.com/spf13/cobra"
	"os"
	"path/filepath"
)

// decryptCmd represents the decrypt command
var decryptCmd = &cobra.Command{
	Use:   "decrypt",
	Short: "Decrypt a Persona 3 Reload save file.",
	Long: `Decrypt a Persona 3 Reload save file. This will convert an encrypted save file to an unencrypted save file,
allowing it to be used on Xbox/Game Pass.`,
	Run: func(cmd *cobra.Command, args []string) {
		if len(args) < 1 || len(args) > 1 {
			fmt.Println("Usage: P3RCleaner decrypt <filename>")
			os.Exit(1)
		}

		filename := args[0]
		data, err := readFile(filename)
		check(err)

		var processedData []byte
		processedData = decrypt(data, encKey)

		base, ext := filepath.Base(filename), filepath.Ext(filename)
		outFilename := base[:len(base)-len(ext)] + "-decrypted" + ext
		err = writeFile(outFilename, processedData)
		check(err)
	},
}

// init adds the decrypt command to the root command
func init() {
	rootCmd.AddCommand(decryptCmd)
}

// decrypt takes the encrypted data and a string key, returning the decrypted data.
func decrypt(data []byte, key string) []byte {
	var result []byte
	for i, b := range data {
		keyIdx := i % len(key)
		result = append(result, decryptByte(b, key[keyIdx]))
	}
	return result
}

// decryptByte performs the decryption of a single byte using a specified transformation and XOR operation.
func decryptByte(data byte, key byte) byte {
	bVar1 := data ^ key
	return (bVar1>>4&3 | (bVar1&3)<<4 | bVar1&0xcc)
}
