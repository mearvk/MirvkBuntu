/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * jvmINTLoader.cpp - INT Loading Structure: Inferrer & Orderer
 *
 * Implementation of the 4-tier INT loading structure for the Secure
 * JDK 28. This file contains the inferrer (tier assignment), the
 * orderer (sequencing within tiers), and the top-level coordinator
 * that binds the module system, setup technology, modulator technocator,
 * and technology mind control system into a single loading architecture.
 *
 * The architecture is self-supporting:
 *   Tier 1 (Module) carries everything.
 *   Tier 2 (Setup) enables lateral trust and trade.
 *   Tier 3 (Technocator) handles art, therapy, chemistry.
 *   Tier 4 (Mind Control) executes final orders and money.
 */

#include "runtime/jvmINTLoader.hpp"
#include "utilities/ostream.hpp"

#include <string.h>

// ============================================================================
// Singleton Instance
// ============================================================================

INTLoadingStructure* INTLoadingStructure::_instance = nullptr;

// ============================================================================
// INTInferrer Implementation
// ============================================================================

bool INTInferrer::validate_module_weight(const ModuleWeight* mod) {
  if (mod == nullptr) return false;
  if (mod->name == nullptr || mod->name[0] == '\0') return false;
  if (mod->self_weight == 0) return false;
  return true;
}

bool INTInferrer::can_carry_upper_tiers(const ModuleWeight* mod) {
  if (!validate_module_weight(mod)) return false;

  // A module must have enough support capacity for all upper tiers.
  // Minimum: own weight × 3 (one weight-share per upper tier)
  size_t minimum_capacity = mod->self_weight * 3;
  return mod->support_capacity >= minimum_capacity;
}

INTTier INTInferrer::infer_tier(const INTWorkUnit* work) {
  if (work == nullptr) return INT_TIER_MODULE;

  // Tier 4: Executive — involves money, final orders, or extreme INT
  if (work->involves_money) return INT_TIER_MIND_CONTROL;
  if (work->is_executive)   return INT_TIER_MIND_CONTROL;
  if (work->int_complexity > MIND_COMPLEXITY_THRESHOLD) return INT_TIER_MIND_CONTROL;

  // Tier 3: Technocator — artistry, therapy, high INT
  if (work->has_artistry) return INT_TIER_TECHNOCATOR;
  if (work->int_complexity > TECHNO_COMPLEXITY_THRESHOLD) return INT_TIER_TECHNOCATOR;
  if (work->highest_int >= TECHNO_CONVEY) {
    // Any explicit technocator interest → Tier 3
    return INT_TIER_TECHNOCATOR;
  }

  // Tier 2: Setup — significant lateral relationships
  if (work->lateral_count > SETUP_LATERAL_THRESHOLD) return INT_TIER_SETUP;

  // Tier 1: Module — default foundation
  return INT_TIER_MODULE;
}

INTTier INTInferrer::infer_tier_verbose(const INTWorkUnit* work, outputStream* st) {
  INTTier tier = infer_tier(work);

  if (st != nullptr) {
    st->print_cr("INT Inferrer: '%s'", work->name ? work->name : "(unnamed)");
    st->print_cr("  Weight:         %zu bytes", work->weight);
    st->print_cr("  Lateral count:  %d", work->lateral_count);
    st->print_cr("  INT complexity: %d/100", work->int_complexity);
    st->print_cr("  Has artistry:   %s", work->has_artistry ? "yes" : "no");
    st->print_cr("  Is executive:   %s", work->is_executive ? "yes" : "no");
    st->print_cr("  Involves money: %s", work->involves_money ? "yes" : "no");
    st->print_cr("  → Assigned Tier: %d (%s)", tier,
      tier == INT_TIER_MODULE      ? "MODULE (foundation)" :
      tier == INT_TIER_SETUP       ? "SETUP (lateral trust)" :
      tier == INT_TIER_TECHNOCATOR ? "TECHNOCATOR (art/therapy)" :
      tier == INT_TIER_MIND_CONTROL? "MIND CONTROL (executive)" : "UNKNOWN");
  }

  return tier;
}

