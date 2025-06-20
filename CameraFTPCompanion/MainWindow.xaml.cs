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
using Microsoft.Win32;
using System.Drawing;
using System.ComponentModel;

namespace CameraFTPCompanion
{
    public partial class MainWindow : Window
    {
        private const int MinPortValue = 0;
        private const int MaxPortValue = 65535;

        private NotifyIcon _notifyIcon;
        private bool _isClosing = false;

        public MainWindow()
        {
            InitializeComponent();
            LoadConfig();
            txtFtpPort.PreviewTextInput += TxtFtpPort_PreviewTextInput;
            txtFtpPort.TextChanged += TxtFtpPort_TextChanged;
            System.Windows.DataObject.AddPastingHandler(txtFtpPort, TxtFtpPort_Pasting);
            chkAutoStart.Checked += ChkAutoStart_Checked;
            chkAutoStart.Unchecked += ChkAutoStart_Unchecked;
            InitializeTrayIcon();
            // 检查是否从开机自启启动，如果是则自动运行Start功能
            string[] args = Environment.GetCommandLineArgs();
            if (args.Length > 1 && args[1] == "--autostart")
            {
                Dispatcher.BeginInvoke(new Action(() =>
                {
                    BtnStart_Click(null, null);
                    if (!_isClosing && chkRunInBackground.IsChecked == true)
                    {
                        // 延迟隐藏窗口，确保窗口完全加载后再隐藏
                        Dispatcher.BeginInvoke(new Action(() =>
                        {
                            Hide();
                        }), System.Windows.Threading.DispatcherPriority.Background);
                    }
                }));
            }
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
                if (lines.Length >= 8)
                {
                    txtFolderPath.Text = lines[0];
                    chkAutoRun.IsChecked = bool.Parse(lines[1]);
                    autoRunExePath.Text = lines[2];
                    txtFileExtension.Text = lines[3];
                    chkFtpMode.IsChecked = bool.Parse(lines[4]);
                    txtFtpPort.Text = lines[5];
                    chkAutoStart.IsChecked = bool.Parse(lines[6]);
                    chkRunInBackground.IsChecked = bool.Parse(lines[7]);
                }
            }
        }

        private void SaveConfig()
        {
            string configPath = GetConfigPath();
            System.IO.File.WriteAllText(configPath, $"{txtFolderPath.Text}\n{chkAutoRun.IsChecked ?? false}\n{autoRunExePath.Text}\n{txtFileExtension.Text}\n{chkFtpMode.IsChecked ?? false}\n{txtFtpPort.Text}\n{chkAutoStart.IsChecked ?? false}\n{chkRunInBackground.IsChecked ?? false}");
        }

        protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
        {
            if (!_isClosing && chkRunInBackground.IsChecked == true)
            {
                e.Cancel = true;
                Hide();
            }
            else
            {
                Stop();
                SaveConfig();
                _notifyIcon?.Dispose();
                base.OnClosing(e);
            }
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
            var fileDialog = new System.Windows.Forms.OpenFileDialog();
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

        private void ChkAutoStart_Checked(object sender, RoutedEventArgs e)
        {
            SetAutoStart(true);
        }

        private void ChkAutoStart_Unchecked(object sender, RoutedEventArgs e)
        {
            SetAutoStart(false);
        }

        private void InitializeTrayIcon()
        {
            _notifyIcon = new NotifyIcon
            {
                Text = "微单相机图传伴侣",
                Visible = true
            };

            var iconStream = System.Windows.Application.GetResourceStream(new Uri("pack://application:,,,/CameraFTPCompanion;component/Resources/icon.ico"))?.Stream;
            if (iconStream != null)
            {
                _notifyIcon.Icon = new Icon(iconStream);
            }

            _notifyIcon.DoubleClick += (s, args) => ShowWindow();

            var contextMenu = new ContextMenuStrip();
            contextMenu.Items.Add("显示", null, (s, e) => ShowWindow());
            contextMenu.Items.Add("开始", null, (s, e) => BtnStart_Click(null, null));
            contextMenu.Items.Add("停止", null, (s, e) => BtnStop_Click(null, null));
            contextMenu.Items.Add("退出", null, (s, e) =>
            {
                _isClosing = true;
                Close();
            });

            _notifyIcon.ContextMenuStrip = contextMenu;
        }

        private void ShowWindow()
        {
            Show();
            WindowState = WindowState.Normal;
            Activate();
        }

        private void SetAutoStart(bool enable)
        {
            string appName = "CameraFTPCompanion";
            string appPath = System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName;
            using (RegistryKey key = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Run", true))
            {
                if (enable)
                {
                    key.SetValue(appName, $"\"{appPath}\" --autostart");
                }
                else
                {
                    key.DeleteValue(appName, false);
                }
            }
        }
    }
}
