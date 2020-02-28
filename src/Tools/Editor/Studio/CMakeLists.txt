project(Alimer.Studio LANGUAGES CSharp)

include(CSharpUtilities)

set(CS_FIILES
    Properties/AssemblyInfo.cs
	Properties/Resources.resx
	Properties/Resources.Designer.cs
	Properties/Settings.settings
	Properties/Settings.Designer.cs

    App.config
    App.xaml
    App.xaml.cs
    MainWindow.xaml
	MainWindow.xaml.cs
)

add_executable(${PROJECT_NAME} ${CS_FIILES})

csharp_set_xaml_cs_properties(
    ${CS_FIILES}
)

set_property(SOURCE App.xaml PROPERTY VS_XAML_TYPE "ApplicationDefinition")
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_GLOBAL_ROOTNAMESPACE "Alimer.Studio")
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.6.1")
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "Editor")

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DOTNET_REFERENCES
	"Microsoft.CSharp"
	"PresentationCore"
	"PresentationFramework"
	"System"
	"System.Core"
	"System.Data"
	"System.Drawing"
	"System.Windows.Forms"
	"System.Data.DataSetExtensions"
	"System.Net.Http"
	"System.Xaml"
	"System.Xml"
	"System.Xml.Linq"
	"WindowsBase"
)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_PACKAGE_REFERENCES
    "Newtonsoft.Json_12.0.3"
)

if (NOT ALIMER_SKIP_INSTALL)
    install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION ${DEST_LIBRARY_DIR_CONFIG}
        RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
        ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG}
        BUNDLE  DESTINATION ${DEST_BIN_DIR_CONFIG}
    )
endif ()