bool INTInferrer::is_valid_foundation(const ModuleWeight* mod) {
  return can_carry_upper_tiers(mod);
}

// ============================================================================
// XML Class File Inference (precise path)
// ============================================================================

INTWorkUnit INTInferrer::infer_from_xclass(const char* class_name,
                                            size_t bytecode_size,
                                            int constant_pool_entries,
                                            int field_count,
                                            int method_count,
                                            int interface_count,
                                            int dependency_count,
                                            int trust_grade,
                                            int classload_grade,
                                            const char* design_intent,
                                            const char* design_pattern) {
  INTWorkUnit work;
  memset(&work, 0, sizeof(INTWorkUnit));

  work.name = class_name;

  // Weight: bytecode + constant pool estimation + fields + methods
  // Each constant pool entry averages ~20 bytes, each field ~8, each method ~40
  work.weight = bytecode_size
              + (size_t)constant_pool_entries * 20
              + (size_t)field_count * 8
              + (size_t)method_count * 40;

  // Lateral count: interfaces + explicit dependencies
  work.lateral_count = interface_count + dependency_count;

  // INT complexity from design metadata
  // Trust grade and classload grade contribute to intellectual weight
  int base_int = 0;
  if (trust_grade >= 3) base_int += 30;   // High trust = high INT responsibility
  if (classload_grade >= 5) base_int += 20; // High classload grade = architectural
  if (method_count > 20) base_int += 10;    // Complex class
  if (design_intent != nullptr) base_int += 15;  // Has declared intent
  if (design_pattern != nullptr) base_int += 10; // Has recognized pattern

  work.int_complexity = (base_int > 100) ? 100 : base_int;

  // Artistry detection from design pattern
  work.has_artistry = (design_intent != nullptr && design_pattern != nullptr);

  // Executive detection from trust/classload grade
  work.is_executive = (trust_grade >= 4 || classload_grade >= 7);

  // Money detection: classload grade 7 (Main entry point) often involves economics
  work.involves_money = (classload_grade == 7);

  // Highest INT interest: derive from design pattern
  if (design_pattern != nullptr) {
    work.highest_int = TECHNO_ARRANGE; // Default: has architectural pattern
    if (work.has_artistry) work.highest_int = TECHNO_ARTISTRY;
  }

  return work;
}

// ============================================================================
// Binary .class Heuristic Inference (approximate path)
// ============================================================================

INTWorkUnit INTInferrer::infer_from_binary(const char* class_name,
                                            size_t class_file_size,
                                            int constant_pool_size,
                                            int interface_count,
                                            int method_count,
                                            int field_count) {
  INTWorkUnit work;
  memset(&work, 0, sizeof(INTWorkUnit));

  work.name = class_name;
  work.weight = class_file_size;

  // Lateral: interfaces only (no dependency declarations in binary)
  work.lateral_count = interface_count;

  // INT complexity heuristic: large classes with many methods are more complex
  int est_int = 0;
  if (class_file_size > 50000) est_int += 20;   // Large class
  if (constant_pool_size > 200) est_int += 15;  // Rich constant pool
  if (method_count > 30) est_int += 15;         // Many methods
  if (interface_count > 5) est_int += 10;       // Highly connected
  if (field_count > 20) est_int += 10;          // Data-heavy

  work.int_complexity = (est_int > 100) ? 100 : est_int;

  // No artistry, executive, or money detection from binary
  // (binary format cannot express these concerns)
  work.has_artistry = false;
  work.is_executive = false;
  work.involves_money = false;
  work.highest_int = TECHNO_CONVEY; // Default: binary has basic conveyance

  return work;
}

// ============================================================================
// INTStructuralMath Implementation
// ============================================================================

size_t INTStructuralMath::total_weight(size_t own_weight, size_t super_weight) {
  // W_total(C) = W(C) + W(super(C))
  // Overflow protection
  if (own_weight > (size_t)-1 - super_weight) return (size_t)-1;
  return own_weight + super_weight;
}

int INTStructuralMath::total_lateral(int own_lateral, int super_lateral, int interface_laterals) {
  // L_total(C) = L(C) + L(super) + Σ L(interfaces)
  return own_lateral + super_lateral + interface_laterals;
}

