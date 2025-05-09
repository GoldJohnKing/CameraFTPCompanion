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
                if (lines.Length >= 6)
                {
                    txtFolderPath.Text = lines[0];
                    chkAutoRun.IsChecked = bool.Parse(lines[1]);
                    autoRunExePath.Text = lines[2];
                    txtFileExtension.Text = lines[3];
                    chkFtpMode.IsChecked = bool.Parse(lines[4]);
                    txtFtpPort.Text = lines[5];
                }
            }
        }

        private void SaveConfig()
        {
            string configPath = GetConfigPath();
            System.IO.File.WriteAllText(configPath, $"{txtFolderPath.Text}\n{chkAutoRun.IsChecked ?? false}\n{autoRunExePath.Text}\n{txtFileExtension.Text}\n{chkFtpMode.IsChecked ?? false}\n{txtFtpPort.Text}");
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

        private void BtnAutoRunBrowse_Click(object sender, RoutedEventArgs e)
        {
            var fileDialog = new OpenFileDialog();
            fileDialog.Filter = "可执行文件 (*.exe)|*.exe|所有文件 (*.*)|*.*";
            fileDialog.Title = "选择要自动打开文件的程序";
            if (fileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                autoRunExePath.Text = fileDialog.FileName;
            }
        }

        [DllImport("CoreFunctions.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool Start(string folderPath, bool autoRunEnabled, string execPath, string fileExtension, int ftpPort);

        [DllImport("CoreFunctions.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Stop();

        private void BtnStart_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(txtFolderPath.Text))
            {
                System.Windows.MessageBox.Show("请配置存储路径！");
                return;
            }

            if ((!chkAutoRun.IsChecked ?? false) && (!chkFtpMode.IsChecked ?? false))
            {
                System.Windows.MessageBox.Show("请至少启用一个功能！");
                return;
            }

            bool isRunning = Start(txtFolderPath.Text, (chkAutoRun.IsChecked ?? false), autoRunExePath.Text, txtFileExtension.Text, (chkFtpMode.IsChecked ?? false) ? Int32.Parse(txtFtpPort.Text) : -1);

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
