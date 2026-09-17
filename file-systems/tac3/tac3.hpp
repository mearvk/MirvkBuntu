// SPDX-License-Identifier: GPL-2.0
//
// tac3.hpp — C++ equivalent of the TAC3 wear/pressure/health engine and its
//            three-table data model.
//
// This is a portable, standalone C++ port of the pure logic in tac3.c/tac3.h.
// The kernel module (tac3.c) is the authoritative, mountable filesystem; it
// depends on Linux VFS APIs and cannot run in userspace. This header re-expresses
// the parts that ARE portable — the N-layer ("multitude") redundant file table,
// the per-region/per-layer wear accounting, and the read QUALITY / PRESSURE and
// disk-health derivations — as a self-contained C++ class that builds and runs
// anywhere. It is intended for simulation, testing, and tooling, and mirrors the
// kernel semantics exactly (same speed ceilings, same per-mille fixed-point
// formulas, same green/white/yellow model, same "jarring spreads impact across
// all N layers" rule).
//
// The three tables map to the kernel structs:
//   Table 1 (FILE)   -> Tac3FileEntry           (n-way redundant file entries)
//   Table 2 (HEALTH) -> Tac3RegionWear / Tac3LayerHealth (wear + disk health)
//   Table 3 (ADMIN)  -> Tac3AdminState          (facts + opaque operator values)
//
// ETHICS (identical to the kernel note): Table 3 stores administrative facts and
// OPERATOR-SUPPLIED opaque values only. Nothing here computes, infers, or judges
// any person's intelligence, worth, feelings, learning, friendships, or standing.
//
// Copyright (C) 2026 MEARVK LLC
// Author: Maximilian Eric Alexander Rupplin von Keffikon
//
#ifndef TAC3_HPP
#define TAC3_HPP

#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <mutex>

namespace tac3 {

// ---- Brand identifiers --------------------------------------------------
inline constexpr const char* kName = "tac3";
inline constexpr std::uint32_t kMagic = 0x54414333u; // "TAC3"

// ---- Multitude (redundancy factor) --------------------------------------
inline constexpr std::uint32_t kMultMin     = 1;
inline constexpr std::uint32_t kMultDefault = 10;  // "10 of file redundancy"
inline constexpr std::uint32_t kMultMax     = 16;  // hard ceiling on layers

inline constexpr std::uint32_t kMaxRegions  = 4096; // wear-tracked regions/layer

// ---- Device class + spec (mirrors pcopy device model) -------------------
enum class DeviceClass : std::uint32_t {
    Unknown = 0,
    IdeHdd, SataHdd, SasHdd,
    SataSsd,
    NvmeGen3, NvmeGen4, NvmeGen5,
    Usb2, Usb3, Usb4,
};

// Practical throughput ceilings (MB/s), used to scale quality/pressure.
inline constexpr std::uint32_t kSpeedIdeHdd   = 80;
inline constexpr std::uint32_t kSpeedSataHdd  = 150;
inline constexpr std::uint32_t kSpeedSasHdd   = 200;
inline constexpr std::uint32_t kSpeedSataSsd  = 550;
inline constexpr std::uint32_t kSpeedNvmeGen3 = 3500;
inline constexpr std::uint32_t kSpeedNvmeGen4 = 7000;
inline constexpr std::uint32_t kSpeedNvmeGen5 = 14000;
inline constexpr std::uint32_t kSpeedUsb2     = 35;
inline constexpr std::uint32_t kSpeedUsb3     = 400;
inline constexpr std::uint32_t kSpeedUsb4     = 3000;

// Health color model: green/white/yellow, no red-alarm (as in aptitude/health).
enum class HealthState : std::uint32_t { Green = 0, White, Yellow };

const char* to_string(HealthState s) noexcept;
const char* to_string(DeviceClass c) noexcept;

// =========================================================================
// TABLE 2 — HEALTH / WEAR (about Table 1)
// =========================================================================
struct Tac3RegionWear {
    std::uint64_t reads         = 0; // read touches
    std::uint64_t writes        = 0; // write touches
    std::uint64_t read_heat     = 0; // cumulative read-heat (grows with re-reads)
    std::uint64_t write_wear    = 0; // cumulative write-wear
    std::uint64_t jarring_events= 0; // heavy/abrupt access recorded as extra impact
    std::uint32_t last_quality  = 0; // last read quality, per-mille (0..1000)
    std::uint32_t last_pressure = 0; // last read pressure, per-mille (0..1000)
    std::uint32_t peak_pressure = 0; // peak pressure observed on this region
};

struct Tac3LayerHealth {
    std::uint32_t layer_index    = 0;
    HealthState   state          = HealthState::Green;
    std::uint64_t total_reads    = 0;
    std::uint64_t total_writes   = 0;
    std::uint64_t total_read_heat= 0;
    std::uint64_t total_write_wear = 0;
    std::uint64_t total_jarring  = 0;
    std::uint32_t avg_quality    = 1000; // rolling avg read quality, per-mille
    std::uint32_t avg_pressure   = 0;    // rolling avg pressure, per-mille
    std::uint32_t disk_health    = 1000; // 0..1000 derived health of the layer
    std::uint32_t error_count    = 0;
    std::vector<Tac3RegionWear> region; // sized to kMaxRegions on construction
    Tac3LayerHealth() : region(kMaxRegions) {}
};

// =========================================================================
// TABLE 1 — FILE (n-way redundant)
// =========================================================================
struct Tac3FileEntry {
    std::uint64_t ino  = 0;
    std::uint64_t size = 0;
    std::uint32_t layer_mask    = 0; // which layers hold a good copy
    std::uint32_t primary_layer = 0; // current authoritative layer
    std::array<std::uint8_t, kMultMax> present{}; // 1 if replica valid on layer i
};

// =========================================================================
// TABLE 3 — ADMIN / STATE
// =========================================================================
// Administrative FACTS + OPERATOR-SUPPLIED OPAQUE reference values. The engine
// does not compute, infer, or judge any person's intelligence, worth, feelings,
// learning, friendships, or standing. The operator_ref fields are stored verbatim
// and carry NO engine-assigned meaning.
struct Tac3AdminState {
    // --- objective administrative facts ---
    std::uint64_t tech_id        = 0; // technical/asset identifier
    HealthState   monitor_health = HealthState::Green;
    std::uint32_t table_multitude= kMultDefault;
    std::uint32_t file_table_health = 1000; // 0..1000 health of Table 1
    std::uint32_t admin_table_revision = 0;
    std::uint64_t created_unix   = 0;
    std::uint64_t updated_unix   = 0;
    std::uint32_t special_use_permit_mask = 0;

