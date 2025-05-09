using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Forms;

namespace CameraFTPCompanion
{
    public partial class MainWindow : Window
    {
        private const int MinPortValue = 0;
        private const int MaxPortValue = 65535;

        public MainWindow()
        {
            InitializeComponent();
            LoadConfig();
            txtFtpPort.PreviewTextInput += TxtFtpPort_PreviewTextInput;
            txtFtpPort.TextChanged += TxtFtpPort_TextChanged;
            System.Windows.DataObject.AddPastingHandler(txtFtpPort, TxtFtpPort_Pasting);
        }

        private string GetConfigPath()
        {
            return System.IO.Path.Combine(System.AppDomain.CurrentDomain.BaseDirectory, "config.conf");
        }

        private void LoadConfig()
        {
            string configPath = GetConfigPath();
            if (System.IO.File.Exists(configPath))
            {
                string[] lines = System.IO.File.ReadAllLines(configPath);
                if (lines.Length >= 5)
                {
                    txtFolderPath.Text = lines[0];
                    autoRunExePath.Text = lines[1];
                    txtFileExtension.Text = lines[2];
                    chkFtpMode.IsChecked = bool.Parse(lines[3]);
                    txtFtpPort.Text = lines[4];
                }
            }
        }

        private void SaveConfig()
        {
            string configPath = GetConfigPath();
            System.IO.File.WriteAllText(configPath, $"{txtFolderPath.Text}\n{autoRunExePath.Text}\n{txtFileExtension.Text}\n{chkFtpMode.IsChecked}\n{txtFtpPort.Text}");
        }

        protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
        {
            Stop();
            SaveConfig();
            base.OnClosing(e);
        }

        private void BtnBrowse_Click(object sender, RoutedEventArgs e)
        {
            var folderDialog = new FolderBrowserDialog();
            if (folderDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                txtFolderPath.Text = folderDialog.SelectedPath;
            }
        }

        [DllImport("CoreFunctions.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool Start(string folderPath, string fileExtension, bool ftpServerEnabled);

        [DllImport("CoreFunctions.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Stop();

        private void BtnStart_Click(object sender, RoutedEventArgs e)
        {
            bool isRunning = Start(txtFolderPath.Text, txtFileExtension.Text, chkFtpMode.IsChecked ?? false);

            if (isRunning)
            {
                txtEmojiStatus.Text = "✅";
                txtEmojiStatus.Foreground = System.Windows.Media.Brushes.Green;
                txtStatus.Text = "正在运行";
                btnStart.IsEnabled = false;
                btnStop.IsEnabled = true;
            }
            else
            {
                txtEmojiStatus.Text = "❌";
                txtEmojiStatus.Foreground = System.Windows.Media.Brushes.Red;
                txtStatus.Text = "启动失败";
                btnStart.IsEnabled = true;
                btnStop.IsEnabled = false;
            }
        }

        private void TxtFtpPort_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            e.Handled = !int.TryParse(e.Text, out _);
        }

        private void TxtFtpPort_Pasting(object sender, DataObjectPastingEventArgs e)
        {
            if (e.DataObject.GetDataPresent(typeof(string)))
            {
                string text = (string)e.DataObject.GetData(typeof(string));
                if (!text.All(char.IsDigit))
                {
                    e.CancelCommand();
                }
            }
        }

        private void TxtFtpPort_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (string.IsNullOrEmpty(txtFtpPort.Text))
            {
                txtFtpPort.Text = MinPortValue.ToString();
                return;
            }

            if (int.TryParse(txtFtpPort.Text, out int port))
            {
                if (port < MinPortValue)
                    txtFtpPort.Text = MinPortValue.ToString();
                else if (port > MaxPortValue)
                    txtFtpPort.Text = MaxPortValue.ToString();
            }
            else
            {
                txtFtpPort.Text = MinPortValue.ToString();
            }
        }

        private void BtnStop_Click(object sender, RoutedEventArgs e)
        {
            btnStop.IsEnabled = false;
            btnStart.IsEnabled = true;
            Stop();

            txtEmojiStatus.Text = "⏸️";
            txtEmojiStatus.Foreground = System.Windows.Media.Brushes.Blue;
            txtStatus.Text = "未运行/已停止";
            btnStart.IsEnabled = true;
            btnStop.IsEnabled = false;
        }
    }
}
