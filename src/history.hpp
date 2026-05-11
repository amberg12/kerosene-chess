/*
  Kerosene - A UCI chess engine.
  Copyright (C) 2026 Amber Goulding

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include "move.hpp"
#include "position.hpp"
#include "util/integer_types.hpp"


#include <memory>

namespace kerosene {

class History {
public:
    auto update_quiet_history(const Position& pos, i32 depth, Move move) const -> void {
        i16& entry = get_piece_to_entry(pos, move);
        update_entry(entry, bonus(depth));
    }

    auto read_quiet_history(const Position& pos, Move move) {
        return get_piece_to_entry(pos, move);
    }

private:
    static constexpr i16 kHistoryMax = 16384;
    static constexpr i16 kHistoryMin = -16384;

    static constexpr auto bonus(i32 depth) -> i16 {
        return static_cast<i16>(std::clamp(320 * depth - 400, 0, 2400));
    }

    static constexpr auto update_entry(i16& entry, i16 bonus) -> void {
        i16 clamped_bonus = std::clamp(bonus, kHistoryMin, kHistoryMax);
        entry += clamped_bonus - entry * std::abs(clamped_bonus) / kHistoryMax;
    }

    auto get_piece_to_entry(const Position& pos, Move move) const -> i16& {
        return (*m_quiet_piece_table)[move.dst()][pos.piece_at(move.src()).compressed_idx()];
    }

    using PieceTo = std::array<std::array<i16, 12>, 64>;

    std::unique_ptr<PieceTo> m_quiet_piece_table = std::make_unique<PieceTo>();
};

}  // namespace kerosene
