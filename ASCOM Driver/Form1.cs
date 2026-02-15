using MetroFramework.Forms;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Globalization;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;


namespace ASCOM.AstroPowerBoxXXL.Switch
{
    public partial class Form1 : MetroForm
    {
        internal static string[] name_ = new string[SwitchHardware.numSwitch];
        internal static string[] nameChanged = new string[SwitchHardware.numSwitch];
        bool doNameChange = false;
        bool doLimitChange = false;
        bool doReverseChange = false;
        bool doSSIDChange = false;
        bool doPWDChange = false;

        public Form1()
        {
            InitializeComponent();
            var cts = new CancellationTokenSource();
            Task periodicTask = RunPeriodicTask(cts.Token);
            this.ComPortBox.Text = SwitchHardware.comPort.ToString();

            for (short i = 0; i < SwitchHardware.numSwitch; i++)
            {
                name_[i] = SwitchHardware.name[i];
                nameChanged[i] = "";
            }
            PopulateParam();

            // 1. Subscribe to the ControlAdded event for future controls
            this.metroTabPage7.ControlAdded += Panel1_ControlAdded;

            // 2. Iterate through and subscribe to events for existing controls
            foreach (Control control in this.metroTabPage7.Controls)
            {
                AttachChangeHandler(control);
            }
            if(SwitchHardware.numUSB!=0) 
            {
                this.SaveEEPROM.Location = new Point(1110, this.SaveEEPROM.Location.Y);
                this.Reload.Location = new Point(1110, this.Reload.Location.Y);
                this.pictureBox2.Size = new Size(616, this.pictureBox2.Size.Height);
                this.metroTabControl1.Size = new Size(1227, this.metroTabControl1.Size.Height);
                this.Size = new Size(1268, this.Size.Height);
            }
            else
            {
                this.SaveEEPROM.Location = new Point(796, this.SaveEEPROM.Location.Y);
                this.Reload.Location = new Point(796, this.Reload.Location.Y);
                this.pictureBox2.Size = new Size(287, this.pictureBox2.Size.Height);
                this.metroTabControl1.Size = new Size(970, this.metroTabControl1.Size.Height);
                this.Size = new Size(1011, this.Size.Height);
                this.metroLabel98.Visible = false;
                this.metroLabel97.Visible = false;
                this.metroLabel100.Visible = false;
                this.metroLabel101.Visible = false;
                this.metroLabel102.Visible = false;
                this.metroLabel104.Visible = false;
                this.metroLabel106.Visible = false;
                this.metroLabel108.Visible = false;
                this.metroLabel109.Visible = false;
                this.name12.Visible = false;
                this.name13.Visible = false;
                this.name14.Visible = false;
                this.name15.Visible = false;
                this.name16.Visible = false;
                this.name17.Visible = false;
                this.name18.Visible = false;

            }

            this.metroTabControl1.SelectedIndex = 0;

        }

        private void Panel1_ControlAdded(object sender, ControlEventArgs e)
        {
            // This handles controls added to the panel at runtime.
            AttachChangeHandler(e.Control);
        }

        private void AttachChangeHandler(Control control)
        {
            // This is your shared method to attach the change event.
            if (control is MetroFramework.Controls.MetroTextBox textbox)
            {
                textbox.TextChanged += GenericControl_Changed;
            }
            else if (control is CheckBox checkBox)
            {
                checkBox.CheckedChanged += GenericControl_Changed;
            }

        }

        private void GenericControl_Changed(object sender, EventArgs e)
        {
            // Your logic to handle the change goes here.
            Control changedControl = (Control)sender;
            if (changedControl.Name.Contains("name"))
            {
                doNameChange = true;
                nameChanged[int.Parse(changedControl.Name.Replace("name", ""))] = changedControl.Text;
            }

            else if (changedControl.Name.Contains("limit")) doLimitChange = true;
            else if (changedControl.Name.Contains("reverse")) doReverseChange = true;
            else if (changedControl.Name.Contains("SSID")) doSSIDChange = true;
            else if (changedControl.Name.Contains("PWD")) doPWDChange = true;

        }

