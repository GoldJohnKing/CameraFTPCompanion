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

namespace PixCakeWindowsCompanion
{
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            LoadConfig();
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
                if (lines.Length >= 3)
                {
                    txtFolderPath.Text = lines[0];
                    txtFileExtension.Text = lines[1];
                    chkFtpMode.IsChecked = bool.Parse(lines[2]);
                }
            }
        }

        private void SaveConfig()
        {
            string configPath = GetConfigPath();
            System.IO.File.WriteAllText(configPath, $"{txtFolderPath.Text}\n{txtFileExtension.Text}\n{chkFtpMode.IsChecked}");
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
