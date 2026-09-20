package cmd

import (
	"fmt"
	"os"
	"path/filepath"

	"github.com/spf13/cobra"
)

// encryptCmd represents the encrypt command
var encryptCmd = &cobra.Command{
	Use:   "encrypt",
	Short: "Encrypt a Persona 3 Reload save file.",
	Long: `Encrypt a Persona 3 Reload save file. This will convert an unencrypted save file to an encrypted save file,
allowing it to be used on Steam.`,
	Run: func(cmd *cobra.Command, args []string) {
		if len(args) < 1 || len(args) > 1 {
			fmt.Println("Usage: P3RCleaner encrypt <filename>")
			os.Exit(1)
		}

		filename := args[0]
		data, err := readFile(filename)
		check(err)

		var processedData []byte
		processedData = encrypt(data, encKey)

		base, ext := filepath.Base(filename), filepath.Ext(filename)
		outFilename := base[:len(base)-len(ext)] + "-encrypted" + ext
		err = writeFile(outFilename, processedData)
		check(err)
	},
}

// init function is used to add the encrypt command to the root command.
func init() {
	rootCmd.AddCommand(encryptCmd)
}

// encrypt takes raw data and a string key, returning the encrypted data.
func encrypt(data []byte, key string) []byte {
	var result []byte
	for i, b := range data {
		keyIdx := i % len(key)
		result = append(result, encryptByte(b, key[keyIdx]))
	}
	return result
}

// encryptByte performs the actual byte encryption using a simple transformation and XOR operation.
func encryptByte(data byte, key byte) byte {
	return ((((data&0xff)>>4)&3 | (data&3)<<4 | data&0xcc) ^ key)
}
