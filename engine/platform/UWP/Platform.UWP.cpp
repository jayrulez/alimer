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
#include "winrt/Windows.System.Profile.h"
#include "winrt/Windows.UI.ViewManagement.h"

using namespace winrt::Windows::System::Profile;
using namespace winrt::Windows::UI::ViewManagement;

namespace Alimer
{
    std::string Platform::GetName()
    {
        return winrt::to_string(AnalyticsInfo::VersionInfo().DeviceFamily());
    }

    PlatformId Platform::GetId()
    {
        return PlatformId::UWP;
    }

    PlatformFamily Platform::GetFamily()
    {
        auto deviceFamily = AnalyticsInfo::VersionInfo().DeviceFamily();
        if (deviceFamily == L"Windows.Mobile")
        {
            return PlatformFamily::Mobile;
        }
        else if (deviceFamily == L"Windows.Xbox" || deviceFamily == L"Windows.Team")
        {
            return PlatformFamily::Console;
        }
        else if (deviceFamily == L"Windows.Universal" || deviceFamily == L"Windows.Desktop")
        {
            auto interactionMode = UIViewSettings::GetForCurrentView().UserInteractionMode();
            if (interactionMode == UserInteractionMode::Mouse)
                return PlatformFamily::Desktop;

            return PlatformFamily::Mobile;
        }
        else if (deviceFamily == L"Windows.Holographic")
        {

        }
        else if (deviceFamily == L"Windows.IoT")
        {

        }

        return PlatformFamily::Unknown;
    }
}