int INTStructuralMath::effective_int_level(int own_level, int super_level, int max_interface_level) {
  // I_total(C) = max(I(C), I(super(C)), max(I(interface_i)))
  // INT level only goes UP through inheritance, never down
  int max = own_level;
  if (super_level > max) max = super_level;
  if (max_interface_level > max) max = max_interface_level;
  return max;
}

size_t INTStructuralMath::minimum_support(size_t own_weight, int expected_subclass_count) {
  // A class must support its own weight × 3 (for upper tiers) PLUS
  // the combined weight of expected subclasses
  // S_min(C) = W(C) × 3 + Σ W(subclass_i)
  // We estimate subclass weight as own_weight per subclass (conservative)
  size_t tier_support = own_weight * 3;
  size_t subclass_support = own_weight * (size_t)expected_subclass_count;
  return tier_support + subclass_support;
}

bool INTStructuralMath::is_structurally_sound(size_t support_capacity, size_t inherited_load) {
  // A class is structurally sound if its support capacity exceeds
  // the total load placed on it by inheritors
  return support_capacity >= inherited_load;
}

INTTier INTStructuralMath::compute_tier_from_structure(size_t total_wt,
                                                       int total_lat,
                                                       int effective_int,
                                                       bool is_executive) {
  // Pure structural math — no heuristics, just the numbers:
  //   Executive flag or INT >= 80 → Tier 4
  //   INT >= 50                   → Tier 3
  //   Lateral >= 4                → Tier 2
  //   Otherwise                   → Tier 1
  if (is_executive || effective_int >= 80) return INT_TIER_MIND_CONTROL;
  if (effective_int >= 50)                 return INT_TIER_TECHNOCATOR;
  if (total_lat >= 4)                      return INT_TIER_SETUP;
  return INT_TIER_MODULE;
}

size_t INTInferrer::estimate_support_needed(int tier2_count, int tier3_count, int tier4_count) {
  // Each tier 2 unit needs ~4KB support base
  // Each tier 3 unit needs ~16KB support base (artistry is heavy)
  // Each tier 4 unit needs ~64KB support base (executive is heaviest)
  return (size_t)tier2_count * 4096
       + (size_t)tier3_count * 16384
       + (size_t)tier4_count * 65536;
}

// ============================================================================
// INTOrderer Implementation
// ============================================================================

INTOrderer::INTOrderer()
  : _tier1_count(0), _tier2_count(0), _tier3_count(0), _tier4_count(0) {
  memset(_tier1_queue, 0, sizeof(_tier1_queue));
  memset(_tier2_queue, 0, sizeof(_tier2_queue));
  memset(_tier3_queue, 0, sizeof(_tier3_queue));
  memset(_tier4_queue, 0, sizeof(_tier4_queue));
}

int INTOrderer::artistry_rank(TechnocatorInterest interest) {
  // Ordering: demange → demart → convey → therapy → arrange → art → artistry
  // Demange is first because it is "before art, but art" — the seed
  // Demart is second because chemistry comes before wisdom
  // Then the progression of intellectual realization
  switch (interest) {
    case TECHNO_DEMANGE:   return 0;  // Before art, but art — earliest
    case TECHNO_DEMART:    return 1;  // Chemistry before wisdom — preparation
    case TECHNO_CONVEY:    return 2;  // Conveyance of meaning
    case TECHNO_THERAPY:   return 3;  // Healing and restoration
    case TECHNO_ARRANGE:   return 4;  // Arrangement into form
    case TECHNO_ART:       return 5;  // Art realized
    case TECHNO_ARTISTRY:  return 6;  // Full craft — highest
    default:               return 99;
  }
}

