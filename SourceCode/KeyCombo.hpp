/* =============================================================================
 *
 * Исходный код в этом файле частично или полностью заимствован мной из проекта
 * hawck: https://github.com/snyball/hawck/tree/af318e24e63fb1ea87cb0080fe25ee293e5f2dd0
 *
 * BSD 2-Clause License
 *
 * Copyright (c) 2019, Jonas Møller
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * =============================================================================
 */

#pragma once

#include "KBDAction.hpp"
#include "KeyValue.hpp"
#include <assert.h>
#include <limits.h>
#include <vector>

#include <iostream>

struct KeyCombo
{
        std::vector<int> key_seq;
        int              activator;
        int              num_seq;

        inline KeyCombo(const std::vector<int> &seq) noexcept
        {
                assert(seq.size() < INT_MAX);
                key_seq   = seq;
                activator = key_seq[key_seq.size() - 1];
                key_seq.pop_back();
        }

        inline bool check(const KBDAction &action) noexcept
        {
                if (action.ev.value == KEY_VAL_REPEAT)
                        return false;

                if (num_seq == ((int)key_seq.size()) && action.ev.code == activator &&
                    action.ev.value == KEY_VAL_DOWN) {
                        num_seq = 0;
                        return true;
                }

                for (auto k : key_seq) {
                        if (k == action.ev.code) {
                                num_seq += (action.ev.value == KEY_VAL_DOWN) ? 1 : -1;
                                break;
                        }
                }

                if (num_seq < 0)
                        num_seq = 0;

                return false;
        }
};

struct KeyComboToggle : public KeyCombo
{
        bool active = false;

        inline KeyComboToggle(const std::vector<int> &seq) noexcept : KeyCombo(seq) {}

        inline bool check(const KBDAction &action) noexcept
        {
                if (KeyCombo::check(action))
                        active = !active;
                return active;
        }
};
