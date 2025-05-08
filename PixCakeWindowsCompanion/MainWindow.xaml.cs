using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Forms;

namespace PixCakeWindowsCompanion
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
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
        public static extern void Start(string folderPath, string fileExtension, bool ftpServerEnabled);

        [DllImport("CoreFunctions.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Stop();

        private void BtnStart_Click(object sender, RoutedEventArgs e)
        {
            Start(txtFolderPath.Text, txtFileExtension.Text, chkFtpMode.IsChecked ?? false);
        }

        private void BtnStop_Click(object sender, RoutedEventArgs e)
        {
            Stop();
        }
    }
}
