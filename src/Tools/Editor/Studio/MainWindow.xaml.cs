using Newtonsoft.Json;
using System.Windows;

namespace Alimer.Studio
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();

            Title += JsonConvert.SerializeObject(new MyObject { Name = "CIAO" });
        }

        class MyObject
        {
            public string Name { get; set; }
        }
    }
}
