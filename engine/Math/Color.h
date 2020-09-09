//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
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

#pragma once

#include "Core/Math.h"

#if defined(__GNUC__) && !defined(__MINGW32__)
#define ALIMER_SELECT_ANY __attribute__((weak))
#else
#define ALIMER_SELECT_ANY __declspec(selectany)
#endif

namespace Alimer
{
    /// Class specifying a floating-point RGBA color.
    class ALIMER_API Color final
    {
    public:
        /// Specifies the red component of the color.
        float r;
        /// Specifies the green component of the color.
        float g;
        /// Specifies the blue component of the color.
        float b;
        /// Specifies the alpha component of the color.
        float a;

        /// Constructor.
        Color() noexcept : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}
        constexpr Color(float r_, float g_, float b_) noexcept : r(r_), g(g_), b(b_), a(1.0f) {}
        constexpr Color(float r_, float g_, float b_, float a_) noexcept : r(r_), g(g_), b(b_), a(a_) {}
        explicit Color(const Float3& vector) noexcept : r(vector.x), g(vector.y), b(vector.z), a(1.0f) {}
        explicit Color(const Float4& vector) noexcept : r(vector.x), g(vector.y), b(vector.z), a(vector.w) {}
        explicit Color(_In_reads_(4) const float* pArray) noexcept : r(pArray[0]), g(pArray[1]), b(pArray[2]), a(pArray[3]) {}

        Color(const Color&) = default;
        Color& operator=(const Color&) = default;
        Color(Color&&) = default;
        Color& operator=(Color&&) = default;

        // Comparison operators
        bool operator == (const Color& rhs) const noexcept { return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a; }
        bool operator != (const Color& rhs) const noexcept { return r != rhs.r || g != rhs.g || b != rhs.b || a != rhs.a; }

        /// Return float data.
        const float* Data() const { return &r; }

