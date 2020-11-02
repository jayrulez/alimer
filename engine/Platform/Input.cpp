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

#include "Platform/Input.h"

namespace alimer
{
    namespace
    {
        bool IsModifierOnlyButton(uint32_t button)
        {
            return button == 0;
        }

        bool IsModifierMatch(uint32_t slot, ModifierKeys modifiers, ModifierKeys stateModifiers)
        {
            const bool areModifiersActive     = (modifiers & stateModifiers) == modifiers;
            const bool areOnlyModifiersActive = modifiers == stateModifiers;
            return IsModifierOnlyButton(slot) ? areModifiersActive : areOnlyModifiersActive;
        }
    }

    void Input::Initialize()
    {
        mouseButtons.Initialize(static_cast<uint32_t>(MouseButton::Count));
    }

    void Input::Update()
    {
        //_keys.update();
        mouseButtons.Update();
        /*for (GamepadState& gamepad : _gamepads)
        {
            gamepad.buttons.update();
        }
        _mouseScroll = 0;
        _previousMousePosition = _mousePosition;*/
    }

    bool Input::IsMouseButtonHeld(MouseButton button)
    {
        return mouseButtons.IsHeld(static_cast<uint32_t>(button));
    }

    bool Input::IsMouseButtonDown(MouseButton button)
    {
        return mouseButtons.IsDown(static_cast<uint32_t>(button));
    }

    bool Input::IsMouseButtonUp(MouseButton button)
    {
        return mouseButtons.IsUp(static_cast<uint32_t>(button));
    }

    void Input::PostMousePressEvent(int x, int y, MouseButton button, ModifierKeys modifiers, bool down)
    {
        mousePositionX = x;
        mousePositionY = y;
        mouseButtons.PostEvent(static_cast<uint32_t>(button), down, modifiers);
    }

    /* Input::ActionState */
    void Input::ActionState::Initialize(uint32_t buttonCount)
    {
        actionSlots.resize(buttonCount);
    }

    void Input::ActionState::Update()
    {
        if (dirty)
        {
            for (ActionSlot& slot : actionSlots)
            {
                if (slot.bits.any())
                {
                    slot.bits.reset(static_cast<uint32_t>(ActionSlot::Bits::Down));
                    slot.bits.reset(static_cast<uint32_t>(ActionSlot::Bits::Up));
                    slot.modifiers = slot.bits.test(static_cast<uint32_t>(ActionSlot::Bits::Held)) ? slot.modifiers : ModifierKeys::None;
                }
            }

            dirty = false;
        }
    }

    void Input::ActionState::PostEvent(uint32_t button, bool down, ModifierKeys modifiers)
    {
        ActionSlot& slot                  = actionSlots[button];
        const bool  wasHeld               = slot.bits.test(static_cast<uint32_t>(ActionSlot::Bits::Held));
        const bool  isReleasing           = wasHeld && !down;
        const bool  hasModifiersRemaining = modifiers != ModifierKeys::None;
        const bool  isModifierUpdate      = IsModifierOnlyButton(button) && isReleasing && hasModifiersRemaining;
        if (isModifierUpdate)
        {
            slot.modifiers = modifiers;
        }
        else
        {
            dirty = true;
            slot.bits.set(static_cast<uint32_t>(ActionSlot::Bits::Down), !wasHeld && down);
            slot.bits.set(static_cast<uint32_t>(ActionSlot::Bits::Held), down);
            slot.bits.set(static_cast<uint32_t>(ActionSlot::Bits::Up), wasHeld && !down);
            slot.modifiers = down ? modifiers : slot.modifiers;
        }
    }

    bool Input::ActionState::Test(uint32_t slot, ModifierKeys modifiers, ActionSlot::Bits bit) const
    {
        const ActionSlot& actionSlotstate = actionSlots[slot];
        return actionSlotstate.bits.test(static_cast<uint32_t>(bit)) && IsModifierMatch(slot, modifiers, actionSlotstate.modifiers);
    }

    bool Input::ActionState::IsActive(uint32_t slot, ModifierKeys modifiers) const
    {
        const ActionSlot& actionSlotstate = actionSlots[slot];
        return actionSlotstate.bits.any() && IsModifierMatch(slot, modifiers, actionSlotstate.modifiers);
    }

    bool Input::ActionState::IsUp(uint32_t slot, ModifierKeys modifiers) const
    {
        return Test(slot, modifiers, ActionSlot::Bits::Up);
    }

    bool Input::ActionState::IsDown(uint32_t slot, ModifierKeys modifiers) const
    {
        return Test(slot, modifiers, ActionSlot::Bits::Down);
    }

    bool Input::ActionState::IsHeld(uint32_t slot, ModifierKeys modifiers) const
    {
        return Test(slot, modifiers, ActionSlot::Bits::Held);
    }
}
