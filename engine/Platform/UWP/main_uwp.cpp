//
// Copyright (c) 2020 Amer Koleci and contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "platform/platform.h"
#include "application.h"
#include <Windows.h>
#include "winrt/Windows.ApplicationModel.h"
#include "winrt/Windows.ApplicationModel.Core.h"
#include "winrt/Windows.ApplicationModel.Activation.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Graphics.Display.h"
#include "winrt/Windows.System.h"
#include "winrt/Windows.UI.Core.h"
#include "winrt/Windows.UI.Input.h"
#include "winrt/Windows.UI.ViewManagement.h"

using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::Input;
using namespace winrt::Windows::UI::ViewManagement;
using namespace winrt::Windows::System;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Graphics::Display;

using namespace Alimer;

namespace
{
    ::IUnknown* window;
}

bool Platform::init(const Config*)
{
    return true;
}

void Platform::shutdown() noexcept
{
}

void Platform::run()
{
}

::IUnknown* Platform::get_native_handle(void) noexcept
{
    return window;
}

namespace
{
    class ViewProvider : public winrt::implements<ViewProvider, IFrameworkView>
    {
    public:
        ViewProvider() noexcept
            : config{}
            , visible(true)
            , dpi(96.0f)
        {
        }

        // IFrameworkView methods
        void Initialize(const CoreApplicationView& applicationView)
        {
            applicationView.Activated({ this, &ViewProvider::OnActivated });

            CoreApplication::Suspending({ this, &ViewProvider::OnSuspending });

            CoreApplication::Resuming({ this, &ViewProvider::OnResuming });

            config = App::main(__argc, __argv);
        }

        void Uninitialize() noexcept
        {

        }

        void SetWindow(const CoreWindow& coreWindow)
        {
            auto currentDisplayInformation = DisplayInformation::GetForCurrentView();
            dpi = currentDisplayInformation.LogicalDpi();

            logicalWidth = coreWindow.Bounds().Width;
            logicalHeight = coreWindow.Bounds().Height;

            config.width = ConvertDipsToPixels(logicalWidth);
            config.height = ConvertDipsToPixels(logicalHeight);

            window = static_cast<::IUnknown*>(winrt::get_abi(coreWindow));
        }

        void Load(winrt::hstring const&) noexcept
        {
        }

        void Run()
        {
            Alimer::App::run(&config);

            while (App::is_running())
            {
                if (visible)
                {
                    App::tick();

                    CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
                }
                else
                {
                    CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
                }
            }
        }

        inline uint32_t ConvertDipsToPixels(float dips) const noexcept
        {
            return int(dips * dpi / 96.f + 0.5f);
        }

        inline float ConvertPixelsToDips(uint32_t pixels) const noexcept
        {
            return (float(pixels) * 96.f / dpi);
        }

    protected:
        // Event handlers
        void OnActivated(CoreApplicationView const& /*applicationView*/, IActivatedEventArgs const& args)
        {
            if (args.Kind() == ActivationKind::Launch)
            {
                auto launchArgs = (const LaunchActivatedEventArgs*)(&args);

                if (launchArgs->PrelaunchActivated())
                {
                    // Opt-out of Prelaunch
                    CoreApplication::Exit();
                    return;
                }
            }

            dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
            auto view = ApplicationView::GetForCurrentView();

            view.Title(L"CIAO");
            if (config.fullscreen)
            {
                ApplicationView::PreferredLaunchWindowingMode(ApplicationViewWindowingMode::FullScreen);
                CoreWindow::GetForCurrentThread().Activate();
            }
            else
            {
                ApplicationView::PreferredLaunchWindowingMode(ApplicationViewWindowingMode::PreferredLaunchViewSize);
                auto desiredSize = Size(ConvertPixelsToDips(config.width), ConvertPixelsToDips(config.height));
                ApplicationView::PreferredLaunchViewSize(desiredSize);

                auto minSize = Size(ConvertPixelsToDips(320), ConvertPixelsToDips(200));
                view.SetPreferredMinSize(minSize);

                CoreWindow::GetForCurrentThread().Activate();

                view.FullScreenSystemOverlayMode(FullScreenSystemOverlayMode::Minimal);

                view.TryResizeView(desiredSize);
            }
        }

        void OnSuspending(IInspectable const& /*sender*/, SuspendingEventArgs const& args)
        {
            auto deferral = args.SuspendingOperation().GetDeferral();

            /*auto f = std::async(std::launch::async, [this, deferral]()
            {
                m_game->OnSuspending();

                deferral.Complete();
            });*/
        }

        void OnResuming(IInspectable const& /*sender*/, IInspectable const& /*args*/)
        {
            //m_game->OnResuming();
        }

    private:
        Alimer::Config config;
        bool visible;
        float dpi;
        float logicalWidth{ 0.0f };
        float logicalHeight{ 0.0f };
    };

    class ViewProviderFactory : public winrt::implements<ViewProviderFactory, IFrameworkViewSource>
    {
    public:
        IFrameworkView CreateView()
        {
            return winrt::make<ViewProvider>();
        }
    };
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (!DirectX::XMVerifyCPUSupport())
    {
        throw std::exception("XMVerifyCPUSupport");
    }

    auto viewProviderFactory = winrt::make<ViewProviderFactory>();
    CoreApplication::Run(viewProviderFactory);
    return 0;
}
