using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ASCOM.LocalServer
{
    public class ConfigStorage
    {
        public static void SaveParametersToFile(string filePath, Dictionary<string, string> parameters)
        {
            try
            {
                // 1. See if the file exists and if not create it
                // The StreamWriter constructor with append: false (default) will create the file if it doesn't exist
                // or overwrite it if it does. If you want to append, set the second parameter to true.
                // Using 'using' statement ensures the StreamWriter is properly disposed and the file is closed,
                // even if an exception occurs.
                using (StreamWriter writer = new StreamWriter(filePath, false)) // 'false' means overwrite if exists, create if not
                {
                    Console.WriteLine($"File '{filePath}' {(File.Exists(filePath) ? "exists. Overwriting..." : "does not exist. Creating...")}");

                    // 2. Save the actual parameters in the file
                    foreach (var param in parameters)
                    {
                        writer.WriteLine($"{param.Key}={param.Value}");
                        Console.WriteLine($"  Saved: {param.Key}={param.Value}");
                    }
                }
                // 3. Close the file - handled automatically by the 'using' statement when the block exits.
                Console.WriteLine($"Parameters successfully saved to '{filePath}'. File closed.");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"An error occurred while saving parameters: {ex.Message}");
            }
        }

        public static Dictionary<string, string> LoadParametersFromFile(string filePath)
        {
            Dictionary<string, string> parameters = new Dictionary<string, string>();
            try
            {
                if (File.Exists(filePath))
                {
                    Console.WriteLine($"Loading parameters from '{filePath}'...");
                    using (StreamReader reader = new StreamReader(filePath))
                    {
                        string line;
                        while ((line = reader.ReadLine()) != null)
                        {
                            string[] parts = line.Split('=');
                            if (parts.Length == 2)
                            {
                                parameters[parts[0]] = parts[1];
                                Console.WriteLine($"  Loaded: {parts[0]}={parts[1]}");
                            }
                        }
                    }
                    Console.WriteLine("Parameters successfully loaded.");
                }
                else
                {
                    Console.WriteLine($"File '{filePath}' does not exist. Cannot load parameters.");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"An error occurred while loading parameters: {ex.Message}");
            }
            return parameters;
        }
    }
}