int INTOrderer::mind_control_rank(MindControlOp op) {
  // Ordering: fill → middle → recycle → reorder → colors → final orders → final sames → money
  // Fill is first — you must fill the orders from above
  // Middle concerns process them
  // Recycle reuses what can be reused
  // Reorder adjusts priority
  // Colors express the nature of intellect applied
  // Final orders are the last instruction
  // Final sames achieve equilibrium
  // Money is the economic conclusion — always last
  switch (op) {
    case MIND_FILL:         return 0;
    case MIND_MIDDLE:       return 1;
    case MIND_RECYCLE:      return 2;
    case MIND_REORDER:      return 3;
    case MIND_COLORS:       return 4;
    case MIND_FINAL_ORDERS: return 5;
    case MIND_FINAL_SAMES:  return 6;
    case MIND_MONEY:        return 7;  // Always last — the conclusion
    default:                return 99;
  }
}

IntellectColor INTOrderer::assign_color(const INTWorkUnit* work) {
  if (work == nullptr) return COLOR_CLEAR;

  // Money → Gold
  if (work->involves_money) return COLOR_GOLD;

  // Executive/final → Red (urgency, security)
  if (work->is_executive) return COLOR_RED;

  // Artistry/therapy → Green (growth, natural)
  if (work->has_artistry) return COLOR_GREEN;

  // High complexity → Blue (communication, conveyance)
  if (work->int_complexity > 60) return COLOR_BLUE;

  // Lateral/trust → Silver (utility, infrastructure)
  if (work->lateral_count > 3) return COLOR_SILVER;

  // Foundation → White (ethics, purity)
  if (work->weight > 0) return COLOR_WHITE;

  // Default → Clear (pure logic)
  return COLOR_CLEAR;
}

int INTOrderer::sequence_by_weight(const INTWorkUnit* work) {
  // Tier 1: heaviest modules load first (they support the most)
  // Sequence inversely to weight — heavy = low number = first
  if (work->weight > 1048576) return 0;   // >1MB: first priority
  if (work->weight > 65536)   return 1;   // >64KB: second
  if (work->weight > 4096)    return 2;   // >4KB: third
  return 3;                                // Light: last
}

int INTOrderer::sequence_by_trust(const INTWorkUnit* work) {
  // Tier 2: most-trusted peers sequence first
  // More laterals = more trust established = earlier in sequence
  if (work->lateral_count > 10) return 0;  // Highly connected: first
  if (work->lateral_count > 5)  return 1;  // Well connected: second
  if (work->lateral_count > 3)  return 2;  // Connected: third
  return 3;                                 // Minimal: last
}

int INTOrderer::sequence_by_artistry(const INTWorkUnit* work) {
  // Tier 3: order by artistry rank
  return artistry_rank(work->highest_int);
}

int INTOrderer::sequence_by_int_order(const INTWorkUnit* work) {
  // Tier 4: executive ordering
  // Money always sequences last (it's the conclusion)
  if (work->involves_money) return mind_control_rank(MIND_MONEY);
  if (work->is_executive)   return mind_control_rank(MIND_FINAL_ORDERS);
  // Default: fill position
  return mind_control_rank(MIND_FILL);
}

INTOrder INTOrderer::order(const INTWorkUnit* work) {
  INTOrder result;
  memset(&result, 0, sizeof(INTOrder));

  if (work == nullptr) return result;

  // Step 1: Infer tier
  INTTier tier = INTInferrer::infer_tier(work);
  result.assigned_tier = tier;

  // Step 2: Assign color
  result.color = assign_color(work);

  // Step 3: Sequence within tier
  switch (tier) {
    case INT_TIER_MODULE:
      result.sequence = sequence_by_weight(work);
      result.priority = (int)(work->weight / 1024);  // KB as priority
      break;

    case INT_TIER_SETUP:
      result.sequence = sequence_by_trust(work);
      result.priority = work->lateral_count;
      result.setup_cap = (work->lateral_count > 5) ? SETUP_TRUST : SETUP_CONTROL;
      break;

    case INT_TIER_TECHNOCATOR:
      result.sequence = sequence_by_artistry(work);
      result.priority = work->int_complexity;
      result.techno_interest = work->highest_int;
      break;

    case INT_TIER_MIND_CONTROL:
      result.sequence = sequence_by_int_order(work);
      result.priority = 100;  // Executive is always high priority
      result.mind_op = work->involves_money ? MIND_MONEY :
                       work->is_executive   ? MIND_FINAL_ORDERS :
                                              MIND_FILL;
      result.is_final = work->is_executive;
      break;
  }

  // Step 4: Enqueue into appropriate tier queue
  switch (tier) {
    case INT_TIER_MODULE:
      if (_tier1_count < MAX_ORDERS_PER_TIER) {
        _tier1_queue[_tier1_count++] = result;
      }
      break;
    case INT_TIER_SETUP:
      if (_tier2_count < MAX_ORDERS_PER_TIER) {
        _tier2_queue[_tier2_count++] = result;
      }
      break;
    case INT_TIER_TECHNOCATOR:
      if (_tier3_count < MAX_ORDERS_PER_TIER) {
        _tier3_queue[_tier3_count++] = result;
      }
      break;
    case INT_TIER_MIND_CONTROL:
      if (_tier4_count < MAX_ORDERS_PER_TIER) {
        _tier4_queue[_tier4_count++] = result;
      }
      break;
  }

  return result;
}

