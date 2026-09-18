// SPDX-License-Identifier: GPL-2.0
//
// tac3_grid.hpp — TAC3 per-layer SQUARE grid linkage (subject-medium topology).
//
// Motivation
// ----------
// A TAC3 layer tracks TAC3_MAX_REGIONS (4096) wear regions. Historically a
// layer was a flat, unlinked array; a file mapped to one region with no
// relationship to its neighbors. This module adds the "subject medium": files
// are treated as cells laid out on a SQUARE grid rather than a 1-D line, so a
// cell's neighbors — and therefore its recovery/verification surface — are
// maximized for the same region count.
//
//   * ASSAILABLE and FIRM: a cell may be worn/attacked (assailable) yet remain
//     recoverable (firm) because it is linked to low-cost neighbors that can
//     re-supply it, in addition to TAC3's N-way layer redundancy.
//   * LOGARITHMIC FALLOFF COST: link cost grows ~ log2(1 + distance), so near
//     neighbors are cheap and distant links are affordable but never free.
//   * DEGREE > 2 IS RARE: soft cap 2 (ordinary); a 3rd/4th link is allowed only
//     with a rarity surcharge and is counted, so high degree stays rare by
//     construction. Hard cap 4 keeps per-cell storage fixed and cache-tight.
//   * 4- AND 8-CONNECTIVITY: both orthogonal (N/S/E/W) and diagonal neighbor
//     enumeration are supported per call.
//
// Memory & search discipline
// ---------------------------
//   * The grid is IMPLICIT. With side = 64 (64*64 = 4096), a region id IS its
//     coordinate: row = id >> 6, col = id & 63. Neighbor positions are O(1)
//     bit-math; topology costs ZERO stored bytes.
//   * Only explicit links are stored, in a FIXED inline array per cell (no
//     per-cell heap allocation, no pointer chasing). A cell's links sit
//     contiguously with its degree in one small record -> one cache line,
//     <= O(degree<=4) linear scan.
//
// This is a portable header (userspace: simulation/testing/tooling). Keep the
// kernel copy (fs/tac3) in sync when the model changes, per the repo convention.
//
// Copyright (C) 2026 MEARVK LLC
// Author: Maximilian Eric Alexander Rupplin von Keffikon
//
#ifndef TAC3_GRID_HPP
#define TAC3_GRID_HPP

#include "tac3.hpp"

#include <cstdint>
#include <vector>
#include <cstddef>