        /// Return as string.
        std::string ToString() const;
    };

    /* Copied from DirectXMath (https://github.com/microsoft/DirectXMath/blob/b412a61c518923e52e2d43d9e4d7084af8352ca2/Inc/DirectXColors.h) */
    namespace Colors
    {
        // Standard colors (Red/Green/Blue/Alpha)
        extern const ALIMER_SELECT_ANY Color AliceBlue = { 0.941176534f, 0.972549081f, 1.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color AntiqueWhite = { 0.980392218f, 0.921568692f, 0.843137324f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Aqua = { 0.0f, 1.0f, 1.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Aquamarine = { 0.498039246f, 1.0f, 0.831372619f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Azure = { 0.941176534f, 1.0f, 1.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Beige = { 0.960784376f, 0.960784376f, 0.862745166f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Bisque = { 1.0f, 0.894117713f, 0.768627524f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Black = { 0.0f, 0.0f, 0.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color BlanchedAlmond = { 1.0f, 0.921568692f, 0.803921640f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color BlueViolet = { 0.541176498f, 0.168627456f, 0.886274576f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Brown = { 0.647058845f, 0.164705887f, 0.164705887f, 1.0f };
        extern const ALIMER_SELECT_ANY Color BurlyWood = { 0.870588303f, 0.721568644f, 0.529411793f, 1.0f };
        extern const ALIMER_SELECT_ANY Color CadetBlue = { 0.372549027f, 0.619607866f, 0.627451003f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Chartreuse = { 0.498039246f, 1.0f, 0.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Chocolate = { 0.823529482f, 0.411764741f, 0.117647067f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Coral = { 1.0f, 0.498039246f, 0.313725501f, 1.0f };
        extern const ALIMER_SELECT_ANY Color CornflowerBlue = { 0.392156899f, 0.584313750f, 0.929411829f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Cornsilk = { 1.0f, 0.972549081f, 0.862745166f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Crimson = { 0.862745166f, 0.078431375f, 0.235294133f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkBlue = { 0.0f, 0.0f, 0.545098066f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkCyan = { 0.0f, 0.545098066f, 0.545098066f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkGoldenrod = { 0.721568644f, 0.525490224f, 0.043137256f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkGray = { 0.662745118f, 0.662745118f, 0.662745118f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkGreen = { 0.0f, 0.392156899f, 0.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkKhaki = { 0.741176486f, 0.717647076f, 0.419607878f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkMagenta = { 0.545098066f, 0.0f, 0.545098066f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkOliveGreen = { 0.333333343f, 0.419607878f, 0.184313729f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkOrange = { 1.0f, 0.549019635f, 0.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkOrchid = { 0.600000024f, 0.196078449f, 0.800000072f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkRed = { 0.545098066f, 0.0f, 0.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkSalmon = { 0.913725555f, 0.588235319f, 0.478431404f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkSeaGreen = { 0.560784340f, 0.737254918f, 0.545098066f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkSlateBlue = { 0.282352954f, 0.239215702f, 0.545098066f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkSlateGray = { 0.184313729f, 0.309803933f, 0.309803933f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkTurquoise = { 0.0f, 0.807843208f, 0.819607913f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DarkViolet = { 0.580392182f, 0.0f, 0.827451050f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DeepPink = { 1.0f, 0.078431375f, 0.576470613f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DeepSkyBlue = { 0.0f, 0.749019623f, 1.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DimGray = { 0.411764741f, 0.411764741f, 0.411764741f, 1.0f };
        extern const ALIMER_SELECT_ANY Color DodgerBlue = { 0.117647067f, 0.564705908f, 1.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Firebrick = { 0.698039234f, 0.133333340f, 0.133333340f, 1.0f };
        extern const ALIMER_SELECT_ANY Color FloralWhite = { 1.0f, 0.980392218f, 0.941176534f, 1.0f };
        extern const ALIMER_SELECT_ANY Color ForestGreen = { 0.133333340f, 0.545098066f, 0.133333340f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Fuchsia = { 1.0f, 0.0f, 1.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Gainsboro = { 0.862745166f, 0.862745166f, 0.862745166f, 1.0f };
        extern const ALIMER_SELECT_ANY Color GhostWhite = { 0.972549081f, 0.972549081f, 1.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Gold = { 1.0f, 0.843137324f, 0.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Goldenrod = { 0.854902029f, 0.647058845f, 0.125490203f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Gray = { 0.501960814f, 0.501960814f, 0.501960814f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Green = { 0.0f, 0.501960814f, 0.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color GreenYellow = { 0.678431392f, 1.0f, 0.184313729f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Honeydew = { 0.941176534f, 1.0f, 0.941176534f, 1.0f };
        extern const ALIMER_SELECT_ANY Color HotPink = { 1.0f, 0.411764741f, 0.705882370f, 1.0f };
        extern const ALIMER_SELECT_ANY Color IndianRed = { 0.803921640f, 0.360784322f, 0.360784322f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Indigo = { 0.294117659f, 0.0f, 0.509803951f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Ivory = { 1.0f, 1.0f, 0.941176534f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Khaki = { 0.941176534f, 0.901960850f, 0.549019635f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Lavender = { 0.901960850f, 0.901960850f, 0.980392218f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LavenderBlush = { 1.0f, 0.941176534f, 0.960784376f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LawnGreen = { 0.486274540f, 0.988235354f, 0.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LemonChiffon = { 1.0f, 0.980392218f, 0.803921640f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LightBlue = { 0.678431392f, 0.847058892f, 0.901960850f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LightCoral = { 0.941176534f, 0.501960814f, 0.501960814f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LightCyan = { 0.878431439f, 1.0f, 1.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LightGoldenrodYellow = { 0.980392218f, 0.980392218f, 0.823529482f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LightGreen = { 0.564705908f, 0.933333397f, 0.564705908f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LightGray = { 0.827451050f, 0.827451050f, 0.827451050f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LightPink = { 1.0f, 0.713725507f, 0.756862819f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LightSalmon = { 1.0f, 0.627451003f, 0.478431404f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LightSeaGreen = { 0.125490203f, 0.698039234f, 0.666666687f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LightSkyBlue = { 0.529411793f, 0.807843208f, 0.980392218f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LightSlateGray = { 0.466666698f, 0.533333361f, 0.600000024f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LightSteelBlue = { 0.690196097f, 0.768627524f, 0.870588303f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LightYellow = { 1.0f, 1.0f, 0.878431439f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Lime = { 0.0f, 1.0f, 0.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color LimeGreen = { 0.196078449f, 0.803921640f, 0.196078449f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Linen = { 0.980392218f, 0.941176534f, 0.901960850f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Maroon = { 0.501960814f, 0.0f, 0.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color MediumAquamarine = { 0.400000036f, 0.803921640f, 0.666666687f, 1.0f };
        extern const ALIMER_SELECT_ANY Color MediumBlue = { 0.0f, 0.0f, 0.803921640f, 1.0f };
        extern const ALIMER_SELECT_ANY Color MediumOrchid = { 0.729411781f, 0.333333343f, 0.827451050f, 1.0f };
        extern const ALIMER_SELECT_ANY Color MediumPurple = { 0.576470613f, 0.439215720f, 0.858823597f, 1.0f };
        extern const ALIMER_SELECT_ANY Color MediumSeaGreen = { 0.235294133f, 0.701960802f, 0.443137288f, 1.0f };
        extern const ALIMER_SELECT_ANY Color MediumSlateBlue = { 0.482352972f, 0.407843173f, 0.933333397f, 1.0f };
        extern const ALIMER_SELECT_ANY Color MediumSpringGreen = { 0.0f, 0.980392218f, 0.603921592f, 1.0f };
        extern const ALIMER_SELECT_ANY Color MediumTurquoise = { 0.282352954f, 0.819607913f, 0.800000072f, 1.0f };
        extern const ALIMER_SELECT_ANY Color MediumVioletRed = { 0.780392230f, 0.082352944f, 0.521568656f, 1.0f };
        extern const ALIMER_SELECT_ANY Color MidnightBlue = { 0.098039225f, 0.098039225f, 0.439215720f, 1.0f };
        extern const ALIMER_SELECT_ANY Color MintCream = { 0.960784376f, 1.0f, 0.980392218f, 1.0f };
        extern const ALIMER_SELECT_ANY Color MistyRose = { 1.0f, 0.894117713f, 0.882353008f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Moccasin = { 1.0f, 0.894117713f, 0.709803939f, 1.0f };
        extern const ALIMER_SELECT_ANY Color NavajoWhite = { 1.0f, 0.870588303f, 0.678431392f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Navy = { 0.0f, 0.0f, 0.501960814f, 1.0f };
        extern const ALIMER_SELECT_ANY Color OldLace = { 0.992156923f, 0.960784376f, 0.901960850f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Olive = { 0.501960814f, 0.501960814f, 0.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color OliveDrab = { 0.419607878f, 0.556862772f, 0.137254909f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Orange = { 1.0f, 0.647058845f, 0.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color OrangeRed = { 1.0f, 0.270588249f, 0.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Orchid = { 0.854902029f, 0.439215720f, 0.839215755f, 1.0f };
        extern const ALIMER_SELECT_ANY Color PaleGoldenrod = { 0.933333397f, 0.909803987f, 0.666666687f, 1.0f };
        extern const ALIMER_SELECT_ANY Color PaleGreen = { 0.596078455f, 0.984313786f, 0.596078455f, 1.0f };
        extern const ALIMER_SELECT_ANY Color PaleTurquoise = { 0.686274529f, 0.933333397f, 0.933333397f, 1.0f };
        extern const ALIMER_SELECT_ANY Color PaleVioletRed = { 0.858823597f, 0.439215720f, 0.576470613f, 1.0f };
        extern const ALIMER_SELECT_ANY Color PapayaWhip = { 1.0f, 0.937254965f, 0.835294187f, 1.0f };
        extern const ALIMER_SELECT_ANY Color PeachPuff = { 1.0f, 0.854902029f, 0.725490212f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Peru = { 0.803921640f, 0.521568656f, 0.247058839f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Pink = { 1.0f, 0.752941251f, 0.796078503f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Plum = { 0.866666734f, 0.627451003f, 0.866666734f, 1.0f };
        extern const ALIMER_SELECT_ANY Color PowderBlue = { 0.690196097f, 0.878431439f, 0.901960850f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Purple = { 0.501960814f, 0.0f, 0.501960814f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Red = { 1.0f, 0.0f, 0.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color RosyBrown = { 0.737254918f, 0.560784340f, 0.560784340f, 1.0f };
        extern const ALIMER_SELECT_ANY Color RoyalBlue = { 0.254901975f, 0.411764741f, 0.882353008f, 1.0f };
        extern const ALIMER_SELECT_ANY Color SaddleBrown = { 0.545098066f, 0.270588249f, 0.074509807f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Salmon = { 0.980392218f, 0.501960814f, 0.447058856f, 1.0f };
        extern const ALIMER_SELECT_ANY Color SandyBrown = { 0.956862807f, 0.643137276f, 0.376470625f, 1.0f };
        extern const ALIMER_SELECT_ANY Color SeaGreen = { 0.180392161f, 0.545098066f, 0.341176480f, 1.0f };
        extern const ALIMER_SELECT_ANY Color SeaShell = { 1.0f, 0.960784376f, 0.933333397f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Sienna = { 0.627451003f, 0.321568638f, 0.176470593f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Silver = { 0.752941251f, 0.752941251f, 0.752941251f, 1.0f };
        extern const ALIMER_SELECT_ANY Color SkyBlue = { 0.529411793f, 0.807843208f, 0.921568692f, 1.0f };
        extern const ALIMER_SELECT_ANY Color SlateBlue = { 0.415686309f, 0.352941185f, 0.803921640f, 1.0f };
        extern const ALIMER_SELECT_ANY Color SlateGray = { 0.439215720f, 0.501960814f, 0.564705908f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Snow = { 1.0f, 0.980392218f, 0.980392218f, 1.0f };
        extern const ALIMER_SELECT_ANY Color SpringGreen = { 0.0f, 1.0f, 0.498039246f, 1.0f };
        extern const ALIMER_SELECT_ANY Color SteelBlue = { 0.274509817f, 0.509803951f, 0.705882370f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Tan = { 0.823529482f, 0.705882370f, 0.549019635f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Teal = { 0.0f, 0.501960814f, 0.501960814f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Thistle = { 0.847058892f, 0.749019623f, 0.847058892f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Tomato = { 1.0f, 0.388235331f, 0.278431386f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Transparent = { 0.0f, 0.0f, 0.0f, 0.0f };
        extern const ALIMER_SELECT_ANY Color Turquoise = { 0.250980407f, 0.878431439f, 0.815686345f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Violet = { 0.933333397f, 0.509803951f, 0.933333397f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Wheat = { 0.960784376f, 0.870588303f, 0.701960802f, 1.0f };
        extern const ALIMER_SELECT_ANY Color White = { 1.0f, 1.0f, 1.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color WhiteSmoke = { 0.960784376f, 0.960784376f, 0.960784376f, 1.0f };
        extern const ALIMER_SELECT_ANY Color Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
        extern const ALIMER_SELECT_ANY Color YellowGreen = { 0.603921592f, 0.803921640f, 0.196078449f, 1.0f };
    }
}