INTOrder* INTOrderer::next_order(INTTier tier) {
  switch (tier) {
    case INT_TIER_MODULE:
      return (_tier1_count > 0) ? &_tier1_queue[0] : nullptr;
    case INT_TIER_SETUP:
      return (_tier2_count > 0) ? &_tier2_queue[0] : nullptr;
    case INT_TIER_TECHNOCATOR:
      return (_tier3_count > 0) ? &_tier3_queue[0] : nullptr;
    case INT_TIER_MIND_CONTROL:
      return (_tier4_count > 0) ? &_tier4_queue[0] : nullptr;
    default:
      return nullptr;
  }
}

void INTOrderer::recycle_order(INTOrder* order) {
  if (order == nullptr) return;

  // Recycling: demote back to Tier 1 as a fresh module unit
  // The recycled order retains its color but loses executive status
  order->assigned_tier = INT_TIER_MODULE;
  order->is_final = false;
  order->mind_op = MIND_RECYCLE;
  order->sequence = 99;  // End of Tier 1 queue (recycled = low priority)

  if (_tier1_count < MAX_ORDERS_PER_TIER) {
    _tier1_queue[_tier1_count++] = *order;
  }
}

void INTOrderer::reorder_concerns(INTTier tier) {
  // Simple insertion sort by priority (descending) within a tier
  INTOrder* queue = nullptr;
  int count = 0;

  switch (tier) {
    case INT_TIER_MODULE:      queue = _tier1_queue; count = _tier1_count; break;
    case INT_TIER_SETUP:       queue = _tier2_queue; count = _tier2_count; break;
    case INT_TIER_TECHNOCATOR: queue = _tier3_queue; count = _tier3_count; break;
    case INT_TIER_MIND_CONTROL:queue = _tier4_queue; count = _tier4_count; break;
    default: return;
  }

  for (int i = 1; i < count; i++) {
    INTOrder key = queue[i];
    int j = i - 1;
    while (j >= 0 && queue[j].priority < key.priority) {
      queue[j + 1] = queue[j];
      j--;
    }
    queue[j + 1] = key;
  }
}

int INTOrderer::pending_count(INTTier tier) const {
  switch (tier) {
    case INT_TIER_MODULE:       return _tier1_count;
    case INT_TIER_SETUP:        return _tier2_count;
    case INT_TIER_TECHNOCATOR:  return _tier3_count;
    case INT_TIER_MIND_CONTROL: return _tier4_count;
    default: return 0;
  }
}

bool INTOrderer::has_final_orders() const {
  for (int i = 0; i < _tier4_count; i++) {
    if (_tier4_queue[i].is_final) return true;
  }
  return false;
}

void INTOrderer::print_status(outputStream* st) const {
  st->print_cr("═══════════════════════════════════════════════════════════════");
  st->print_cr("  INT ORDERER — Queue Status");
  st->print_cr("═══════════════════════════════════════════════════════════════");
  st->print_cr("  Tier 1 (Module):       %d pending", _tier1_count);
  st->print_cr("  Tier 2 (Setup):        %d pending", _tier2_count);
  st->print_cr("  Tier 3 (Technocator):  %d pending", _tier3_count);
  st->print_cr("  Tier 4 (Mind Control): %d pending", _tier4_count);
  st->print_cr("  Final orders queued:   %s", has_final_orders() ? "YES" : "no");
  st->print_cr("═══════════════════════════════════════════════════════════════");
}