namespace tac3 {

// ---- Grid geometry -------------------------------------------------------
// 4096 regions == 64 x 64 exactly. Stored as a compile-time constant so the
// id<->coordinate mapping is pure shift/mask.
inline constexpr std::uint32_t kGridSide  = 64;              // sqrt(kMaxRegions)
inline constexpr std::uint32_t kGridShift = 6;               // log2(64)
inline constexpr std::uint32_t kGridMask  = kGridSide - 1;   // 63
static_assert(kGridSide * kGridSide == kMaxRegions,
              "TAC3 grid side must satisfy side*side == kMaxRegions");

// ---- Link degree policy --------------------------------------------------
inline constexpr std::uint8_t kLinkSoftCap = 2;  // ordinary max (>2 is "rare")
inline constexpr std::uint8_t kLinkHardCap = 4;  // fixed inline storage ceiling

// ---- Cost model ----------------------------------------------------------
// cost(a,b) = round(kLinkCostK * log2(1 + distance)); distance is Chebyshev for
// 8-connectivity (a diagonal is one step) and orthogonal for 4-connectivity.
// A 3rd/4th (rare) link pays an additive surcharge on top of the log cost.
inline constexpr std::uint32_t kLinkCostK        = 100; // per-mille style scale
inline constexpr std::uint32_t kRaritySurcharge  = 250; // added to links #3, #4

enum class Connectivity : std::uint8_t { Four = 4, Eight = 8 };

// ---- Per-cell link record (fixed, inline, allocation-free) ---------------
struct Tac3Link {
    std::uint16_t neighbor = 0; // region id of the linked neighbor
    std::uint16_t cost     = 0; // resolved link cost (log falloff [+ surcharge])
};

struct Tac3CellLinks {
    std::uint8_t degree       = 0;                 // 0 .. kLinkHardCap
    std::uint8_t connectivity = 0;                 // 4 or 8 (how last enumerated)
    Tac3Link     link[kLinkHardCap]{};             // contiguous inline links
};

// ---- Pure geometry helpers (O(1) bit-math; no storage) -------------------
inline std::uint32_t grid_row(std::uint32_t id) noexcept { return id >> kGridShift; }
inline std::uint32_t grid_col(std::uint32_t id) noexcept { return id & kGridMask; }
inline std::uint32_t grid_id(std::uint32_t row, std::uint32_t col) noexcept {
    return (row << kGridShift) | col;
}

// Integer log2 floor of (1 + d): log2(1)=0, log2(2)=1, log2(3)=1, log2(4)=2 ...
inline std::uint32_t ilog2_1p(std::uint32_t d) noexcept {
    std::uint32_t v = d + 1u, r = 0;
    while (v > 1u) { v >>= 1; ++r; }
    return r;
}

// Chebyshev distance (8-connectivity metric) and orthogonal (4-connectivity).
inline std::uint32_t chebyshev(std::uint32_t a, std::uint32_t b) noexcept {
    std::uint32_t dr = grid_row(a) > grid_row(b) ? grid_row(a) - grid_row(b)
                                                 : grid_row(b) - grid_row(a);
    std::uint32_t dc = grid_col(a) > grid_col(b) ? grid_col(a) - grid_col(b)
                                                 : grid_col(b) - grid_col(a);
    return dr > dc ? dr : dc;
}
inline std::uint32_t manhattan(std::uint32_t a, std::uint32_t b) noexcept {
    std::uint32_t dr = grid_row(a) > grid_row(b) ? grid_row(a) - grid_row(b)
                                                 : grid_row(b) - grid_row(a);
    std::uint32_t dc = grid_col(a) > grid_col(b) ? grid_col(a) - grid_col(b)
                                                 : grid_col(b) - grid_col(a);
    return dr + dc;
}

// Resolved link cost: log falloff on the metric matching `conn`, plus the
// rarity surcharge when this would be the cell's 3rd or later link.
inline std::uint32_t link_cost(std::uint32_t a, std::uint32_t b,
                               Connectivity conn,
                               std::uint8_t existing_degree) noexcept {
    std::uint32_t dist = (conn == Connectivity::Eight) ? chebyshev(a, b)
                                                       : manhattan(a, b);
    std::uint32_t base = kLinkCostK * ilog2_1p(dist);
    if (existing_degree >= kLinkSoftCap) base += kRaritySurcharge;
    return base;
}

// =========================================================================
// Tac3LayerGrid — the square linkage over ONE layer's regions.
// One instance corresponds to the linkage state of one Tac3LayerHealth layer.
// Memory: kMaxRegions * sizeof(Tac3CellLinks) (~4096 * 20 B ~= 80 KiB/layer),
// allocated once, contiguously; no per-cell dynamic allocation.
// =========================================================================
class Tac3LayerGrid {
public:
    Tac3LayerGrid() : cells_(kMaxRegions) {}

    static constexpr std::uint32_t side()    noexcept { return kGridSide; }
    static constexpr std::uint32_t regions() noexcept { return kMaxRegions; }

    const Tac3CellLinks& cell(std::uint32_t id) const { return cells_.at(id); }

    // Enumerate the up-to-4 (orthogonal) or up-to-8 (with diagonals) grid
    // neighbor ids of `id`. Returns the count; fills out[] (caller sizes >= 8).
    static std::uint32_t neighbors(std::uint32_t id, Connectivity conn,
                                   std::uint32_t out[8]) noexcept {
        std::uint32_t r = grid_row(id), c = grid_col(id), n = 0;
        const bool up = r > 0, down = r + 1 < kGridSide;
        const bool left = c > 0, right = c + 1 < kGridSide;
        if (up)    out[n++] = grid_id(r - 1, c);
        if (down)  out[n++] = grid_id(r + 1, c);
        if (left)  out[n++] = grid_id(r, c - 1);
        if (right) out[n++] = grid_id(r, c + 1);
        if (conn == Connectivity::Eight) {
            if (up   && left)  out[n++] = grid_id(r - 1, c - 1);
            if (up   && right) out[n++] = grid_id(r - 1, c + 1);
            if (down && left)  out[n++] = grid_id(r + 1, c - 1);
            if (down && right) out[n++] = grid_id(r + 1, c + 1);
        }
        return n;
    }

