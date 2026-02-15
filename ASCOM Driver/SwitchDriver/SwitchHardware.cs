// TODO fill in this information for your driver, then remove this line!
//
// ASCOM Switch hardware class for AstroPowerBoxXXL
//
// Description:	 <To be completed by driver developer>
//
// Implements:	ASCOM Switch interface version: <To be completed by driver developer>
// Author:		(XXX) Your N. Here <your@email.here>

// TODO: Customise the SetConnected and InitialiseHardware methods as needed for your hardware

using ASCOM;
//using ASCOM.Astrometry;
//using ASCOM.Astrometry.AstroUtils;
//using ASCOM.Astrometry.NOVAS;
using ASCOM.DeviceInterface;
using ASCOM.LocalServer;
using ASCOM.Utilities;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using static System.Windows.Forms.AxHost;

namespace ASCOM.AstroPowerBoxXXL.Switch
{
    public class SensorConfig
    {
     public string SensorName { get; set; }
        public bool VoltageVisible { get; set; }
        public bool CurrentVisible { get; set; }
    }
    /// <summary>
    /// ASCOM Switch hardware class for AstroPowerBoxXXL.
    /// </summary>
    [HardwareClass()] // Class attribute flag this as a device hardware class that needs to be disposed by the local server when it exits.
    internal static class SwitchHardware
    {
        // Constants used for Profile persistence
        internal const string comPortProfileName = "COM Port";
        internal const string comPortDefault = "COM1";
        internal const string traceStateProfileName = "Trace Level";
        internal const string traceStateDefault = "true";

        private static string DriverProgId = ""; // ASCOM DeviceID (COM ProgID) for this driver, the value is set by the driver's class initialiser.
        private static string DriverDescription = ""; // The value is set by the driver's class initialiser.
        internal static string comPort; // COM port name (if required)
        public static bool connectedState; // Local server's connected state
        private static bool runOnce = false; // Flag to enable "one-off" activities only to run once.
        internal static Util utilities; // ASCOM Utilities object for use as required
        //internal static AstroUtils astroUtilities; // ASCOM AstroUtilities object for use as required
        internal static TraceLogger tl; // Local server's trace logger object for diagnostic log with information that you specify

        private static List<Guid> uniqueIds = new List<Guid>(); // List of driver instance unique IDs
        private static bool prevquery = false; // Previous query state, used to determine whether the hardware was already prompted


        public static short numDC;// = 7; // Number of DC switches, this is the number of DC switches available in the hardware.
        public static short numRelay;// = 1; // Number of Relays
        public static short numOn;// = 1;// Number of On switches
        public static short numPWM;// = 3; // Number of PWM switches
        public static short numUSB;// = 7; // Number of USB switches
        public static short numRen;// Flag (0 or 1) idicating presence of Environmental sensor
        public static int total;// = numDC + numOn + numPWM + numRelay + numUSB; // Total number of switches, this is the total number of switches available in the hardware, including DC, USB, ADJ, On and PWM switches.
        public static short numSwitch;// = (short)((numDC + numOn + numPWM)*2+total+4); // Number of switches, this is the total number of switches available in the hardware, including DC, USB, ADJ, On and PWM switches.
        public static short numSwitch_visible= 0; // Number of visible switches, this is the total number of switches that are visible to ASCOM clients, including all DC, USB, ADJ, On and PWM switches and the sensors set to Visible in the parameter file.
        

        internal static string[] name = new string[100];
        internal static string[] description = new string[100];
        internal static string[] state = new string[100];
        internal static Switch_type[] type = new Switch_type[100];
        internal static int[] writable = new int[100];
        internal static bool ReverseDC,ReverseRelay,ReverseOn,ReversePWM, ReverseUSB;
        internal static float LimitDC, LimitOn, LimitPWM, LimitTotalDC, LimitTotalPWM, LimitTotal;
        internal static String SSID, PWD;
        internal static String WiFiIP= "Not Connected";
        internal static String LastErrorMessage = "";

        static Form1 clientform;

        private static bool loadclient;

        public static bool loadclient_
        {
            get { return loadclient; }
            set
            {
                loadclient = value;
            }
        }

        private static bool clientloaded;

        public static bool clientloaded_
        {
            get { return clientloaded; }
            set
            {
                clientloaded = value;
            }
        }

        public static bool[] Visible=new bool[100];



        public enum Switch_type
        {
            empty,
            DC,
            USB,
            Relay,
            On,
            PWM,
            Sensor,
            Regul
        }

        private static short Index_Translator(int ind)
        {
            int n = ind + 1;
            int size = numSwitch;
            // If the user requests to find the 0th '1', that's not possible.
            if (n < 0)
            {
                return -1;
            }

            int countOfOnes = 0;

            // Loop through each element of the array
            for (int i = 0; i < size; i++)
            {
                // Check if the current element is a '1'
                if (Visible[i] == true)
                {
                    countOfOnes++;
                    // Check if we have found the nth '1'
                    if (countOfOnes == n)
                    {
                        return (short)i; // Return the current index
                    }
                }
            }
            return -1;
        }