void INTOrderer::print_tier_queue(INTTier tier, outputStream* st) const {
  const INTOrder* queue = nullptr;
  int count = 0;
  const char* tier_name = "";

  switch (tier) {
    case INT_TIER_MODULE:
      queue = _tier1_queue; count = _tier1_count; tier_name = "MODULE"; break;
    case INT_TIER_SETUP:
      queue = _tier2_queue; count = _tier2_count; tier_name = "SETUP"; break;
    case INT_TIER_TECHNOCATOR:
      queue = _tier3_queue; count = _tier3_count; tier_name = "TECHNOCATOR"; break;
    case INT_TIER_MIND_CONTROL:
      queue = _tier4_queue; count = _tier4_count; tier_name = "MIND CONTROL"; break;
    default: return;
  }

  st->print_cr("  Tier %d (%s) — %d orders:", (int)tier, tier_name, count);
  for (int i = 0; i < count && i < 20; i++) {
    st->print_cr("    [%d] seq=%d pri=%d color=%d final=%s",
      i, queue[i].sequence, queue[i].priority, (int)queue[i].color,
      queue[i].is_final ? "YES" : "no");
  }
  if (count > 20) {
    st->print_cr("    ... (%d more)", count - 20);
  }
}

// ============================================================================
// INTLoadingStructure Implementation
// ============================================================================

INTLoadingStructure::INTLoadingStructure()
  : _orderer(nullptr), _initialized(false), _module_count(0),
    _total_self_weight(0), _total_support_capacity(0) {
  memset(_modules, 0, sizeof(_modules));
}

INTLoadingStructure::~INTLoadingStructure() {
  if (_orderer != nullptr) {
    delete _orderer;
    _orderer = nullptr;
  }
  _initialized = false;
}

INTLoadingStructure* INTLoadingStructure::instance() {
  return _instance;
}

void INTLoadingStructure::initialize() {
  if (_instance != nullptr) return;

  _instance = new INTLoadingStructure();
  _instance->_orderer = new INTOrderer();
  _instance->_initialized = true;
}

void INTLoadingStructure::destroy() {
  if (_instance != nullptr) {
    delete _instance;
    _instance = nullptr;
  }
}

bool INTLoadingStructure::register_module(const ModuleWeight* mod) {
  if (!_initialized) return false;
  if (mod == nullptr) return false;
  if (_module_count >= MAX_MODULES) return false;

  // Validate: module must be a valid foundation
  if (!INTInferrer::is_valid_foundation(mod)) {
    // Module cannot support upper tiers — reject
    return false;
  }

  _modules[_module_count] = *mod;
  _total_self_weight += mod->self_weight;
  _total_support_capacity += mod->support_capacity;
  _module_count++;

  return true;
}

INTOrder INTLoadingStructure::submit(const INTWorkUnit* work) {
  INTOrder empty;
  memset(&empty, 0, sizeof(INTOrder));

  if (!_initialized || _orderer == nullptr) return empty;
  if (work == nullptr) return empty;

  return _orderer->order(work);
}

INTOrder* INTLoadingStructure::execute_next() {
  if (!_initialized || _orderer == nullptr) return nullptr;

  // Execute highest tier first (Tier 4 → 3 → 2 → 1)
  // Mind control takes precedence — it lives at the top
  INTOrder* order = _orderer->next_order(INT_TIER_MIND_CONTROL);
  if (order != nullptr) return order;

  order = _orderer->next_order(INT_TIER_TECHNOCATOR);
  if (order != nullptr) return order;

  order = _orderer->next_order(INT_TIER_SETUP);
  if (order != nullptr) return order;

  order = _orderer->next_order(INT_TIER_MODULE);
  return order;
}

