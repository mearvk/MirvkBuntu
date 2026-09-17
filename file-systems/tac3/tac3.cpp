// SPDX-License-Identifier: GPL-2.0
//
// tac3.cpp — C++ equivalent of the TAC3 wear/pressure/health engine.
//
// Portable, standalone implementation of the pure logic in tac3.c. See tac3.hpp
// for the design rationale and the mapping to the kernel structs. This file
// deliberately re-implements ONLY the portable engine (Tables 1/2/3 + the
// quality/pressure/health derivations); the mountable-filesystem VFS glue lives
// in the kernel module tac3.c and has no userspace equivalent.
//
// The arithmetic matches the kernel module exactly:
//   - quality (per-mille) = min(1000, observed*1000/ceiling)
//   - pressure (per-mille) = +50 per doubling of region read-heat, capped 1000
//   - disk_health = 1000 - (write_wear>>6 + jarring>>2 + errors*20), floored 0
//   - a jarring access increments total_jarring on ALL N layers
//
// Copyright (C) 2026 MEARVK LLC
// Author: Maximilian Eric Alexander Rupplin von Keffikon
//
#include "tac3.hpp"

#include <chrono>

namespace tac3 {

const char* to_string(HealthState s) noexcept {
    switch (s) {
    case HealthState::Green:  return "GREEN";
    case HealthState::White:  return "WHITE";
    case HealthState::Yellow: return "YELLOW";
    default:                  return "?";
    }
}

const char* to_string(DeviceClass c) noexcept {
    switch (c) {
    case DeviceClass::IdeHdd:   return "IDE_HDD";
    case DeviceClass::SataHdd:  return "SATA_HDD";
    case DeviceClass::SasHdd:   return "SAS_HDD";
    case DeviceClass::SataSsd:  return "SATA_SSD";
    case DeviceClass::NvmeGen3: return "NVME_GEN3";
    case DeviceClass::NvmeGen4: return "NVME_GEN4";
    case DeviceClass::NvmeGen5: return "NVME_GEN5";
    case DeviceClass::Usb2:     return "USB2";
    case DeviceClass::Usb3:     return "USB3";
    case DeviceClass::Usb4:     return "USB4";
    default:                    return "UNKNOWN";
    }
}

// ---- pure helpers (identical math to the kernel module) -----------------

std::uint32_t Tac3Engine::speed_ceiling(DeviceClass c) noexcept {
    switch (c) {
    case DeviceClass::IdeHdd:   return kSpeedIdeHdd;
    case DeviceClass::SataHdd:  return kSpeedSataHdd;
    case DeviceClass::SasHdd:   return kSpeedSasHdd;
    case DeviceClass::SataSsd:  return kSpeedSataSsd;
    case DeviceClass::NvmeGen3: return kSpeedNvmeGen3;
    case DeviceClass::NvmeGen4: return kSpeedNvmeGen4;
    case DeviceClass::NvmeGen5: return kSpeedNvmeGen5;
    case DeviceClass::Usb2:     return kSpeedUsb2;
    case DeviceClass::Usb3:     return kSpeedUsb3;
    case DeviceClass::Usb4:     return kSpeedUsb4;
    default:                    return kSpeedSataSsd;
    }
}

// Read QUALITY (per-mille): how well observed throughput met the device spec.
std::uint32_t Tac3Engine::quality(DeviceClass c, std::uint32_t observed_mbps) noexcept {
    std::uint32_t ceil = speed_ceiling(c);
    if (!ceil)
        return 0;
    std::uint64_t q = static_cast<std::uint64_t>(observed_mbps) * 1000ULL / ceil;
    return (q > 1000) ? 1000u : static_cast<std::uint32_t>(q);
}

// Read PRESSURE (per-mille): rises with region read-heat, +50 per doubling.
std::uint32_t Tac3Engine::pressure(std::uint64_t read_heat) noexcept {
    std::uint32_t p = 0;
    std::uint64_t h = read_heat;
    while (h && p < 1000) { p += 50; h >>= 1; }
    return p;
}

// Derive green/white/yellow state from a layer's health value + error count.
HealthState Tac3Engine::state_of(std::uint32_t disk_health, std::uint32_t errors) noexcept {
    if (errors > 0 || disk_health < 600)
        return HealthState::Yellow; // attention recommended
    if (disk_health < 850)
        return HealthState::White;  // informational / normal
    return HealthState::Green;      // healthy / verified
}

std::uint64_t Tac3Engine::now_unix() noexcept {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(
        duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
}

// ---- engine -------------------------------------------------------------

Tac3Engine::Tac3Engine(const Tac3Config& cfg)
    : multitude_(cfg.multitude), device_class_(cfg.device_class),
      layers_(kMultMax) {
    if (multitude_ < kMultMin) multitude_ = kMultMin;
    if (multitude_ > kMultMax) multitude_ = kMultMax;

    for (std::uint32_t i = 0; i < kMultMax; ++i) {
        layers_[i].layer_index = i;
        layers_[i].disk_health = 1000;
        layers_[i].state = HealthState::Green;
        layers_[i].avg_quality = 1000;
    }
    admin_.table_multitude    = multitude_;
    admin_.monitor_health     = HealthState::Green;
    admin_.file_table_health  = 1000;
    admin_.created_unix = admin_.updated_unix = now_unix();
}

void Tac3Engine::set_config(const Tac3Config& cfg) {
    std::lock_guard<std::mutex> g(lock_);
    std::uint32_t m = cfg.multitude;
    if (m < kMultMin) m = kMultMin;
    if (m > kMultMax) m = kMultMax;
    multitude_ = m;
    device_class_ = cfg.device_class;
    admin_.table_multitude = m;
    admin_.updated_unix = now_unix();
}

Tac3FileEntry Tac3Engine::allocate_entry() {
    std::lock_guard<std::mutex> g(lock_);
    Tac3FileEntry e;
    e.ino  = next_ino_;
    e.size = 0;
    e.primary_layer = static_cast<std::uint32_t>(next_ino_ % multitude_);
    e.layer_mask = (multitude_ >= 32) ? 0xffffffffu
                                      : ((1u << multitude_) - 1u);
    for (std::uint32_t i = 0; i < multitude_ && i < kMultMax; ++i)
        e.present[i] = 1;
    ++next_ino_;
    return e;
}

void Tac3Engine::record_access(std::uint32_t layer, std::uint32_t region,
                               AccessOp op, std::uint32_t observed_mbps,
                               bool jarring) {
    if (layer >= multitude_ || region >= kMaxRegions)
        return;

    std::lock_guard<std::mutex> g(lock_);
    Tac3LayerHealth& L = layers_[layer];
    Tac3RegionWear&  R = L.region[region];

    if (op == AccessOp::Read) {
        R.reads++;      L.total_reads++;
        R.read_heat++;  L.total_read_heat++;
        std::uint32_t q = quality(device_class_, observed_mbps);
        std::uint32_t p = pressure(R.read_heat);
        R.last_quality  = q;
        R.last_pressure = p;
        if (p > R.peak_pressure) R.peak_pressure = p;
        // rolling averages: += (sample - avg) >> 3  (signed step, unsigned store)
        L.avg_quality  = static_cast<std::uint32_t>(
            L.avg_quality  + ((static_cast<std::int64_t>(q) - L.avg_quality)  >> 3));
        L.avg_pressure = static_cast<std::uint32_t>(
            L.avg_pressure + ((static_cast<std::int64_t>(p) - L.avg_pressure) >> 3));
    } else {
        R.writes++;      L.total_writes++;
        R.write_wear++;  L.total_write_wear++;
    }

    if (jarring) {
        R.jarring_events++;
        // A jarring event's extra impact is spread across ALL N layers.
        for (std::uint32_t i = 0; i < multitude_; ++i)
            layers_[i].total_jarring++;
    }

    std::uint64_t penalty = (L.total_write_wear >> 6)
                          + (L.total_jarring >> 2)
                          + static_cast<std::uint64_t>(L.error_count) * 20ULL;
    L.disk_health = (penalty >= 1000) ? 0u
                                      : static_cast<std::uint32_t>(1000 - penalty);
    L.state = state_of(L.disk_health, L.error_count);

    admin_.updated_unix = now_unix();
}

const Tac3LayerHealth& Tac3Engine::layer(std::uint32_t i) const {
    std::lock_guard<std::mutex> g(lock_);
    return layers_.at(i);
}

std::uint32_t Tac3Engine::file_table_health() const {
    std::lock_guard<std::mutex> g(lock_);
    std::uint32_t m = 1000;
    for (std::uint32_t i = 0; i < multitude_; ++i)
        if (layers_[i].disk_health < m)
            m = layers_[i].disk_health;
    return m;
}

void Tac3Engine::set_admin(const Tac3AdminState& a) {
    std::lock_guard<std::mutex> g(lock_);
    std::uint32_t rev = admin_.admin_table_revision;
    admin_ = a;                       // operator sets verbatim; no meaning applied
    admin_.admin_table_revision = rev + 1;
    admin_.updated_unix = now_unix();
}

} // namespace tac3
