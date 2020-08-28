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

#include "platform/platform.h"
#include <deque>

namespace Alimer
{
    namespace platform
    {
        namespace
        {
            std::deque<Event>& get_event_queue() noexcept
            {
                static std::deque<Event> event_queue;
                return event_queue;
            }

            
            bool pop_event(Event& e) noexcept
            {
                auto& event_queue = get_event_queue();
                // Pop the first event of the queue, if it is not empty
                if (!event_queue.empty())
                {
                    e = event_queue.front();
                    event_queue.pop_front();
                    return true;
                }

                return false;
            }
        }

        /* Implemented by platform */
        extern void pump_events() noexcept;

        void push_event(const Event& e)
        {
            get_event_queue().emplace_back(e);
        }

        void push_event(Event&& e)
        {
            get_event_queue().emplace_back(std::move(e));
        }

        bool poll_event(Event& e) noexcept
        {
            pump_events();

            return pop_event(e);
        }
    }
}