void INTLoadingStructure::fill_int_orders() {
  if (!_initialized || _orderer == nullptr) return;

  // Tier 4 operation: fill orders from the executive queue
  // This means processing pending Tier 4 items in sequence
  _orderer->reorder_concerns(INT_TIER_MIND_CONTROL);
}

void INTLoadingStructure::recycle_int_orders() {
  if (!_initialized || _orderer == nullptr) return;

  // Take completed Tier 4 orders and recycle them back to Tier 1
  // They re-enter the foundation as recycled weight
  INTOrder* order = _orderer->next_order(INT_TIER_MIND_CONTROL);
  if (order != nullptr && !order->is_final) {
    _orderer->recycle_order(order);
  }
}

void INTLoadingStructure::reorder_int_concerns() {
  if (!_initialized || _orderer == nullptr) return;

  // Reorder all tiers by current priority
  _orderer->reorder_concerns(INT_TIER_MODULE);
  _orderer->reorder_concerns(INT_TIER_SETUP);
  _orderer->reorder_concerns(INT_TIER_TECHNOCATOR);
  _orderer->reorder_concerns(INT_TIER_MIND_CONTROL);
}

void INTLoadingStructure::issue_final_order() {
  if (!_initialized || _orderer == nullptr) return;

  // Create a final same — equilibrium marker
  INTWorkUnit final_work;
  memset(&final_work, 0, sizeof(INTWorkUnit));
  final_work.name = "FINAL_SAME";
  final_work.is_executive = true;
  final_work.int_complexity = 100;
  final_work.involves_money = true;

  _orderer->order(&final_work);
}

void INTLoadingStructure::print_on(outputStream* st) const {
  st->print_cr("═══════════════════════════════════════════════════════════════");
  st->print_cr("  INT LOADING STRUCTURE — Secure JDK 28");
  st->print_cr("  Galactic Cherry Marvell Edition 98");
  st->print_cr("═══════════════════════════════════════════════════════════════");
  st->cr();
  st->print_cr("  TIER 1 — MODULE SYSTEM (Foundation)");
  st->print_cr("    Registered modules:  %d / %d", _module_count, MAX_MODULES);
  st->print_cr("    Total self weight:   %zu bytes", _total_self_weight);
  st->print_cr("    Total support cap:   %zu bytes", _total_support_capacity);
  st->cr();
  st->print_cr("  TIER 2 — SETUP TECHNOLOGY (Lateral Trust)");
  st->print_cr("    Capabilities: control, trust, gain-of-mind, lateral, trade");
  st->cr();
  st->print_cr("  TIER 3 — MODULATOR TECHNOCATOR (Higher INT)");
  st->print_cr("    Interests: convey, therapy, arrange, art, demange, demart, artistry");
  st->print_cr("    Ordering:  demange → demart → convey → therapy → arrange → art → artistry");
  st->cr();
  st->print_cr("  TIER 4 — TECHNOLOGY MIND CONTROL (Executive)");
  st->print_cr("    Operations: fill → middle → recycle → reorder → colors → finals → money");
  st->print_cr("    Colors: white, gold, blue, green, silver, red, clear");
  st->cr();

  if (_orderer != nullptr) {
    _orderer->print_status(st);
  }
}

void INTLoadingStructure::print_proc_status(outputStream* st) const {
  st->print_cr("INT Loading Structure v1.0");
  st->print_cr("Status: %s", _initialized ? "ACTIVE" : "INACTIVE");
  st->print_cr("Modules: %d", _module_count);
  st->print_cr("Weight: %zu bytes", _total_self_weight);
  st->print_cr("Capacity: %zu bytes", _total_support_capacity);

  if (_orderer != nullptr) {
    st->print_cr("Tier1_pending: %d", _orderer->pending_count(INT_TIER_MODULE));
    st->print_cr("Tier2_pending: %d", _orderer->pending_count(INT_TIER_SETUP));
    st->print_cr("Tier3_pending: %d", _orderer->pending_count(INT_TIER_TECHNOCATOR));
    st->print_cr("Tier4_pending: %d", _orderer->pending_count(INT_TIER_MIND_CONTROL));
    st->print_cr("Final_orders: %s", _orderer->has_final_orders() ? "YES" : "NO");
  }
}
