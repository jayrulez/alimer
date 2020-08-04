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

#pragma once

#include "Core/Object.h"
#include <EASTL/vector.h>
#include <EASTL/bitset.h>

namespace alimer
{
    /// Defines input key modifiers.
    enum class ModifierKeys : uint8_t
    {
        None = 0,
        Alt = 0x01,
        Control = 0x02,
        Shift = 0x04,
        Meta = 0x08,
        Count
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(ModifierKeys, uint8_t);

    /// Defines input mouse buttons.
    enum class MouseButton
    {
        Left,
        Right,
        Middle,
        XButton1,
        XButton2,
        Count
    };

    class ALIMER_API Input final : public Object
    {
        ALIMER_OBJECT(Input, Object);

    public:
        void Initialize();
        void Update();

        bool IsMouseButtonHeld(MouseButton button);
        bool IsMouseButtonDown(MouseButton button);
        bool IsMouseButtonUp(MouseButton button);

        // Event posting.
        void PostMousePressEvent(int x, int y, MouseButton button, ModifierKeys modifiers, bool down);

    private:
        class ActionState
        {
        public:
            ActionState() = default;
            void Initialize(uint32_t slotCount);
            void Update();
            void PostEvent(uint32_t slot, bool down, ModifierKeys modifiers = ModifierKeys::None);
            bool IsActive(uint32_t slot, ModifierKeys modifiers = ModifierKeys::None) const;
            bool IsUp(uint32_t slot, ModifierKeys modifiers = ModifierKeys::None) const;
            bool IsDown(uint32_t slot, ModifierKeys modifiers = ModifierKeys::None) const;
            bool IsHeld(uint32_t slot, ModifierKeys modifiers = ModifierKeys::None) const;

        private:
            struct ActionSlot
            {
                enum class Bits : uint32_t
                {
                    Up,
                    Down,
                    Held,
                    Count
                };

                ModifierKeys modifiers{ ModifierKeys::None };
                eastl::bitset<static_cast<uint32_t>(Bits::Count)> bits;
            };

            bool Test(uint32_t slot, ModifierKeys modifiers, ActionSlot::Bits bit) const;
            bool dirty{ false };
            eastl::vector<ActionSlot> actionSlots;
        };

        int32_t mousePositionX;
        int32_t mousePositionY;
        ActionState mouseButtons;
    };
}