    // --- operator-supplied OPAQUE reference values (no engine meaning) ---
    std::array<std::int32_t, 8>  operator_ref{};
    std::array<std::string, 8>   operator_ref_label{};
};

struct Tac3Config {
    std::uint32_t multitude    = kMultDefault;
    DeviceClass   device_class = DeviceClass::NvmeGen4;
};

// =========================================================================
// Tac3Engine — the portable wear/pressure/health engine over N layers.
// One instance corresponds to one kernel `struct tac3_sb_info`.
// =========================================================================
class Tac3Engine {
public:
    explicit Tac3Engine(const Tac3Config& cfg = {});

    // Configuration
    std::uint32_t multitude()   const noexcept { return multitude_; }
    DeviceClass   device_class()const noexcept { return device_class_; }
    void set_config(const Tac3Config& cfg);

    // Table 1: allocate a new n-way redundant file entry (spread across layers).
    Tac3FileEntry allocate_entry();

    // Record one access against (layer, region).
    //   op:            AccessOp::Read or AccessOp::Write
    //   observed_mbps: measured read throughput (ignored for writes)
    //   jarring:       heavy/abrupt access -> extra impact across all N layers
    enum class AccessOp { Read = 0, Write = 1 };
    void record_access(std::uint32_t layer, std::uint32_t region,
                       AccessOp op, std::uint32_t observed_mbps, bool jarring);

    // Table 2 reads
    const Tac3LayerHealth& layer(std::uint32_t i) const;
    std::uint32_t file_table_health() const; // min layer health across N layers

    // Table 3
    const Tac3AdminState& admin() const { return admin_; }
    void set_admin(const Tac3AdminState& a); // verbatim; bumps revision

    // Pure helpers (static; identical math to the kernel module)
    static std::uint32_t speed_ceiling(DeviceClass c) noexcept;
    static std::uint32_t quality(DeviceClass c, std::uint32_t observed_mbps) noexcept;
    static std::uint32_t pressure(std::uint64_t read_heat) noexcept;
    static HealthState    state_of(std::uint32_t disk_health,
                                   std::uint32_t errors) noexcept;

private:
    mutable std::mutex           lock_;
    std::uint32_t                multitude_;
    DeviceClass                  device_class_;
    std::vector<Tac3LayerHealth> layers_;   // Table 2, sized kMultMax
    Tac3AdminState               admin_;     // Table 3
    std::uint64_t                next_ino_ = 1;

    static std::uint64_t now_unix() noexcept;
};

} // namespace tac3

#endif // TAC3_HPP
