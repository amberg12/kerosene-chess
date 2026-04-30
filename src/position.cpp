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

auto PieceList::add_piece(Square at, Piece to) -> PieceId {
    PieceId piece_id = [&] {
        if (to.piece_type() == PieceType::kKing) {
            return PieceId::king();
        }

        for (PieceId id : ~m_piece_mask) {
            if (id == PieceId::king()) {
                continue;
            }

            return id;
        }

        std::unreachable();
    }();

    m_squares[static_cast<usize>(piece_id)]     = at;
    m_piece_types[static_cast<usize>(piece_id)] = to.piece_type();
    m_piece_mask.set_id(piece_id);

    return piece_id;
}

auto PieceList::move_piece(PieceId piece_id, Square dst) -> void {
    m_squares[static_cast<usize>(piece_id)] = dst;
}

auto PieceList::delete_piece(PieceId piece_id) -> void {
    m_piece_mask.unset_id(piece_id);
}

auto PieceList::mutate_piece(PieceId piece_id, PieceType to) -> void {
    m_piece_types[static_cast<usize>(piece_id)] = to;
}

Position::Position(const Position& parent, Move move) :
    m_mail_box(parent.m_mail_box),
    m_piece_list(parent.m_piece_list),
    m_side_to_move(parent.m_side_to_move),
    m_move_rule(parent.m_move_rule),
    m_castling_rights(parent.m_castling_rights),
    m_en_passant_target_square(parent.m_en_passant_target_square) {
    make_move(move);
}

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

            out.add_piece(at, to);

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

auto Position::piece_at(Square at) const -> Piece {
    return tile_at(at).unpack().second;
}

auto Position::tile_at(Square at) const -> Tile {
    return m_mail_box[at];
}

auto Position::to_string() const -> std::string {
    std::string out;

    for (u8 rank = 8; rank-- > 0;) {
        out += "+-+-+-+-+-+-+-+-+\n";
        for (u8 file = 0; file < 8; ++file) {
            Square at{file, rank};

            out += "|";
            out += piece_at(at).to_string();
        }
        out += "|\n";
    }

    out += "+-+-+-+-+-+-+-+-+";

    return out;
}

auto Position::castling_rights() const -> const CastlingRights& {
    return m_castling_rights;
}

auto Position::en_passant() const -> Square {
    return m_en_passant_target_square;
}

auto Position::make_move(Move move) -> void {
    switch (move.special_type()) {
    case Move::kNormal: {
        // TODO: consider double pushes for en passant square creation.
        Piece target = piece_at(move.dst());

        if (!target.empty()) {
            delete_piece(move.dst());
        }

        move_piece(move.src(), move.dst());
        break;
    }
    case Move::kPromotion: {
        Piece target = piece_at(move.dst());

        if (!target.empty()) {
            delete_piece(move.dst());
        }

        move_piece(move.src(), move.dst());
        mutate_piece(move.dst(), move.promotes_to());
        break;
    }
    case Move::kEnPassant: {
        Square en_passant_pawn{move.dst().file(), move.src().rank()};

        move_piece(move.src(), move.dst());
        delete_piece(en_passant_pawn);
        break;
    }
    case Move::kCastling: {
    }
    }

    if (piece_at(move.src()).piece_type() == PieceType::kKing) {
        m_castling_rights.del_castling_rights(CastlingRights::from_color(m_side_to_move));
    }

    if (move.src() == Square::kH1 || move.dst() == Square::kH1) {
        m_castling_rights.del_castling_rights(CastlingRights::kWhiteKSide);
    }

    if (move.src() == Square::kA1 || move.dst() == Square::kA1) {
        m_castling_rights.del_castling_rights(CastlingRights::kWhiteQSide);
    }

    if (move.src() == Square::kH8 || move.dst() == Square::kH8) {
        m_castling_rights.del_castling_rights(CastlingRights::kBlackKSide);
    }

    if (move.src() == Square::kA8 || move.dst() == Square::kA8) {
        m_castling_rights.del_castling_rights(CastlingRights::kBlackQSide);
    }

    bool is_pawn_move = piece_at(move.src()).piece_type() == PieceType::kPawn;
    bool is_capture =
      !piece_at(move.dst()).empty() && piece_at(move.dst()).color() != m_side_to_move;

    if (is_pawn_move || is_capture) {
        m_move_rule = 0;
    } else {
        ++m_move_rule;
    }

    m_side_to_move = ~m_side_to_move;
}

auto Position::add_piece(Square at, Piece to) -> void {
    PieceId piece_id = m_piece_list[to.color()].add_piece(at, to);
    Tile    tile{piece_id, to};

    m_mail_box[at] = tile;
}

auto Position::move_piece(Square src, Square dst) -> void {
    Tile tile = tile_at(src);
    auto [piece_id, piece] = tile.unpack();

    m_mail_box[dst] = tile;
    m_mail_box[src] = Tile{};
    m_piece_list[piece.color()].move_piece(piece_id, dst);
}

auto Position::delete_piece(Square at) -> void {
    auto [piece_id, piece] = tile_at(at).unpack();

    m_piece_list[piece.color()].delete_piece(piece_id);
    m_mail_box[at] = Tile{};
}

auto Position::mutate_piece(Square at, PieceType to) -> void {
    auto [piece_id, piece] = tile_at(at).unpack();

    m_piece_list[piece.color()].mutate_piece(piece_id, to);
    m_mail_box[at] = Tile{piece_id, {piece.color(), to}};
}

}  // kerosene
