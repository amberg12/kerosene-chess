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

#include <array>
#include <string_view>

#include "move.hpp"
#include "types.hpp"
#include "util/bits.hpp"

namespace kerosene {

class PieceId {
public:
    explicit constexpr PieceId() = default;

    explicit constexpr PieceId(u8 raw) :
        m_raw(raw) {
    }

    static constexpr auto king() -> PieceId {
        return PieceId{0};
    }

    explicit operator usize() const {
        return m_raw;
    }

    friend constexpr auto operator==(const PieceId&, const PieceId&) -> bool = default;

private:
    friend class PieceMask;

    [[nodiscard]] constexpr auto to_bit_position() const -> u16 {
        return 1 << m_raw;
    }

    u8 m_raw{0x10};
};

static_assert(sizeof(PieceId) == sizeof(u8));

class PieceMask {
public:
    static constexpr usize kNb = 16;

    constexpr PieceMask() = default;

    explicit constexpr PieceMask(u16 raw) :
        m_raw(raw) {
    }

    constexpr auto set_id(PieceId id) -> void {
        m_raw |= id.to_bit_position();
    }

    constexpr auto unset_id(PieceId id) -> void {
        m_raw &= ~id.to_bit_position();
    }

    friend constexpr auto operator~(const PieceMask piece_mask) -> PieceMask {
        return PieceMask{static_cast<u16>(~piece_mask.m_raw)};
    }

    constexpr auto operator==(const PieceMask&) const -> bool = default;

    struct Iterator {
        auto operator++() -> Iterator& {
            m_raw = clear_lsb(m_raw);
            return *this;
        }

        auto operator*() const -> PieceId {
            return PieceId{static_cast<u8>(std::countr_zero(m_raw))};
        }

        friend constexpr auto operator==(const Iterator&, const Iterator&) -> bool = default;

    private:
        friend class PieceMask;

        Iterator(u16 raw) :
            m_raw(raw) {
        }

        u16 m_raw{};
    };

    [[nodiscard]] auto begin() const -> Iterator {
        return Iterator{m_raw};
    }

    [[nodiscard]] static auto end() -> Iterator {
        return Iterator{0};
    }

private:
    u16 m_raw{0};
};

class Tile {
    static constexpr u8 kEmpty     = 0b0000'0000;
    static constexpr u8 kPieceMask = 0b0000'1111;
    static constexpr u8 kIdMask    = 0b1111'0000;

public:
    constexpr Tile() = default;

    constexpr Tile(PieceId id, Piece piece) {
        pack(id, piece);
    }

    [[nodiscard]] constexpr auto unpack() const -> std::pair<PieceId, Piece> {
        u8 id_bits    = (m_raw & kIdMask) >> 4;
        u8 piece_bits = (m_raw & kPieceMask);

        return {PieceId{id_bits}, Piece{static_cast<Piece::Underlying>(piece_bits)}};
    }

private:
    constexpr auto pack(PieceId id, Piece piece) -> void {
        u8 id_bits    = std::bit_cast<u8>(id);
        u8 piece_bits = std::bit_cast<u8>(piece);

        m_raw = id_bits << 4 | piece_bits;
    }

    u8 m_raw{};
};

class alignas(64) ByteBoard {
public:
    constexpr ByteBoard() = default;

    [[nodiscard]] constexpr auto operator[](Square square) -> Tile& {
        return m_byte_board[square];
    }

    [[nodiscard]] constexpr auto operator[](Square square) const -> const Tile& {
        return m_byte_board[square];
    }

private:
    std::array<Tile, 64> m_byte_board;
};

class alignas(64) WordBoard {
public:
    constexpr WordBoard() = default;

private:
    std::array<PieceMask, 64> m_word_board;
};

class PieceList {
public:
    constexpr PieceList() = default;

    auto add_piece(Square at, Piece to) -> PieceId;
    auto move_piece(PieceId piece_id, Square dst) -> void;
    auto delete_piece(PieceId piece_id) -> void;
    auto mutate_piece(PieceId piece_id, PieceType to) -> void;

private:
    PieceMask                             m_piece_mask{};
    std::array<Square, PieceMask::kNb>    m_squares;
    std::array<PieceType, PieceMask::kNb> m_piece_types;
};

class CastlingRights {
public:
    enum Underlying : u8 {
        kWhiteKSide = 0b0001,
        kWhiteQSide = 0b0010,
        kWhite      = kWhiteKSide | kWhiteQSide,
        kBlackKSide = 0b0100,
        kBlackQSide = 0b1000,
        kBlack      = kBlackKSide | kBlackQSide,
    };

    /* implicit */ CastlingRights(Underlying raw) :
        m_raw(raw) {
    }

    /* implicit */ operator Underlying() const {
        return static_cast<Underlying>(m_raw);
    }

    static constexpr auto none() -> CastlingRights {
        return CastlingRights{static_cast<Underlying>(0)};
    }

    static constexpr auto from_color(Color color) -> CastlingRights {
        return color == Color::kWhite ? kWhite : kBlack;
    }

    constexpr void add_casting_rights(CastlingRights castling_rights) {
        m_raw |= castling_rights;
    }

    constexpr void del_castling_rights(CastlingRights castling_rights) {
        m_raw &= ~castling_rights;
    }

    [[nodiscard]] auto to_string() const -> std::string {
        std::string out;

        if (m_raw & kWhiteKSide) {
            out += "K";
        }

        if (m_raw & kWhiteQSide) {
            out += "Q";
        }

        if (m_raw & kBlackKSide) {
            out += "k";
        }

        if (m_raw & kBlackQSide) {
            out += "q";
        }

        return out.empty() ? "-" : out;
    }

private:
    u8 m_raw{};
};

class Position {
public:
    Position(const Position& parent, Move move);

    static auto parse(std::string_view fen) -> Position;

    [[nodiscard]] auto piece_at(Square at) const -> Piece;
    [[nodiscard]] auto tile_at(Square at) const -> Tile;

    [[nodiscard]] auto to_string() const -> std::string;

    [[nodiscard]] auto castling_rights() const -> const CastlingRights&;
    [[nodiscard]] auto en_passant() const -> Square;

private:
    Position() = default;

    auto make_move(Move move) -> void;

    auto add_piece(Square at, Piece to) -> void;
    auto move_piece(Square src, Square dst) -> void;
    auto delete_piece(Square at) -> void;
    auto mutate_piece(Square at, PieceType to) -> void;

    ColorMap<WordBoard> m_attack_table{};
    ByteBoard           m_mail_box{};
    ColorMap<PieceList> m_piece_list{};

    Color          m_side_to_move{Color::kWhite};
    u8             m_move_rule{0};
    CastlingRights m_castling_rights{CastlingRights::none()};
    Square         m_en_passant_target_square{Square::kInvalid};
};

}  // kerosene