        private static async Task GetValues()
        {
            if (IsConnected)
            {
                try
                {
                    for(short i=0;i<numSwitch; i++) 
                    {
                        GetSwitchValueUSB(i);
                    }
                    GetLastError_USB();
                    await Task.Delay(100);
                }
                catch (Exception ex)
                {
                    MessageBox.Show($"Error retrieving DC voltage: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private static Task RunPeriodicTask(CancellationToken token)
        {
            return Task.Run(async () =>
            {
                while (!token.IsCancellationRequested)
                {
                    await GetValues();
                    try
                    {
                        // Wait for 5 seconds before the next execution
                        await Task.Delay(2000, token);
                    }
                    catch (TaskCanceledException)
                    {
                        // This exception is expected when cancellation is requested
                        Console.WriteLine("Task was cancelled. Stopping.");
                        break;
                    }
                }
            }, token);
        }
        /// <summary>
        /// Initializes a new instance of the device Hardware class.
        /// </summary>
        static SwitchHardware()
        {
            try
            {
                var cts = new CancellationTokenSource();
                Task periodicTask = RunPeriodicTask(cts.Token);
                // Create the hardware trace logger in the static initialiser.
                // All other initialisation should go in the InitialiseHardware method.
                tl = new TraceLogger("", "AstroPowerBoxXXL.Hardware");

                // DriverProgId has to be set here because it used by ReadProfile to get the TraceState flag.
                DriverProgId = Switch.DriverProgId; // Get this device's ProgID so that it can be used to read the Profile configuration values

                // ReadProfile has to go here before anything is written to the log because it loads the TraceLogger enable / disable state.
                ReadProfile(); // Read device configuration from the ASCOM Profile store, including the trace state

                LogMessage("SwitchHardware", $"Static initialiser completed.");
                //var clientForm = new Form1();
                //clientForm.ShowDialog(); // Show the client form, this is just an example, you can remove this line if you do not need a client form.
            }
            catch (Exception ex)
            {
                try { LogMessage("SwitchHardware", $"Initialisation exception: {ex}"); } catch { }
                MessageBox.Show($"SwitchHardware - {ex.Message}\r\n{ex}", $"Exception creating {Switch.DriverProgId}", MessageBoxButtons.OK, MessageBoxIcon.Error);
                throw;
            }
        }

        /// <summary>
        /// Place device initialisation code here
        /// </summary>
        /// <remarks>Called every time a new instance of the driver is created.</remarks>
        internal static void InitialiseHardware()
        {
            // This method will be called every time a new ASCOM client loads your driver
            LogMessage("InitialiseHardware", $"Start.");

            // Add any code that you want to run every time a client connects to your driver here

            // Add any code that you only want to run when the first client connects in the if (runOnce == false) block below
            if (runOnce == false)
            {
                LogMessage("InitialiseHardware", $"Starting one-off initialisation.");

                DriverDescription = Switch.DriverDescription; // Get this device's Chooser description

                LogMessage("InitialiseHardware", $"ProgID: {DriverProgId}, Description: {DriverDescription}");

                connectedState = false; // Initialise connected to false
                utilities = new Util(); //Initialise ASCOM Utilities object
                //astroUtilities = new AstroUtils(); // Initialise ASCOM Astronomy Utilities object

                LogMessage("InitialiseHardware", "Completed basic initialisation");

                // Add your own "one off" device initialisation here e.g. validating existence of hardware and setting up communications
                // If you are using a serial COM port you will find the COM port name selected by the user through the setup dialogue in the comPort variable.

                LogMessage("InitialiseHardware", $"One-off initialisation complete.");
                runOnce = true; // Set the flag to ensure that this code is not run again
            }
        }

        // PUBLIC COM INTERFACE ISwitchV3 IMPLEMENTATION

        #region Common properties and methods.

        /// <summary>
        /// Displays the Setup Dialogue form.
        /// If the user clicks the OK button to dismiss the form, then
        /// the new settings are saved, otherwise the old values are reloaded.
        /// THIS IS THE ONLY PLACE WHERE SHOWING USER INTERFACE IS ALLOWED!
        /// </summary>
        public static void SetupDialog()
        {
            // Don't permit the setup dialogue if already connected
            if (IsConnected)
            {
                MessageBox.Show("Already connected, just press OK");
                return; // Exit the method if already connected
            }

            using (SetupDialogForm F = new SetupDialogForm(tl))
            {
                var result = F.ShowDialog();
                if (result == DialogResult.OK)
                {
                    WriteProfile(); // Persist device configuration values to the ASCOM Profile store
                }
                if(true) // If the loadclient flag is set, then we need to load the client
                {
                    // Load the client form

                    
                }
                else
                {
                    MessageBox.Show("You can now connect to the device.", "Setup complete", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
            }
        }

        /// <summary>Returns the list of custom action names supported by this driver.</summary>
        /// <value>An ArrayList of strings (SafeArray collection) containing the names of supported actions.</value>
        public static ArrayList SupportedActions
        {
            get
            {
                LogMessage("SupportedActions Get", "Returning empty ArrayList");
                return new ArrayList();
            }
        }

        /// <summary>Invokes the specified device-specific custom action.</summary>
        /// <param name="ActionName">A well known name agreed by interested parties that represents the action to be carried out.</param>
        /// <param name="ActionParameters">List of required parameters or an <see cref="String.Empty">Empty String</see> if none are required.</param>
        /// <returns>A string response. The meaning of returned strings is set by the driver author.
        /// <para>Suppose filter wheels start to appear with automatic wheel changers; new actions could be <c>QueryWheels</c> and <c>SelectWheel</c>. The former returning a formatted list
        /// of wheel names and the second taking a wheel name and making the change, returning appropriate values to indicate success or failure.</para>
        /// </returns>
        public static string Action(string actionName, string actionParameters)
        {
             LogMessage("Action", $"Action {actionName}, parameters {actionParameters} is not implemented");
             throw new ActionNotImplementedException("Action " + actionName + " is not implemented by this driver");
        }

        /// <summary>
        /// Transmits an arbitrary string to the device and does not wait for a response.
        /// Optionally, protocol framing characters may be added to the string before transmission.
        /// </summary>
        /// <param name="Command">The literal command string to be transmitted.</param>
        /// <param name="Raw">
        /// if set to <c>true</c> the string is transmitted 'as-is'.
        /// If set to <c>false</c> then protocol framing characters may be added prior to transmission.
        /// </param>
        public static void CommandBlind(string command, bool raw)
        {
            CheckConnected("CommandBlind");
            // TODO The optional CommandBlind method should either be implemented OR throw a MethodNotImplementedException
            // If implemented, CommandBlind must send the supplied command to the mount and return immediately without waiting for a response

            throw new MethodNotImplementedException($"CommandBlind - Command:{command}, Raw: {raw}.");
        }

        /// <summary>
        /// Transmits an arbitrary string to the device and waits for a boolean response.
        /// Optionally, protocol framing characters may be added to the string before transmission.
        /// </summary>
        /// <param name="Command">The literal command string to be transmitted.</param>
        /// <param name="Raw">
        /// if set to <c>true</c> the string is transmitted 'as-is'.
        /// If set to <c>false</c> then protocol framing characters may be added prior to transmission.
        /// </param>
        /// <returns>
        /// Returns the interpreted boolean response received from the device.
        /// </returns>
        public static bool CommandBool(string command, bool raw)
        {
            CheckConnected("CommandBool");
            // TODO The optional CommandBool method should either be implemented OR throw a MethodNotImplementedException
            // If implemented, CommandBool must send the supplied command to the mount, wait for a response and parse this to return a True or False value

            throw new MethodNotImplementedException($"CommandBool - Command:{command}, Raw: {raw}.");
        }

        /// <summary>
        /// Transmits an arbitrary string to the device and waits for a string response.
        /// Optionally, protocol framing characters may be added to the string before transmission.
        /// </summary>
        /// <param name="Command">The literal command string to be transmitted.</param>
        /// <param name="Raw">
        /// if set to <c>true</c> the string is transmitted 'as-is'.
        /// If set to <c>false</c> then protocol framing characters may be added prior to transmission.
        /// </param>
        /// <returns>
        /// Returns the string response received from the device.
        /// </returns>
        public static string CommandString(string command, bool raw)
        {
            CheckConnected("CommandString");
            // TODO The optional CommandString method should either be implemented OR throw a MethodNotImplementedException
            // If implemented, CommandString must send the supplied command to the mount and wait for a response before returning this to the client

            throw new MethodNotImplementedException($"CommandString - Command:{command}, Raw: {raw}.");
        }

        /// <summary>
        /// Deterministically release both managed and unmanaged resources that are used by this class.
        /// </summary>
        /// <remarks>
        /// TODO: Release any managed or unmanaged resources that are used in this class.
        /// 
        /// Do not call this method from the Dispose method in your driver class.
        ///
        /// This is because this hardware class is decorated with the <see cref="HardwareClassAttribute"/> attribute and this Dispose() method will be called 
        /// automatically by the  local server executable when it is irretrievably shutting down. This gives you the opportunity to release managed and unmanaged 
        /// resources in a timely fashion and avoid any time delay between local server close down and garbage collection by the .NET runtime.
        ///
        /// For the same reason, do not call the SharedResources.Dispose() method from this method. Any resources used in the static shared resources class
        /// itself should be released in the SharedResources.Dispose() method as usual. The SharedResources.Dispose() method will be called automatically 
        /// by the local server just before it shuts down.
        /// 
        /// </remarks>
        public static void Dispose()
        {
            try { LogMessage("Dispose", $"Disposing of assets and closing down."); } catch { }

            try
            {
                // Clean up the trace logger and utility objects
                tl.Enabled = false;
                tl.Dispose();
                tl = null;
            }
            catch { }

            try
            {
                utilities.Dispose();
                utilities = null;
            }
            catch { }

            //try
           // {
            //    astroUtilities.Dispose();
           //     astroUtilities = null;
            //}
           // catch { }
        }



        /// <summary>
        /// Synchronously connects to or disconnects from the hardware
        /// </summary>
        /// <param name="uniqueId">Driver's unique ID</param>
        /// <param name="newState">New state: Connected or Disconnected</param>
        public static void SetConnected(Guid uniqueId, bool newState)
        {
            // Check whether we are connecting or disconnecting
            if (newState) // We are connecting
            {
                // Check whether this driver instance has already connected
                if (uniqueIds.Contains(uniqueId)) // Instance already connected
                {
                    // Ignore the request, the unique ID is already in the list
                    LogMessage("SetConnected", $"Ignoring request to connect because the device is already connected.");
                }
                else // Instance not already connected, so connect it
                {
                    // Check whether this is the first connection to the hardware
                    if (uniqueIds.Count == 0) // This is the first connection to the hardware so initiate the hardware connection
                    {
                        SharedResources.SharedSerial.PortName = comPort;
                        SharedResources.SharedSerial.Speed = SerialSpeed.ps115200; // Set the serial port speed to 115200 baud
                        SharedResources.Connected = true;

                        //numSwitch = 2;

                        if (prevquery ==false) {
                        GetNum_USB();
                        //numUSB = 0;
                        total = numDC + numOn + 2*numPWM + numRelay + numUSB; 
                        numSwitch = (short)((numDC + numOn + numPWM) * 2 + total + 4+3*numRen);
                        for (short i = 0; i < numSwitch; i++) { GetSwitchName_USB(i); }
                        for (short i = 0; i < numSwitch; i++) { GetSwitchDescription_USB(i); }
                        for (short i = 0; i < numSwitch; i++) { GetSwitchType_USB(i); }
                        for (short i = 0; i < numSwitch; i++) { CanWrite_USB(i); }
                        for (short i = 0; i < numSwitch; i++) { GetVisible_USB(i); }
                        for (short i = 0; i < 4; i++) { GetReverse_USB(i); }
                        for (short i = 0; i < 6; i++) { GetLimit_USB(i); }
                        GetIP_USB();
                        GetSSID_USB();
                        //GetPWD_USB();

                            short j = 0;
                            for (int i = 0; i < numSwitch; i++)
                            {
                                if (Visible[i] == true) j++;
                            }
                            numSwitch_visible = j;
                         for (short i = 0; i < numSwitch; i++) { state[i]="0"; }
                            connectedState = true;
                            //Getting the parameters from the settings file
                            string fileName = "appsettings.txt";
                            string filePath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, fileName);

                            Dictionary<string, string> loadedParameters = ConfigStorage.LoadParametersFromFile(filePath);

                            prevquery = true; // Set the flag to indicate that we have already queried the hardware

                            Thread uiThread = new Thread(() =>
                            {
                                // This code runs on the new UI thread.

                                // 2. Create the form instance.
                                clientform = new Form1();

                                // 3. Run the message loop.
                                Application.Run(clientform);
                            });
                            uiThread.SetApartmentState(ApartmentState.STA);
                            uiThread.Start();

                        }
                        LogMessage("SetConnected", $"Connecting to hardware.");
                    }
                    else // Other device instances are connected so the hardware is already connected
                    {
                        // Since the hardware is already connected no action is required
                        LogMessage("SetConnected", $"Hardware already connected.");
                    }

                    // The hardware either "already was" or "is now" connected, so add the driver unique ID to the connected list
                    uniqueIds.Add(uniqueId);
                    LogMessage("SetConnected", $"Unique id {uniqueId} added to the connection list.");
                }
            }
            else // We are disconnecting
            {
                // Check whether this driver instance has already disconnected
                if (!uniqueIds.Contains(uniqueId)) // Instance not connected so ignore request
                {
                    // Ignore the request, the unique ID is not in the list
                    LogMessage("SetConnected", $"Ignoring request to disconnect because the device is already disconnected.");
                }
                else // Instance currently connected so disconnect it
                {
                    // Remove the driver unique ID to the connected list
                    uniqueIds.Remove(uniqueId);
                    LogMessage("SetConnected", $"Unique id {uniqueId} removed from the connection list.");

                    // Check whether there are now any connected driver instances 
                    if (uniqueIds.Count == 0) // There are no connected driver instances so disconnect from the hardware
                    {
                        connectedState = false;
                        SharedResources.Connected = false;
                        prevquery = false;
                        clientform.Invoke(new Action(() => clientform.Close()));
                    }
                    else // Other device instances are connected so do not disconnect the hardware
                    {
                        // No action is required
                        LogMessage("SetConnected", $"Hardware already connected.");
                    }
                }
            }

            // Log the current connected state
            LogMessage("SetConnected", $"Currently connected driver ids:");
            foreach (Guid id in uniqueIds)
            {
                LogMessage("SetConnected", $" ID {id} is connected");
            }
        }

        /// <summary>
        /// Returns a description of the device, such as manufacturer and model number. Any ASCII characters may be used.
        /// </summary>
        /// <value>The description.</value>
        public static string Description
        {
            // TODO customise this device description if required
            get
            {
                LogMessage("Description Get", DriverDescription);
                return DriverDescription;
            }
        }

        /// <summary>
        /// Descriptive and version information about this ASCOM driver.
        /// </summary>
        public static string DriverInfo
        {
            get
            {
                Version version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
                // TODO customise this driver description if required
                string driverInfo = $"Information about the driver itself. Version: {version.Major}.{version.Minor}";
                LogMessage("DriverInfo Get", driverInfo);
                return driverInfo;
            }
        }

        /// <summary>
        /// A string containing only the major and minor version of the driver formatted as 'm.n'.
        /// </summary>
        public static string DriverVersion
        {
            get
            {
                Version version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
                string driverVersion = $"{version.Major}.{version.Minor}";
                LogMessage("DriverVersion Get", driverVersion);
                return driverVersion;
            }
        }

        /// <summary>
        /// The interface version number that this device supports.
        /// </summary>
        public static short InterfaceVersion
        {
            // set by the driver wizard
            get
            {
                LogMessage("InterfaceVersion Get", "3");
                return Convert.ToInt16("3");
            }
        }

        /// <summary>
        /// The short name of the driver, for display purposes
        /// </summary>
        public static string Name
        {
            // TODO customise this device name as required
            get
            {
                string name = "OpenPowerBoxXXL";
                LogMessage("Name Get", name);
                return name;
            }
        }


        #endregion


        #region ISwitch Implementation

        internal static void GetNum_USB()
        {
            SharedResources.SharedSerial.Transmit($"# Z{Environment.NewLine}");
            string buf = SharedResources.SharedSerial.ReceiveTerminated(";");
            buf = buf.Substring(buf.IndexOf('#') + 1);
            buf = buf.Substring(buf.IndexOf(':') + 1);
            buf = buf.Remove(buf.Length - 1);
            numDC= short.Parse(buf.Split(',')[0]);
            numPWM = short.Parse(buf.Split(',')[1]);
            numRelay = short.Parse(buf.Split(',')[2]);
            numOn = short.Parse(buf.Split(',')[3]);
            numUSB = short.Parse(buf.Split(',')[4]);
            numRen = short.Parse(buf.Split(',')[5]);
            return;
        }
        internal static void GetIP_USB()
        {
            string answer;
            SharedResources.SharedSerial.Transmit($"# I{Environment.NewLine}");
            string buf = SharedResources.SharedSerial.ReceiveTerminated(";");
            buf = buf.Substring(buf.IndexOf('#') + 1);
            buf = buf.Substring(buf.IndexOf(':') + 1);
            answer = buf.Remove(buf.Length-1);
            WiFiIP  = answer; 
            return;
        }

        internal static void GetSSID_USB()
        {
            string answer;
            SharedResources.SharedSerial.Transmit($"# f{Environment.NewLine}");
            string buf = SharedResources.SharedSerial.ReceiveTerminated(";");
            buf = buf.Substring(buf.IndexOf('#') + 1);
            buf = buf.Substring(buf.IndexOf(':') + 1);
            answer = buf.Remove(buf.Length - 1);
            SSID = answer;
            return;
        }

        internal static void SetSSID_USB(String ssid)
        {
            int sep;
            string answer = "";
            string a, b;

            a = "# F ";
            a = a + "0";
            a = a + " ";
            a = a + ssid;
            a = a + Environment.NewLine;
            SharedResources.SharedSerial.Transmit(a);
            answer = SharedResources.SharedSerial.ReceiveTerminated(";");
            answer = answer.Substring(answer.IndexOf('#') + 1);
            sep = answer.IndexOf(':');
            a = answer.Substring(1, sep - 1);

            if ((answer[0] == 'f') && (short.Parse(a) == 0))
            {

                b = answer.Substring(sep + 1);
                SSID = b.Remove(b.Length - 1);
            }
            return;
        }

        internal static void Restart_USB()
        {
            string a;
            a = "# p";
            a = a + Environment.NewLine;
            SharedResources.SharedSerial.Transmit(a);
            return;
        }

        //internal static void GetPWD_USB()
        //{
        //    string answer;
        //    SharedResources.SharedSerial.Transmit($"# h{Environment.NewLine}");
        //    string buf = SharedResources.SharedSerial.ReceiveTerminated(";");
        //    buf = buf.Substring(buf.IndexOf('#') + 1);
        //    buf = buf.Substring(buf.IndexOf(':') + 1);
        //    answer = buf.Remove(buf.Length - 1);
        //    PWD = answer;
        //    return;
        //}
        internal static void SetPWD_USB(String pwd)
        {
            int sep;
            string answer = "";
            string a, b;

            a = "# H ";
            a = a + "0";
            a = a + " ";
            a = a + pwd;
            a = a + Environment.NewLine;
            SharedResources.SharedSerial.Transmit(a);
            answer = SharedResources.SharedSerial.ReceiveTerminated(";");
            answer = answer.Substring(answer.IndexOf('#') + 1);
            sep = answer.IndexOf(':');
            a = answer.Substring(1, sep - 1);

            if ((answer[0] == 'h') && (short.Parse(a) == 0))
            {

                b = answer.Substring(sep + 1);
                //PWD = b.Remove(b.Length - 1);
            }
            return;
        }

        /// <summary>
        /// The number of switches managed by this driver
        /// </summary>
        /// <returns>The number of devices managed by this driver.</returns>
        internal static short MaxSwitch
        {
            get
            {
                LogMessage("MaxSwitch Get", numSwitch_visible.ToString());
                return numSwitch_visible ;
            }
        }

        internal static void MaxSwitch_USB()
        {
            string answer;
            SharedResources.SharedSerial.Transmit($"# X{Environment.NewLine}");
            string buf = SharedResources.SharedSerial.ReceiveTerminated(";");
            buf = buf.Substring(buf.IndexOf('#') + 1);
            buf = buf.Substring(buf.IndexOf(':') + 1);
            answer = buf.Remove(buf.Length-1);
            numSwitch_visible  = short.Parse(answer); 
            return;
        }

        internal static void GetSwitchName_USB(short id)
        {
            int sep;
            string answer = "";
            string cmd = "# n ";
            string a,b;

            cmd = string.Concat(cmd, id.ToString(), Environment.NewLine);
            SharedResources.SharedSerial.Transmit(cmd);
            answer = SharedResources.SharedSerial.ReceiveTerminated(";");
            answer = answer.Substring(answer.IndexOf('#') + 1);
            sep = answer.IndexOf(':');
            a= answer.Substring(1, sep-1);

            if ((answer[0] == 'n') && (short.Parse(a) == id))
            {

                b = answer.Substring(sep + 1);
                name[id] = b.Remove(b.Length - 1);
            }
            return;
        }

        /// <summary>
        /// Return the name of switch device n.
        /// </summary>
        /// <param name="id">The device number (0 to <see cref="MaxSwitch"/> - 1)</param>
        /// <returns>The name of the device</returns>
        internal static string GetSwitchName(short ind)
        {
            short id = Index_Translator(ind);

            Validate("GetSwitchName", id);
            GetSwitchName_USB(id);
            return name[id];

        }

        internal static void GetSwitchType_USB(short id)
        {
            int sep;
            string answer = "";
            string cmd = "# T ";
            string a, b;
            int t=-1;


            cmd = string.Concat(cmd, id.ToString(), Environment.NewLine);
            SharedResources.SharedSerial.Transmit(cmd);
            answer = SharedResources.SharedSerial.ReceiveTerminated(";");
            answer = answer.Substring(answer.IndexOf('#') + 1);
            sep = answer.IndexOf(':');
            a = answer.Substring(1, sep - 1);

            if ((answer[0] == 'T') && (short.Parse(a) == id))
            {

                b = answer.Substring(sep + 1);
                t = int.Parse(b.Remove(b.Length - 1));


                switch (t)
                {
                    case -1: type[id] = Switch_type.empty; break;
                    case 0: type[id] = Switch_type.DC; break;
                    case 3: type[id] = Switch_type.Relay; break;
                    case 2: type[id] = Switch_type.On; break;
                    case 1: type[id] = Switch_type.PWM; break;
                    case 4: type[id] = Switch_type.USB; break;
                    case 5: type[id] = Switch_type.Sensor; break;

                }
            }
            return ;
        }

        internal static Switch_type GetSwitchType(short id)
        {
            return type[id];
        }

        /// <summary>
        /// Set a switch device name to a specified value.
        /// </summary>
        /// <param name="id">The device number (0 to <see cref="MaxSwitch"/> - 1)</param>
        /// <param name="name">The name of the device</param>
        internal static void SetSwitchName(short ind, string name)
        {
            short id = Index_Translator(ind);
            Validate("SetSwitchName", id);
            SetSwitchName_USB(ind, name);
        }

        internal static void SetSwitchName_USB(short ind, string Name)
        {
            int sep;
            string answer = "";
            string a, b;

            a = "# N ";
            a = a + ind.ToString();
            a = a + " ";
            a = a + Name;
            a = a + Environment.NewLine;
            SharedResources.SharedSerial.Transmit(a);
            answer = SharedResources.SharedSerial.ReceiveTerminated(";");
            answer = answer.Substring(answer.IndexOf('#') + 1);
            sep = answer.IndexOf(':');
            a = answer.Substring(1, sep - 1);

            if ((answer[0] == 'n') && (short.Parse(a) == ind))
            {

                b = answer.Substring(sep + 1);
                name[ind] = b.Remove(b.Length - 1);
            }
            return;
        }



        internal static string GetSwitchDescription_USB(short id)
        {
            int sep;
            string answer = "";
            string cmd = "# D ";
            string a, b;

            cmd = string.Concat(cmd, id.ToString(), Environment.NewLine);
            SharedResources.SharedSerial.Transmit(cmd);
            answer = SharedResources.SharedSerial.ReceiveTerminated(";");
            answer = answer.Substring(answer.IndexOf('#') + 1);
            sep = answer.IndexOf(':');
            a = answer.Substring(1, sep - 1);

            if ((answer[0] == 'D') && (short.Parse(a) == id))
            {

                b = answer.Substring(sep + 1);
                description[id] = b.Remove(b.Length - 1);
            }

            return description[id];
        }
        internal static void GetReverse_USB(short id)
        {
            int sep;
            string answer = "";
            string cmd = "# r ";
            string a, b;

            cmd = string.Concat(cmd, id.ToString(), Environment.NewLine);
            SharedResources.SharedSerial.Transmit(cmd);
            answer = SharedResources.SharedSerial.ReceiveTerminated(";");
            answer = answer.Substring(answer.IndexOf('#') + 1);
            sep = answer.IndexOf(':');
            a = answer.Substring(1, sep - 1);

            if ((answer[0] == 'r') && (short.Parse(a) == id))
            {

                b = answer.Substring(sep + 1);
                b = b.Remove(b.Length - 1);
                if(id==0) ReverseDC = Convert.ToBoolean(int.Parse(b));
                else if (id == 4) ReverseUSB = Convert.ToBoolean(int.Parse(b));
                else if (id == 3) ReverseRelay = Convert.ToBoolean(int.Parse(b));
                else if (id == 2) ReverseOn = Convert.ToBoolean(int.Parse(b));
                else if (id == 1) ReversePWM = Convert.ToBoolean(int.Parse(b));
                
            }
        }
        internal static void SetReverse_USB(short id, bool rev)
        {
            int sep;
            string answer = "";
            string a, b;

            Int32 etat = Convert.ToInt32(rev);
            a = "# R ";
            a = a + id.ToString();
            a = a + " ";
            a = a + etat.ToString();
            a = a + Environment.NewLine;
            SharedResources.SharedSerial.Transmit(a);

            answer = SharedResources.SharedSerial.ReceiveTerminated(";");
            answer = answer.Substring(answer.IndexOf('#') + 1);
            sep = answer.IndexOf(':');
            a = answer.Substring(1, sep - 1);

            if ((answer[0] == 'r') && (short.Parse(a) == id))
            {

                b = answer.Substring(sep + 1);
                b = b.Remove(b.Length - 1);
                if (id == 0) ReverseDC = Convert.ToBoolean(int.Parse(b));
                else if (id == 4) ReverseUSB = Convert.ToBoolean(int.Parse(b));
                else if (id == 3) ReverseRelay = Convert.ToBoolean(int.Parse(b));
                else if (id == 2) ReverseOn = Convert.ToBoolean(int.Parse(b));
                else if (id == 1) ReversePWM = Convert.ToBoolean(int.Parse(b));
            }
        }

        internal static void GetLimit_USB(short id)
        {
            int sep;
            string answer = "";
            string cmd = "# l ";
            string a, b;

            cmd = string.Concat(cmd, id.ToString(), Environment.NewLine);
            SharedResources.SharedSerial.Transmit(cmd);
            answer = SharedResources.SharedSerial.ReceiveTerminated(";");
            answer = answer.Substring(answer.IndexOf('#') + 1);
            sep = answer.IndexOf(':');
            a = answer.Substring(1, sep - 1);

            if ((answer[0] == 'l') && (short.Parse(a) == id))
            {
                b = answer.Substring(sep + 1);
                b = b.Remove(b.Length - 1);
                if (id == 0) LimitDC = float.Parse(b, CultureInfo.InvariantCulture);
                else if (id == 2) LimitOn = float.Parse(b, CultureInfo.InvariantCulture);
                else if (id == 1) LimitPWM = float.Parse(b, CultureInfo.InvariantCulture);
                else if (id == 3) LimitTotalDC= float.Parse(b, CultureInfo.InvariantCulture);
                else if (id == 4) LimitTotalPWM = float.Parse(b, CultureInfo.InvariantCulture);
                else if (id == 5) LimitTotal = float.Parse(b, CultureInfo.InvariantCulture);

            }
        }

        internal static void SetLimit_USB(short id, String lim)
        {
            int sep;
            string answer = "";
            string a, b;


            a = "# L ";
            a = a + id.ToString();
            a = a + " ";
            a = a + lim;
            a = a + Environment.NewLine;
            SharedResources.SharedSerial.Transmit(a);

            answer = SharedResources.SharedSerial.ReceiveTerminated(";");
            answer = answer.Substring(answer.IndexOf('#') + 1);
            sep = answer.IndexOf(':');
            a = answer.Substring(1, sep - 1);

            if ((answer[0] == 'l') && (short.Parse(a) == id))
            {
                b = answer.Substring(sep + 1);
                b = b.Remove(b.Length - 1);
                if (id == 0) LimitDC = float.Parse(b, CultureInfo.InvariantCulture);
                else if (id == 2) LimitOn = float.Parse(b, CultureInfo.InvariantCulture);
                else if (id == 1) LimitPWM = float.Parse(b, CultureInfo.InvariantCulture);
                else if (id == 3) LimitTotalDC = float.Parse(b, CultureInfo.InvariantCulture);
                else if (id == 4) LimitTotalPWM = float.Parse(b, CultureInfo.InvariantCulture);
                else if (id == 5) LimitTotal = float.Parse(b, CultureInfo.InvariantCulture);
            }
        }

        internal static void GetVisible_USB(short id)
        {
            int sep;
            string answer = "";
            string cmd = "# y ";
            string a, b;

            cmd = string.Concat(cmd, id.ToString(), Environment.NewLine);
            SharedResources.SharedSerial.Transmit(cmd);
            answer = SharedResources.SharedSerial.ReceiveTerminated(";");
            answer = answer.Substring(answer.IndexOf('#') + 1);
            sep = answer.IndexOf(':');
            a = answer.Substring(1, sep - 1);

            if ((answer[0] == 'y') && (short.Parse(a) == id))
            {

                b = answer.Substring(sep + 1);
                b = b.Remove(b.Length - 1);
                Visible[id] = Convert.ToBoolean(int.Parse(b));
            }
        }

    /*    internal static void SetVisible_USB(short id, bool vis)
        {
            int sep;
            string answer = "";
            string a, b;

            Int32 etat = Convert.ToInt32(vis);
            a = "# Y ";
            a = a + id.ToString();
            a = a + " ";
            a = a + etat.ToString();
            a = a + Environment.NewLine;
            SharedResources.SharedSerial.Transmit(a);

            answer = SharedResources.SharedSerial.ReceiveTerminated(";");
            answer = answer.Substring(answer.IndexOf('#') + 1);
            sep = answer.IndexOf(':');
            a = answer.Substring(1, sep - 1);

            if ((answer[0] == 'Y') && (short.Parse(a) == id))
            {

                b = answer.Substring(sep + 1);
                Visible[id] = bool.Parse(b.Remove(b.Length - 1));
            }
        }*/

        /// <summary>
        /// Gets the description of the specified switch device. This is to allow a fuller description of
        /// the device to be returned, for example for a tool tip.
        /// </summary>
        /// <param name="id">The device number (0 to <see cref="MaxSwitch"/> - 1)</param>
        /// <returns>
        /// String giving the device description.
        /// </returns>
        internal static string GetSwitchDescription(short ind)
        {
            short id = Index_Translator(ind);
            Validate("GetSwitchDescription", id);
            LogMessage("GetSwitchDescription", $"GetSwitchDescription({id}) - not implemented");
            return description[id];
        }

        /// <summary>
        /// Reports if the specified switch device can be written to, default true.
        /// This is false if the device cannot be written to, for example a limit switch or a sensor.
        /// </summary>
        /// <param name="id">The device number (0 to <see cref="MaxSwitch"/> - 1)</param>
        /// <returns>
        /// <c>true</c> if the device can be written to, otherwise <c>false</c>.
        /// </returns>
        /// 
        internal static void CanWrite_USB(short id)
        {

            int sep;
            string answer = "";
            string cmd = "# W ";
            string a, b;

            cmd = string.Concat(cmd, id.ToString(), Environment.NewLine);
            SharedResources.SharedSerial.Transmit(cmd);
            answer = SharedResources.SharedSerial.ReceiveTerminated(";");
            answer = answer.Substring(answer.IndexOf('#') + 1);
            sep = answer.IndexOf(':');
            a = answer.Substring(1, sep - 1);

            if ((answer[0] == 'W') && (short.Parse(a) == id))
            {

                b = answer.Substring(sep + 1);
                writable[id] = int.Parse(b.Remove(b.Length - 1));
            }
            return ;
        }
        internal static bool CanWrite(short ind)
        {
            short id = Index_Translator(ind);
            //bool writable = true;
            Validate("CanWrite", id);
            // default behavour is to report true
            LogMessage("CanWrite", $"CanWrite({id}): {writable}");
            return Convert.ToBoolean(writable[id]);
        }

        #region Boolean switch members

        /// <summary>
        /// Return the state of switch device id as a boolean
        /// </summary>
        /// <param name="id">The device number (0 to <see cref="MaxSwitch"/> - 1)</param>
        /// <returns>True or false</returns>
        /// 

        internal static void GetSwitchUSB(short id)
        {

            Validate("GetSwitch", id);
            int sep;
            string answer = "";
            string cmd = "# G ";
            string a, b;
            bool value = false;

            cmd = string.Concat(cmd, id.ToString(), Environment.NewLine);
            SharedResources.SharedSerial.Transmit(cmd);
            answer = SharedResources.SharedSerial.ReceiveTerminated(";");
            answer = answer.Substring(answer.IndexOf('#') + 1);
            sep = answer.IndexOf(':');
            a = answer.Substring(1, sep - 1);

            if ((answer[0] == 'G') && (short.Parse(a) == id))
            {
                b = answer.Substring(sep + 1);
                state[id] = b.Remove(b.Length - 1);
                value = bool.Parse(state[id]);
            }
        }
        internal static bool GetSwitch(short ind)
        {
            short id = Index_Translator(ind);
            Validate("GetSwitch", id);
            bool value = bool.Parse(state[id]);
            return value;
        }

        /// <summary>
        /// Sets a switch controller device to the specified state, true or false.
        /// </summary>
        /// <param name="id">The device number (0 to <see cref="MaxSwitch"/> - 1)</param>
        /// <param name="state">The required control state</param>
        internal static void SetSwitchUSB(short id, bool _state)
        {
            Validate("SetSwitch", id);
            if (CanWrite(id))
            {
                Int32 etat = Convert.ToInt32(_state);
                string a = "# S ";
                a = a + id.ToString();
                a = a + " ";
                a = a + etat.ToString();
                a = a + Environment.NewLine;
                SharedResources.SharedSerial.Transmit(a);

                int sep;
                string answer = "";
                string  b;
                bool value = false;

                answer = SharedResources.SharedSerial.ReceiveTerminated(";");
                answer = answer.Substring(answer.IndexOf('#') + 1);
                sep = answer.IndexOf(':');
                a = answer.Substring(1, sep - 1);

                if ((answer[0] == 'G') && (short.Parse(a) == id))
                {
                    b = answer.Substring(sep + 1);
                    state[id] = b.Remove(b.Length - 1);
                    value = bool.Parse(state[id]);
                }
            }
        }
        internal static void SetSwitch(short ind, bool _state)
        {
            short id = Index_Translator(ind);
            //Validate("SetSwitch", id);
            SetSwitchUSB(id, _state);
        }
        #endregion

        #region Analogue members

        /// <summary>
        /// Returns the maximum value for this switch device, this must be greater than <see cref="MinSwitchValue"/>.
        /// </summary>
        /// <param name="id">The device number (0 to <see cref="MaxSwitch"/> - 1)</param>
        /// <returns>The maximum value to which this device can be set or which a read only sensor will return.</returns>
        internal static double MaxSwitchValue(short ind)
        {
            double a = 1.0;
            short id = Index_Translator(ind);

            Validate("MaxSwitchValue", id);
            if (type[id] == Switch_type.PWM)
            { 
                if (id < numDC+numPWM) a = 100.0;
                else a = 1.0;
            }
            return a;
        }

        /// <summary>
        /// Returns the minimum value for this switch device, this must be less than <see cref="MaxSwitchValue"/>
        /// </summary>
        /// <param name="id">The device number (0 to <see cref="MaxSwitch"/> - 1)</param>
        /// <returns>The minimum value to which this device can be set or which a read only sensor will return.</returns>
        internal static double MinSwitchValue(short ind)
        {
            short id = Index_Translator(ind);
            Validate("MinSwitchValue", id);
            return 0.0;
        }

        /// <summary>
        /// Returns the step size that this device supports (the difference between successive values of the device).
        /// </summary>
        /// <param name="id">The device number (0 to <see cref="MaxSwitch"/> - 1)</param>
        /// <returns>The step size for this device.</returns>
        internal static double SwitchStep(short ind)
        {
            return 1.0;
        }

        /// <summary>
        /// Returns the value for switch device id as a double
        /// </summary>
        /// <param name="id">The device number (0 to <see cref="MaxSwitch"/> - 1)</param>
        /// <returns>The value for this switch, this is expected to be between <see cref="MinSwitchValue"/> and
        /// <see cref="MaxSwitchValue"/>.</returns>
        internal static void GetSwitchValueUSB(short id)
        {
            //short id = Index_Translator(ind);
            Validate("GetSwitchValue", id);

            List<Guid> uniques = uniqueIds;
            
            int sep;
            string answer = "";
            string cmd = "# G ";
            string a, b;
            double value = 0.0;
            //short id = idex_trans(id_vis);
            //            if (type[id] == Switch_type.DC || type[id] == Switch_type.USB || type[id] == Switch_type.ADJ || type[id] == Switch_type.On || type[id] == Switch_type.Regul)
            //            {

            cmd = string.Concat(cmd, id.ToString(), Environment.NewLine);
                SharedResources.SharedSerial.Transmit(cmd);
                answer = SharedResources.SharedSerial.ReceiveTerminated(";");
                answer = answer.Substring(answer.IndexOf('#') + 1);
                sep = answer.IndexOf(':');
                a = answer.Substring(1, sep - 1);

                if ((answer[0] == 'G') && (short.Parse(a) == id))
                {
                    b = answer.Substring(sep + 1);
                    state[id] = b.Remove(b.Length - 1);
                    state[id].Replace('.', ','); // Ensure decimal point is correct for parsing
                    value = double.Parse(state[id], CultureInfo.InvariantCulture);
                }
        }

        internal static double GetSwitchValue(short ind)
        {
            short id = Index_Translator(ind);
            double value = double.Parse(state[id], CultureInfo.InvariantCulture);
            return value;
        }

        /// <summary>
        /// Set the value for this device as a double.
        /// </summary>
        /// <param name="id">The device number (0 to <see cref="MaxSwitch"/> - 1)</param>
        /// <param name="value">The value to be set, between <see cref="MinSwitchValue"/> and <see cref="MaxSwitchValue"/></param>
        internal static void SetSwitchValueUSB(short id, double _value)
        {
            Validate("SetSwitchValue", id, _value);
            if (!CanWrite(id))
            {
                LogMessage("SetSwitchValue", $"SetSwitchValue({id}) - Cannot write");
                throw new ASCOM.MethodNotImplementedException($"SetSwitchValue({id}) - Cannot write");
            }
            else
            {
                string a = "# S ";
                a = a + id.ToString();
                a = a + " ";
                a = a + _value.ToString();
                a = a + Environment.NewLine;
                SharedResources.SharedSerial.Transmit(a);

                int sep;
                string answer = "";
                string cmd = "# G ";
                string b;
                double value = 0.0;


                answer = SharedResources.SharedSerial.ReceiveTerminated(";");
                answer = answer.Substring(answer.IndexOf('#') + 1);
                sep = answer.IndexOf(':');
                a = answer.Substring(1, sep - 1);

                if ((answer[0] == 'G') && (short.Parse(a) == id))
                {
                    b = answer.Substring(sep + 1);
                    state[id] = b.Remove(b.Length - 1);
                    state[id].Replace('.', ','); // Ensure decimal point is correct for parsing
                    value = double.Parse(state[id], CultureInfo.InvariantCulture);
                }

            }

        }

        internal static void SetSwitchValue(short ind, double _value)
        {
            short id = Index_Translator(ind);
            SetSwitchValueUSB(id, _value);

        }

        internal static void GetLastError_USB()
        {
            int sep;
            string answer = "";
            string cmd = "# e ";
            string a, b;
            double value = 0.0;


            cmd = string.Concat(cmd, "0", Environment.NewLine);
            SharedResources.SharedSerial.Transmit(cmd);
            answer = SharedResources.SharedSerial.ReceiveTerminated(";");
            answer = answer.Substring(answer.IndexOf('#') + 1);
            sep = answer.IndexOf(':');
            a = answer.Substring(1, sep - 1);

            if (answer[0] == 'e')
            {
                b = answer.Substring(sep + 1);
                b = b.Remove(b.Length - 1);
                if(b != LastErrorMessage) LogMessage("New Error", b);
                LastErrorMessage = b;
                
            }
        }


        #endregion

        #region Async members

        /// <summary>
        /// Set a boolean switch's state asynchronously
        /// </summary>
        /// <exception cref="MethodNotImplementedException">When CanAsync(id) is false.</exception>
        /// <param name="id">Switch number.</param>
        /// <param name="state">New boolean state.</param>
        /// <remarks>
        /// <p style="color:red"><b>This is an optional method and can throw a <see cref="MethodNotImplementedException"/> when <see cref="CanAsync(short)"/> is <see langword="false"/>.</b></p>
        /// </remarks>
        public static void SetAsync(short id, bool state)
        {
            Validate("SetAsync", id);
            if (!CanAsync(id))
            {
                var message = $"SetAsync({id}) - Switch cannot operate asynchronously";
                LogMessage("SetAsync", message);
                throw new MethodNotImplementedException(message);
            }

            // Implement async support here if required
            LogMessage("SetAsync", $"SetAsync({id}) = {state} - not implemented");
            throw new MethodNotImplementedException("SetAsync");
        }

        /// <summary>
        /// Set a switch's value asynchronously
        /// </summary>
        /// <param name="id">Switch number.</param>
        /// <param name="value">New double value.</param>
        /// <p style="color:red"><b>This is an optional method and can throw a <see cref="MethodNotImplementedException"/> when <see cref="CanAsync(short)"/> is <see langword="false"/>.</b></p>
        /// <exception cref="MethodNotImplementedException">When CanAsync(id) is false.</exception>
        /// <remarks>
        /// <p style="color:red"><b>This is an optional method and can throw a <see cref="MethodNotImplementedException"/> when <see cref="CanAsync(short)"/> is <see langword="false"/>.</b></p>
        /// </remarks>
        public static void SetAsyncValue(short id, double value)
        {
            Validate("SetSwitchValue", id, value);
            if (!CanWrite(id))
            {
                LogMessage("SetSwitchValue", $"SetSwitchValue({id}) - Cannot write");
                throw new ASCOM.MethodNotImplementedException($"SetSwitchValue({id}) - Cannot write");
            }

            // Implement async support here if required
            LogMessage("SetSwitchValue", $"SetSwitchValue({id}) = {value} - not implemented");
            throw new MethodNotImplementedException("SetSwitchValue");
        }

        /// <summary>
        /// Flag indicating whether this switch can operate asynchronously.
        /// </summary>
        /// <param name="id">Switch number.</param>
        /// <returns>True if the switch can operate asynchronously.</returns>
        /// <exception cref="MethodNotImplementedException">When CanAsync(id) is false.</exception>
        /// <remarks>
        /// <p style="color:red"><b>This is a mandatory method and must not throw a <see cref="MethodNotImplementedException"/>.</b></p>
        /// </remarks>
        public static bool CanAsync(short id)
        {
            const bool ASYNC_SUPPORT_DEFAULT = false;

            Validate("CanAsync", id);

            // Default behaviour is not to support async operation
            LogMessage("CanAsync", $"CanAsync({id}): {ASYNC_SUPPORT_DEFAULT}");
            return ASYNC_SUPPORT_DEFAULT;
        }

        /// <summary>
        /// Completion variable for asynchronous switch state change operations.
        /// </summary>
        /// <param name="id">Switch number.</param>
        /// <exception cref="OperationCancelledException">When an in-progress operation is cancelled by the <see cref="CancelAsync(short)"/> method.</exception>
        /// <returns>False while an asynchronous operation is underway and true when it has completed.</returns>
        /// <remarks>
        /// <p style="color:red"><b>This is a mandatory method and must not throw a <see cref="MethodNotImplementedException"/>.</b></p>
        /// </remarks>
        public static bool StateChangeComplete(short id)
        {
            const bool STATE_CHANGE_COMPLETE_DEFAULT = true;

            Validate("StateChangeComplete", id);
            LogMessage("StateChangeComplete", $"StateChangeComplete({id}) - Returning {STATE_CHANGE_COMPLETE_DEFAULT}");
            return STATE_CHANGE_COMPLETE_DEFAULT;
        }

        /// <summary>
        /// Cancels an in-progress asynchronous state change operation.
        /// </summary>
        /// <param name="id">Switch number.</param>
        /// <exception cref="MethodNotImplementedException">When it is not possible to cancel an asynchronous change.</exception>
        /// <remarks>
        /// <p style="color:red"><b>This is an optional method and can throw a <see cref="MethodNotImplementedException"/>.</b></p>
        /// This method must be implemented if it is possible for the device to cancel an asynchronous state change operation, otherwise it must throw a <see cref="MethodNotImplementedException"/>.
        /// </remarks>
        public static void CancelAsync(short id)
        {
            Validate("CancelAsync", id);
            LogMessage("CancelAsync", $"CancelAsync({id}) - not implemented");
            throw new MethodNotImplementedException("CancelAsync");
        }

        #endregion

        #endregion

        #region Private methods

        /// <summary>
        /// Checks that the switch id is in range and throws an InvalidValueException if it isn't
        /// </summary>
        /// <param name="message">The message.</param>
        /// <param name="id">The id.</param>
        private static void Validate(string message, short id)
        {
            if (id < 0 || id >= numSwitch )
            {
                LogMessage(message, string.Format("Switch {0} not available, range is 0 to {1}", id, numSwitch - 1));
                throw new InvalidValueException(message, id.ToString(), string.Format("0 to {0}", numSwitch - 1));
            }
        }

        /// <summary>
        /// Checks that the switch id and value are in range and throws an
        /// InvalidValueException if they are not.
        /// </summary>
        /// <param name="message">The message.</param>
        /// <param name="id">The id.</param>
        /// <param name="value">The value.</param>
        private static void Validate(string message, short id, double value)
        {
            Validate(message, id);
            var min = MinSwitchValue(id);
            var max = MaxSwitchValue(id);
            if (value < min || value > max)
            {
                LogMessage(message, string.Format("Value {1} for Switch {0} is out of the allowed range {2} to {3}", id, value, min, max));
                throw new InvalidValueException(message, value.ToString(), string.Format("Switch({0}) range {1} to {2}", id, min, max));
            }
        }

        #endregion

        #region Private properties and methods
        // Useful methods that can be used as required to help with driver development

        /// <summary>
        /// Returns true if there is a valid connection to the driver hardware
        /// </summary>
        private static bool IsConnected
        {
            get
            {
                // TODO check that the driver hardware connection exists and is connected to the hardware
                return connectedState;
            }
        }

        /// <summary>
        /// Use this function to throw an exception if we aren't connected to the hardware
        /// </summary>
        /// <param name="message"></param>
        private static void CheckConnected(string message)
        {
            if (!IsConnected)
            {
                throw new NotConnectedException(message);
            }
        }

        /// <summary>
        /// Read the device configuration from the ASCOM Profile store
        /// </summary>
        internal static void ReadProfile()
        {
            using (Profile driverProfile = new Profile())
            {
                driverProfile.DeviceType = "Switch";
                tl.Enabled = Convert.ToBoolean(driverProfile.GetValue(DriverProgId, traceStateProfileName, string.Empty, traceStateDefault));
                comPort = driverProfile.GetValue(DriverProgId, comPortProfileName, string.Empty, comPortDefault);
            }
        }

        /// <summary>
        /// Write the device configuration to the  ASCOM  Profile store
        /// </summary>
        internal static void WriteProfile()
        {
            using (Profile driverProfile = new Profile())
            {
                driverProfile.DeviceType = "Switch";
                driverProfile.WriteValue(DriverProgId, traceStateProfileName, tl.Enabled.ToString());
                driverProfile.WriteValue(DriverProgId, comPortProfileName, comPort.ToString());
            }
        }

        /// <summary>
        /// Log helper function that takes identifier and message strings
        /// </summary>
        /// <param name="identifier"></param>
        /// <param name="message"></param>
        internal static void LogMessage(string identifier, string message)
        {
            tl.LogMessageCrLf(identifier, message);
        }

        /// <summary>
        /// Log helper function that takes formatted strings and arguments
        /// </summary>
        /// <param name="identifier"></param>
        /// <param name="message"></param>
        /// <param name="args"></param>
        internal static void LogMessage(string identifier, string message, params object[] args)
        {
            var msg = string.Format(message, args);
            LogMessage(identifier, msg);
        }
        #endregion
    }
}

