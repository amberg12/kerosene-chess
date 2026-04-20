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

#include <sstream>
#include <string>

#include "position.hpp"

namespace kerosene {

auto Position::parse(std::string_view fen) -> Position {
    Position out{};

    std::string        input{fen};
    std::istringstream is{input};

    std::string board, color, castling_rights, en_passant, move_rule;
    is >> board >> color >> castling_rights >> en_passant >> move_rule;

    u8 file = 0, rank = 7;
    for (char c : board) {
        if (c == '/') {
            --rank;
            file = 0;
            continue;
        }

        if (std::isdigit(c)) {
            file += c - '0';
        } else {
            Piece  to = Piece::parse(c);
            Square at = Square{file, rank};

            out.add_tile(at, to);

            ++file;
        }
    }

    out.m_side_to_move = Color::parse(color);

    for (char c : castling_rights) {
        switch (c) {
        case 'K':
            out.m_castling_rights.add_casting_rights(CastlingRights::kWhiteKSide);
            break;
        case 'k':
            out.m_castling_rights.add_casting_rights(CastlingRights::kBlackKSide);
            break;
        case 'Q':
            out.m_castling_rights.add_casting_rights(CastlingRights::kWhiteQSide);
            break;
        case 'q':
            out.m_castling_rights.add_casting_rights(CastlingRights::kBlackQSide);
            break;
        default:
            break;
        }
    }

    out.m_en_passant_target_square = Square::parse(en_passant).value_or(Square::kInvalid);

    out.m_move_rule = std::stoi(move_rule);

    return out;
}

auto Position::tile_at(Square at) const -> Piece {
    return m_mailbox[at];
}

auto Position::to_string() const -> std::string {
    std::string out;

    for (u8 rank = 8; rank-- > 0;) {
        out += "+-+-+-+-+-+-+-+-+\n";
        for (u8 file = 0; file < 8; ++file) {
            Square at{file, rank};

            out += "|";
            out += tile_at(at).to_string();
        }
        out += "|\n";
    }

    out += "+-+-+-+-+-+-+-+-+";

    return out;
}

auto Position::castling_rights() const -> const CastlingRights& {
    return m_castling_rights;
}

auto Position::add_tile(Square at, Piece to) -> void {
    m_mailbox[at] = to;
}

}  // kerosene