    // Try to add a link a<->b (undirected pair). Returns true on success.
    // Enforces the hard cap; sets the surcharged cost when degree >= soft cap;
    // rejects self-links, out-of-range ids, and duplicates.
    bool add_link(std::uint32_t a, std::uint32_t b, Connectivity conn) {
        if (a == b || a >= kMaxRegions || b >= kMaxRegions) return false;
        if (!link_one(a, b, conn)) return false;
        if (!link_one(b, a, conn)) { unlink_one(a, b); return false; }
        return true;
    }

    // Convenience: link a cell to its cheapest available grid neighbors until it
    // reaches the soft cap (2). Returns how many links were made. This is the
    // "ordinary" wiring path -- it never exceeds the soft cap on its own.
    std::uint32_t link_to_neighbors(std::uint32_t id, Connectivity conn) {
        std::uint32_t nb[8];
        std::uint32_t k = neighbors(id, conn, nb);
        std::uint32_t made = 0;
        for (std::uint32_t i = 0; i < k && cells_[id].degree < kLinkSoftCap; ++i)
            if (add_link(id, nb[i], conn)) ++made;
        return made;
    }

    // ---- statistics (for reporting how rare high degree is) -------------
    struct Stats {
        std::uint32_t linked_cells   = 0;   // cells with degree >= 1
        std::uint32_t degree_hist[kLinkHardCap + 1]{}; // [0..hardcap]
        std::uint32_t rare_cells     = 0;   // degree > soft cap
        std::uint64_t total_links    = 0;   // undirected links (counted once)
        std::uint64_t total_cost     = 0;   // sum of resolved link costs (dir.)
    };
    Stats stats() const {
        Stats s;
        for (const auto& c : cells_) {
            s.degree_hist[c.degree]++;
            if (c.degree >= 1) s.linked_cells++;
            if (c.degree > kLinkSoftCap) s.rare_cells++;
            for (std::uint8_t i = 0; i < c.degree; ++i)
                s.total_cost += c.link[i].cost;
        }
        // each undirected link is stored on both endpoints -> halve.
        std::uint64_t dir = 0;
        for (const auto& c : cells_) dir += c.degree;
        s.total_links = dir / 2;
        return s;
    }

private:
    bool link_one(std::uint32_t from, std::uint32_t to, Connectivity conn) {
        Tac3CellLinks& cl = cells_[from];
        if (cl.degree >= kLinkHardCap) return false;         // hard cap
        for (std::uint8_t i = 0; i < cl.degree; ++i)
            if (cl.link[i].neighbor == to) return false;     // duplicate
        std::uint32_t cost = link_cost(from, to, conn, cl.degree);
        cl.link[cl.degree].neighbor = static_cast<std::uint16_t>(to);
        cl.link[cl.degree].cost     = static_cast<std::uint16_t>(
            cost > 0xffffu ? 0xffffu : cost);
        cl.connectivity = static_cast<std::uint8_t>(conn);
        cl.degree++;
        return true;
    }
    void unlink_one(std::uint32_t from, std::uint32_t to) {
        Tac3CellLinks& cl = cells_[from];
        for (std::uint8_t i = 0; i < cl.degree; ++i) {
            if (cl.link[i].neighbor == to) {
                cl.link[i] = cl.link[cl.degree - 1];
                cl.link[cl.degree - 1] = Tac3Link{};
                cl.degree--;
                return;
            }
        }
    }

    std::vector<Tac3CellLinks> cells_; // one contiguous block, sized once
};

} // namespace tac3

#endif // TAC3_GRID_HPP