        private void PopulateParam()
        {

            this.reverse0.Checked = SwitchHardware.ReverseDC;
            this.reverse3.Checked = SwitchHardware.ReverseRelay;
            this.reverse2.Checked = SwitchHardware.ReverseOn;
            this.reverse1.Checked = SwitchHardware.ReversePWM;

            this.limit0.Text = SwitchHardware.LimitDC.ToString();
            this.limit2.Text = SwitchHardware.LimitOn.ToString();
            this.limit1.Text = SwitchHardware.LimitPWM.ToString();
            this.limit3.Text = SwitchHardware.LimitTotalDC.ToString();
            this.limit4.Text = SwitchHardware.LimitTotalPWM.ToString();
            this.limit5.Text = SwitchHardware.LimitTotal.ToString();

            this.name0.Text = SwitchHardware.name[0];
            this.name1.Text = SwitchHardware.name[1];
            this.name2.Text = SwitchHardware.name[2];
            this.name3.Text = SwitchHardware.name[3];
            this.name4.Text = SwitchHardware.name[4];
            this.name5.Text = SwitchHardware.name[5];
            this.name6.Text = SwitchHardware.name[6];
            this.name7.Text = SwitchHardware.name[7];
            this.name8.Text = SwitchHardware.name[8];
            this.name9.Text = SwitchHardware.name[9];
            this.name10.Text = SwitchHardware.name[13];
            this.name11.Text = SwitchHardware.name[14];

            this.SSIDBox.Text = SwitchHardware.SSID;
            this.PWDBox.Text = SwitchHardware.PWD;
            this.IPbox1.Text = SwitchHardware.WiFiIP;
            this.IPbox2.Text = SwitchHardware.WiFiIP;

            if (SwitchHardware.numUSB > 0)
            {
                this.name12.Text = SwitchHardware.name[15];
                this.name13.Text = SwitchHardware.name[16];
                this.name14.Text = SwitchHardware.name[17];
                this.name15.Text = SwitchHardware.name[18];
                this.name16.Text = SwitchHardware.name[19];
                this.name17.Text = SwitchHardware.name[20];
                this.name18.Text = SwitchHardware.name[21];
            }

        }
        private async Task PopulateDashboard()
        {
            if (SwitchHardware.connectedState)
            {

                this.ErrorMessage.Text = SwitchHardware.LastErrorMessage;
                int Index_Sensor0 = (SwitchHardware.total);
                int Index_SensorDC0 = Index_Sensor0 + 4;
                int Index_SensorPWM0 = Index_SensorDC0+2*SwitchHardware.numDC ;
                int Index_SensorOn0 = Index_SensorPWM0 + 2 * SwitchHardware.numPWM;
                float v, a;


                try
                {
                    SwitchHardware.GetIP_USB();
                    this.IPbox1.Text = SwitchHardware.WiFiIP;
                    this.IPbox2.Text = SwitchHardware.WiFiIP;
                    this.Vin.Text = SwitchHardware.state[Index_Sensor0];
                    v = float.Parse(this.Vin.Text, CultureInfo.InvariantCulture);
                    this.TotalA.Text = SwitchHardware.state[Index_Sensor0 + 1];
                    a = float.Parse(this.TotalA.Text, CultureInfo.InvariantCulture);
                    this.TotalDCA.Text = SwitchHardware.state[Index_Sensor0 + 2];
                    this.TotalDCA.Text = SwitchHardware.state[Index_Sensor0 + 2];
                    this.TotalPWMA.Text = SwitchHardware.state[Index_Sensor0 + 3];
                    if(SwitchHardware.numRen==1)
                    {
                        this.TempBox.Text = SwitchHardware.state[SwitchHardware.numSwitch -3];
                        this.HumBox.Text = SwitchHardware.state[SwitchHardware.numSwitch - 2];
                        this.DewBox.Text = SwitchHardware.state[SwitchHardware.numSwitch - 1];
                    }
                    this.TotalW.Text = (v * a).ToString();

                    String g;
                    g = SwitchHardware.state[0].Substring(0, 1);
                    this.DC1.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[0].Substring(0, 1)));
                    this.DC2.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[1].Substring(0, 1)));
                    this.DC3.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[2].Substring(0, 1)));
                    this.DC4.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[3].Substring(0, 1)));
                    this.DC5.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[4].Substring(0, 1)));
                    this.DC6.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[5].Substring(0, 1)));
                    this.DC7.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[6].Substring(0, 1)));
                    this.PWM1Getlabel.Text = SwitchHardware.state[7];
                    this.PWM2Getlabel.Text = SwitchHardware.state[8];
                    this.PWM3Getlabel.Text = SwitchHardware.state[9];
                    this.Auto1.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[10].Substring(0, 1)));
                    this.Auto2.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[11].Substring(0, 1)));
                    this.Auto3.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[12].Substring(0, 1)));
                    this.Rail.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[13].Substring(0, 1)));
                    this.Relay.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[14].Substring(0, 1)));
                    if (SwitchHardware.numUSB == 7) { 
                    this.USB1.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[15].Substring(0, 1)));
                    this.USB2.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[16].Substring(0, 1)));
                    this.USB3.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[17].Substring(0, 1)));
                    this.USB4.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[18].Substring(0, 1)));
                    this.USB5.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[19].Substring(0, 1)));
                    this.USB6.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[20].Substring(0, 1)));
                    this.USB7.Checked = Convert.ToBoolean(int.Parse(SwitchHardware.state[21].Substring(0, 1)));
                }
                    this.DCV1.Text = SwitchHardware.state[Index_SensorDC0];
                    this.DCA1.Text = SwitchHardware.state[Index_SensorDC0 + 1];
                    this.DCV2.Text = SwitchHardware.state[Index_SensorDC0 + 2];
                    this.DCA2.Text = SwitchHardware.state[Index_SensorDC0 + 3];
                    this.DCV3.Text = SwitchHardware.state[Index_SensorDC0 + 4];
                    this.DCA3.Text = SwitchHardware.state[Index_SensorDC0 + 5];
                    this.DCV4.Text = SwitchHardware.state[Index_SensorDC0 + 6];
                    this.DCA4.Text = SwitchHardware.state[Index_SensorDC0 + 7];
                    this.DCV5.Text = SwitchHardware.state[Index_SensorDC0 + 8];
                    this.DCA5.Text = SwitchHardware.state[Index_SensorDC0 + 9];
                    this.DCV6.Text = SwitchHardware.state[Index_SensorDC0 + 10];
                    this.DCA6.Text = SwitchHardware.state[Index_SensorDC0 + 11];
                    this.DCV7.Text = SwitchHardware.state[Index_SensorDC0 + 12];
                    this.DCA7.Text = SwitchHardware.state[Index_SensorDC0 + 13];

                    this.RailV.Text = SwitchHardware.state[Index_SensorOn0];
                    this.RailA.Text = SwitchHardware.state[Index_SensorOn0 + 1];

                    this.PWM1V.Text = SwitchHardware.state[Index_SensorPWM0];
                    this.PWM1A.Text = SwitchHardware.state[Index_SensorPWM0 + 1];
                    this.PWM2V.Text = SwitchHardware.state[Index_SensorPWM0 + 2];
                    this.PWM2A.Text = SwitchHardware.state[Index_SensorPWM0 + 3];
                    this.PWM3V.Text = SwitchHardware.state[Index_SensorPWM0 + 4];
                    this.PWM3A.Text = SwitchHardware.state[Index_SensorPWM0 + 5];

                }
                catch (Exception ex)
                {
                    MessageBox.Show($"Error retrieving DC voltage: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private async Task RunPeriodicTask(CancellationToken token)
        {
            while (!token.IsCancellationRequested)
            {
                try
                {
                    await PopulateDashboard();
                    await Task.Delay(TimeSpan.FromSeconds(1), token);
                }
                catch (TaskCanceledException)
                {
                    // This exception is expected when cancellation is requested
                    Console.WriteLine("Task was cancelled. Stopping.");
                    break;
                }
            }
        }

        private void CloseButton_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void SaveEEPROM_Click(object sender, EventArgs e)
        {
            if(doNameChange)
            {
                for (int i = 0; i < SwitchHardware.total; i++)
                {
                    if (nameChanged[i] != "")
                    {
                        if (nameChanged[i].Contains(' ')) nameChanged[i] = nameChanged[i].Replace(' ', '_');
                        SwitchHardware.SetSwitchName((short)i, nameChanged[i]);
                        nameChanged[i] = "";
                    }
                }
                doNameChange = false;
            }
            else if (doLimitChange)
            {
                 float g;
                if(float.TryParse(this.limit0.Text,out g))SwitchHardware.SetLimit_USB(0,this.limit0.Text );
                if (float.TryParse(this.limit2.Text, out g)) SwitchHardware.SetLimit_USB(2, this.limit2.Text);
                if (float.TryParse(this.limit1.Text, out g)) SwitchHardware.SetLimit_USB(3, this.limit1.Text);
                if (float.TryParse(this.limit3.Text, out g)) SwitchHardware.SetLimit_USB(4, this.limit3.Text);
                if (float.TryParse(this.limit4.Text, out g)) SwitchHardware.SetLimit_USB(5, this.limit4.Text);
                if (float.TryParse(this.limit5.Text, out g)) SwitchHardware.SetLimit_USB(6, this.limit5.Text);
                doLimitChange = false;
            }
            else if (doReverseChange)
            {
                SwitchHardware.SetReverse_USB(0, this.reverse0.Checked);
                SwitchHardware.SetReverse_USB(1, this.reverse3.Checked);
                SwitchHardware.SetReverse_USB(2, this.reverse2.Checked);
                SwitchHardware.SetReverse_USB(3, this.reverse1.Checked);
                doReverseChange = false;
            }
            else if (doSSIDChange)
            {
                SwitchHardware.SetSSID_USB(this.SSIDBox.Text);
                doSSIDChange = false;
            }
            else if (doPWDChange)
            {
                SwitchHardware.SetPWD_USB(this.PWDBox.Text);
                doPWDChange = false;
            }  
            
            Thread.Sleep(1000);
            PopulateParam();
        }

        private void Reload_Click(object sender, EventArgs e)
        {
            PopulateParam();
            for (short i = 0; i < SwitchHardware.numSwitch; i++)
            {
                nameChanged[i] = "";
            }

            doLimitChange = false;
            doReverseChange = false;
            doSSIDChange = false;
            doPWDChange = false;
            doNameChange = false;
        }

        private void OpenBrowser_Click(object sender, EventArgs e)
        {
            if(ValidateIPv4(SwitchHardware.WiFiIP)) System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo("http://" + SwitchHardware.WiFiIP + ":4040/") { UseShellExecute = true });
        }

        public bool ValidateIPv4(string ipString)
        {
            if (String.IsNullOrWhiteSpace(ipString))
            {
                return false;
            }

            string[] splitValues = ipString.Split('.');
            if (splitValues.Length != 4)
            {
                return false;
            }

            byte tempForParsing;

            return splitValues.All(r => byte.TryParse(r, out tempForParsing));
        }

        private void metroTrackBar1_ValueChanged(object sender, EventArgs e)
        {
            PWM1Set.Text = metroTrackBar1.Value.ToString();
        }

        private void metroTrackBar2_ValueChanged(object sender, EventArgs e)
        {
            PWM2Set.Text = metroTrackBar2.Value.ToString();
        }

        private void metroTrackBar3_ValueChanged(object sender, EventArgs e)
        {
            PWM3Set.Text = metroTrackBar3.Value.ToString();
        }

        private void Set1_Click(object sender, EventArgs e)
        {
            if ((PWM1Set.Text != PWM1Getlabel + "%") && (int.Parse(PWM1Set.Text) >= 0) && (int.Parse(PWM1Set.Text) <= 100)) SwitchHardware.SetSwitchValueUSB(SwitchHardware.numDC, double.Parse(PWM1Set.Text));
            
        }

        private void Set2_Click(object sender, EventArgs e)
        {
            if ((PWM2Set.Text != PWM2Getlabel + "%") && (int.Parse(PWM2Set.Text) >= 0) && (int.Parse(PWM2Set.Text) <= 100)) SwitchHardware.SetSwitchValueUSB((short)(SwitchHardware.numDC+1), double.Parse(PWM2Set.Text));
        }

        private void Set3_Click(object sender, EventArgs e)
        {
            if ((PWM3Set.Text != PWM3Getlabel + "%") && (int.Parse(PWM3Set.Text) >= 0) && (int.Parse(PWM3Set.Text) <= 100)) SwitchHardware.SetSwitchValueUSB((short)(SwitchHardware.numDC+2), double.Parse(PWM3Set.Text));
        }

        private void PWM1Set_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (!char.IsDigit(e.KeyChar) && e.KeyChar != (char)Keys.Back)
            {
                e.Handled = true; // Block the input.
            }
            if (int.Parse(PWM1Set.Text) > 100) PWM1Set.Text = "100";
        }

        private void PWM2Set_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (!char.IsDigit(e.KeyChar) && e.KeyChar != (char)Keys.Back)
            {
                e.Handled = true; // Block the input.
            }
            if (int.Parse(PWM2Set.Text) > 100) PWM2Set.Text = "100";
        }

        private void PWM3Set_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (!char.IsDigit(e.KeyChar) && e.KeyChar != (char)Keys.Back)
            {
                e.Handled = true; // Block the input.
            }
            if (int.Parse(PWM3Set.Text) > 100) PWM3Set.Text = "100";
        }

        private void PWM1Set_TextChanged(object sender, EventArgs e)
        {
            int a = int.Parse(PWM1Set.Text);
            if (a > 100) PWM1Set.Text = "100";
        }

        private void PWM2Set_TextChanged(object sender, EventArgs e)
        {
            int a = int.Parse(PWM2Set.Text);
            if (a > 100) PWM2Set.Text = "100";
        }

        private void PWM3Set_TextChanged(object sender, EventArgs e)
        {
            int a = int.Parse(PWM3Set.Text);
            if (a > 100) PWM3Set.Text = "100";
        }

        private void DC1_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(0, Convert.ToDouble(DC1.Checked));
        }

        private void DC2_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(1, Convert.ToDouble(DC2.Checked));
        }

        private void DC3_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(2, Convert.ToDouble(DC3.Checked));
        }

        private void DC4_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(3, Convert.ToDouble(DC4.Checked));
        }

        private void DC5_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(4, Convert.ToDouble(DC5.Checked));
        }

        private void DC6_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(5, Convert.ToDouble(DC6.Checked));
        }

        private void DC7_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(6, Convert.ToDouble(DC7.Checked));
        }

        private void Rail_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(13, Convert.ToDouble(Rail.Checked));
        }

        private void Relay_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(14, Convert.ToDouble(Relay.Checked));
        }

        private void reverse0_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetReverse_USB(0, this.reverse0.Checked);
        }

        private void reverse1_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetReverse_USB(1, this.reverse1.Checked);
        }

        private void reverse2_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetReverse_USB(2, this.reverse2.Checked);
        }

        private void reverse3_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetReverse_USB(3, this.reverse3.Checked);
        }

        private void USB1_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(15, Convert.ToDouble(USB1.Checked));
        }

        private void USB2_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(16, Convert.ToDouble(USB2.Checked));
        }

        private void USB3_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(17, Convert.ToDouble(USB3.Checked));
        }

        private void USB4_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(18, Convert.ToDouble(USB4.Checked));
        }

        private void USB5_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(19, Convert.ToDouble(USB5.Checked));
        }

        private void USB6_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(20, Convert.ToDouble(USB6.Checked));
        }

        private void USB7_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(21, Convert.ToDouble(USB7.Checked));
        }

        private void metroButton1_Click(object sender, EventArgs e)
        {
           DialogResult Result =MessageBox.Show("We must restart the device to load new wifi parameters. Please confirm or cancel", "Info", MessageBoxButtons.OKCancel, MessageBoxIcon.Information);
            if (Result == DialogResult.OK)
            {
                SwitchHardware.SetSSID_USB(this.SSIDBox.Text);
                SwitchHardware.SetPWD_USB(this.PWDBox.Text);
                SwitchHardware.Restart_USB();
            }
        }

        private void Auto1_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(10, Convert.ToDouble(Auto1.Checked));
        }

        private void Auto2_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(11, Convert.ToDouble(Auto2.Checked));
        }

        private void Auto3_CheckedChanged(object sender, EventArgs e)
        {
            SwitchHardware.SetSwitchValueUSB(12, Convert.ToDouble(Auto3.Checked));
        }
    }
}
