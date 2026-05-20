# VIGIL — Mini Game Design Document

**Version:** 1.2
**Status:** Pre-production locked — visual direction pinned
**Tagline:** *Hold the Ruin — or be Forgotten*
**Target platform (primary):** macOS (Apple Silicon, M3)
**Target platform (secondary):** Windows
**Engine:** Custom C++20 + Vulkan 1.2 (MoltenVK on macOS)
**Production window:** 8 weeks, ship to Steam + itch.io
**Author:** Mehmet Isitmen
**Companion docs:** `vigil_roadmap_v1.md`, `vigil_tripo_mixamo_pipeline.md`
**Changes from v1.1:**
- Boss renamed: *Pale King* → **Pale Sovereign** (throughout)
- §15 Main Menu pinned to Omma-generated mockup as Phase 7 visual reference
- §15 Continue menu item removed (single-run game; no resume)
- §15 Settings rewritten: Validation Layers toggle stripped (dev-only concern)
- §15 New: *Visual Token Reference* — palette vec4s, fonts, animation timings
- §17 Build path declared: custom Vulkan UI pass for menu (Day 35)
- §1 Tagline added
- §21 Open questions: all resolved

---

## Table of Contents

1. [Vision & Pitch](#1-vision--pitch)
2. [Design Pillars](#2-design-pillars)
3. [Player Experience Goals](#3-player-experience-goals)
4. [High-Level Game Flow](#4-high-level-game-flow)
5. [Combat System](#5-combat-system)
6. [Player Kit — Frame Data & Numbers](#6-player-kit--frame-data--numbers)
7. [Stamina & Faith Economy](#7-stamina--faith-economy)
8. [Enemy Designs](#8-enemy-designs)
9. [Wave Structure](#9-wave-structure)
10. [Boss Fight — The Pale Sovereign](#10-boss-fight--the-pale-sovereign)
11. [Arena Design](#11-arena-design)
12. [Atmosphere & Art Direction](#12-atmosphere--art-direction)
13. [Audio Brief](#13-audio-brief)
14. [Polish & Juice Specification](#14-polish--juice-specification)
15. [UI / HUD Specification](#15-ui--hud-specification)
16. [Meta & Scoring](#16-meta--scoring)
17. [Technical Architecture](#17-technical-architecture)
18. [Asset Bill of Materials](#18-asset-bill-of-materials)
19. [Production Scope Cuts](#19-production-scope-cuts)
20. [Success Criteria](#20-success-criteria)
21. [Resolved Open Questions](#21-resolved-open-questions)
22. [Asset Pipeline Reference](#22-asset-pipeline-reference)

---

## 1. Vision & Pitch

**Logline.** A lone knight holds an ancient ruin against the rising dark. Seven waves, one boss, one night.

**Tagline.** *Hold the Ruin — or be Forgotten.*

**Elevator.** *Vigil* is a tight, atmospheric single-arena combat gauntlet. You approach the courtyard of a ruined keep at dusk, walk its perimeter, and when ready cross the broken altar to begin. Across forty minutes of escalating waves, the light fails, the cold sets in, and finally the Pale Sovereign arrives. There is no progression, no inventory, no map — only the rhythm of attack, parry, and roll, and a steadily darkening sky.

The game is deliberately small. Its ambition is craft, not scope: every system that exists is polished to a degree the prototype only hints at. The pitch to a studio reviewer is *I can take an idea from prototype to shippable on a modern C++/Vulkan stack*, demonstrated alongside Mythbreaker and Legionfall.

**Launch destination:** Steam (primary) and itch.io (concurrent free release). Steamworks Direct account application submitted Day 36 of production to allow the 5-day Valve review window before launch.

---

## 2. Design Pillars

Every design decision below answers to one of these four pillars. When two pillars conflict, the higher-numbered one loses.

1. **Atmosphere over scope.** The arena, the lighting curve, the sound of wind through ruined stone — these carry the experience. We will cut mechanics before we cut atmosphere.
2. **Combat that demands attention.** Every enemy attack is telegraphed and punishable. The player should die from inattention, not from unfairness. Parry exists because it converts attention into spectacle.
3. **One run, one arc.** A complete play is ~30–40 minutes with a clear beginning, escalation, climax, and end. No grinding, no extending, no metagame loops, no resume.
4. **Polish as a deliverable.** Hitstop, screen shake, audio mix, parry feedback — these are the actual product. The combat numbers exist to give the polish something to attach to.

---

## 3. Player Experience Goals

What we want the player to *feel*:

- **Approach (pre-wave-1):** Curious. The arena is silent. The player walks the perimeter, sees the altar in the centre, sees what's coming. They cross the threshold when ready.
- **Wave 1–2:** Confident. The knight is heavy and capable. Cultists are dispatched cleanly. The light is still warm.
- **Wave 3–4:** Cautious. The first Wraith forces a slower, more deliberate rhythm. The player learns to read telegraphs and parry. Dusk is dying.
- **Wave 5–6:** Pressured. Mixed compositions; stamina becomes a real budget. Full night.
- **Wave 7:** Confronted. The Pale Sovereign arrives with a music sting and a lighting shift. The player has earned this fight; whether they survive it is on them.

A successful run should end in either a death the player owns or a victory that feels narrow.

---

## 4. High-Level Game Flow

```
[Main Menu]
    -> "Begin the Vigil"
    -> [Approach Phase: knight spawns at south breach, free wander, ~30-90s]
    -> Player crosses altar threshold -> wave 1 triggers
    -> [Wave Banner: "I — The Faithful"]
    -> [Wave 1 Combat]
    -> [5s breather, +20 HP refund, atmosphere shift]
    -> [Wave 2] ... [Wave 6]
    -> [Wave Banner: "VII — The Pale Sovereign"]
    -> [Boss music sting, lighting darkens further, boss spawns from altar]
    -> [Boss Fight]
    -> [Victory screen OR Death screen]
        -> Run stats: wave reached, time, parries landed, hits taken
        -> Best run displayed
    -> [Back to Main Menu]
```

The whole loop is reachable in two clicks from launch (Main Menu → Begin the Vigil).

---

## 5. Combat System

Combat is **directional, stamina-gated, and reactive**, leaning closer to *Sekiro* readability than *Dark Souls* weight. Three things define a good engagement:

1. **Reading the telegraph.** Every enemy attack has a clear pre-swing animation lasting 0.5–1.5 seconds.
2. **Choosing the response.** Roll (i-frames, costly), block (reduces damage, drains stamina), or parry (tight window, opens riposte).
3. **Punishing.** After a parry, dodge, or guard break, the player has a brief window to land a combo.

There is no lock-on. Soft targeting is handled by camera-relative input and a forgiving attack arc (90° hit cone, 2.0m reach).

---

## 6. Player Kit — Frame Data & Numbers

| Stat | Value | Notes |
|---|---|---|
| HP | 100 (fixed) | No level system; refunded +20 between waves |
| Stamina | 100 | Regens 25/s when not attacking, blocking, sprinting |
| Faith | 60 | Regens 5/s, always |
| Walk speed | 3.0 m/s | |
| Sprint speed | 6.0 m/s | Drains 30 stamina/s |
| Attack arc | 90° cone, 2.0m reach | |

**Light attack (LMB):**
- Cost: 12 stamina
- Damage: 18 / 18 / 25 (3-hit combo, last hit knockback)
- Startup: 0.30s, Active: 0.10s, Recovery: 0.40s
- Combo window: 0.5s after recovery start

**Heavy attack (Shift + LMB):**
- Cost: 24 stamina
- Damage: 45, AoE knockback in cone
- Startup: 0.55s, Active: 0.15s, Recovery: 0.70s
- Cancels into roll only

**Block (hold RMB):**
- Cost: 0 stamina to hold; on hit, drains stamina = 50% of damage taken
- Reduces incoming damage by 80%
- Tap RMB within 0.15s before hit: **parry**

**Parry (tap RMB just before incoming hit):**
- Window: 0.15s pre-hit
- Effect: enemy staggered for 1.5s, opens **riposte** prompt
- Riposte: instant 60 damage, cinematic camera, hitstop 250ms, signature SFX

**Roll (Space):**
- Cost: 25 stamina
- Duration: 0.5s, **i-frames 0.10s–0.35s**
- Distance: 4.0m in input direction (or backward if no input)
- Cannot be cancelled

**Holy Bolt spell (Q):**
- Cost: 20 Faith
- Damage: 50 (projectile)
- Cast time: 0.8s (interruptible — taking damage cancels and refunds 50% Faith)
- Travel speed: 18 m/s, homes weakly toward nearest enemy in 30° cone

---

## 7. Stamina & Faith Economy

The economy is tuned so a careful player can sustain output indefinitely against a single enemy, but **cannot sustain against three without rolling and disengaging**.

- Light combo (3 hits) costs 36 stamina, takes ~1.5s including recovery.
- Stamina regen of 25/s means 1.5s recovery = 37.5 stamina returned — net positive 1.5 stamina per full combo.
- One roll (25 stamina) costs roughly one combo's worth.
- **Roll → combo → roll → combo** is stamina-neutral against a non-pressuring enemy; adding a block or sprint to that loop forces a recovery beat.

Faith regen is deliberately slow (5/s) so Holy Bolt is a **threat tool, not a DPS tool** — one bolt per wave at most.

---

## 8. Enemy Designs

### Cultist (light melee)

Fragile, fast, swarming. One-shot with heavy attack, two-shot with light attacks, but they come in groups.

| Stat | Value |
|---|---|
| HP | 40 |
| Move speed | 4.5 m/s |
| Aggro range | 25m |
| Score on kill | 10 |

**Attacks:** Slash (0.5s telegraph, 8 dmg); Lunge (0.8s telegraph, 12 dmg, parry-able, exposed 1.0s on miss).

**AI:** Aggressive, surrounds player, commits to attacks even mid-others-swing. Backs off briefly if hit. No block.

### Wraith (heavy melee)

Slow, tanky, methodical. Forces player out of light-attack spam.

| Stat | Value |
|---|---|
| HP | 120 |
| Move speed | 2.0 m/s |
| Aggro range | 30m |
| Score on kill | 30 |

**Attacks:** Heavy horizontal swing (1.2s, 25 dmg, 3m reach); Overhead slam (1.5s, 30 dmg + AoE, **prime parry target**, riposte = 90 dmg).

**AI:** Methodical. Won't engage while another Wraith is mid-attack within 4m (they queue). Stops moving during telegraphs.

### Pale Sovereign (boss) — see §10.

---

## 9. Wave Structure

Total active combat: ~25 min. Inter-wave breathers + boss = full run 30–40 min.

| # | Title | Composition | Duration | Lighting | Notes |
|---|---|---|---|---|---|
| 1 | The Faithful | 3 Cultists | 30s | Warm dusk | Tutorial wave |
| 2 | The Devoted | 5 Cultists | 45s | Dusk | First pressure test |
| 3 | The Risen | 4 Cultists, 1 Wraith | 60s | Dusk→twilight | Wraith intro |
| 4 | The Patient | 2 Wraiths | 90s | Twilight | Tempo shift; tests parry |
| 5 | The Tide | 6 Cultists, 2 Wraiths | 75s | Night, moonrise | Hardest mixed wave |
| 6 | The Final Watch | 4 Cultists → 1 Wraith → 4 Cultists (sub-waves) | 90s | Full night | Resource management, sub-waves spaced 20s |
| 7 | **The Pale Sovereign** | Boss + 2 summoned Cultists mid-fight | 3–5 min | Boss-emissive | See §10 |

**Inter-wave breather:** 5s. Wave banner fades centre-screen. Player heals +20 HP. Audio shift — a breath, a distant bell, wind picks up.

---

## 10. Boss Fight — The Pale Sovereign

The Pale Sovereign is the only fight in the game with a name and a moveset of its own. He is taller than the knight (1.4× scale), wears a tattered crown, and carries a two-handed greatsword that drags sparks along the stone. Memory of the first dark — warden of the threshold.

| Stat | Value |
|---|---|
| HP | 800 |
| Move speed | 3.0 m/s |
| Score on kill | 250 |

### Phase 1 (100% → 50% HP)

**Sword combo** — 2-hit horizontal slash, 1.0s telegraph on first, 0.4s on second, 35 dmg each. Second hit parry-able.

**Charge** — telegraphed by foot-stamp, 1.0s wind-up, 8 m/s line across arena, 50 dmg. **Vulnerable on miss for 1.5s** (back exposed).

**Summon** — at 75% and 60% HP, raises sword, summons two Cultists from altar. 2.0s cast, interruptible by 200+ dmg in one hit.

### Phase 2 (50% → 0% HP)

Triggered by cinematic beat: roar, camera shakes, music shifts to harder layer.

**Shockwave** — slams sword into ground, AoE ring expanding to 6m over 0.5s, 25 dmg, knockdown. Cannot be parried; **must be rolled or jumped over**.

**Combo extension** — sword combo becomes 3 hits. Third is overhead slam, parry-able.

**One-time summon** — at 25% HP, summons 2 Wraiths once.

### Win

HP = 0 → kneels. Camera pans, music swells, fade to white. Victory screen, final stats.

---

## 11. Arena Design

A single roughly circular courtyard, **~80m × 80m core** combat arena, surrounded by a wider walkable perimeter (~120m × 120m outer bound). Centre-point is a broken altar (Sovereign spawn). Player begins at the south-facing breach in the outer wall, **outside the combat arena**.

### Approach phase

Player spawns in perimeter — free to walk around outer wall, into/out of southern breach, to observe arena from threshold of altar circle. No enemies. Ambient audio. Lighting = dusk-of-wave-1 state.

Crossing the **altar threshold** (marked stone ring ~20m from altar) triggers wave 1. Threshold is one-way: once crossed, no leaving until victory or death.

### Combat arena (interior)

- **Central altar** — raised stone dais, ~3m diameter. Boss spawn point.
- **Four braziers** — cardinal points around altar, lit, flickering point lights.
- **Partial walls** — ruined stone, waist-to-shoulder height in places. LoS breaks but **no movement collision**.
- **Broken pillars** — six, scattered, full height, strong shadows.

### Perimeter (exterior)

- **Outer wall** — ruined, southern breach is the only player entry.
- **Dead trees** — three or four, atmospheric only.
- **Rubble and rocks** — scattered.
- **Soft wall** — invisible bound at ~60m from arena centre.

**Spawn points:** Eight pre-placed enemy spawn nodes inside arena perimeter.

---

## 12. Atmosphere & Art Direction

The visual language is **late-medieval Northern European at dusk into night**. Cold blues, warm braziers, no saturation in midtones.

**Palette (target):**
- Sky: deep desaturated cobalt at full night, transitioning from warm amber dusk
- Stone: bleached grey-brown
- Cultist robes: muted maroon and ash
- Wraith: pale bone-white with green-tinted emissive eyes
- **Pale Sovereign:** bleached white armour with cyan emissive runes
- Knight: dark steel with crimson cape (visual hero against the palette)

**Lighting curve through the run:**

| Wave | Sky | Sun/Moon | Ambient | Brazier weight |
|---|---|---|---|---|
| Approach | Warm amber, sun visible | Sun setting | 0.7 warm | Subtle, lit |
| 1 | Warm amber | Sun low, warm | 0.6 warm | Subtle |
| 3 | Cobalt/violet | Sunset gradient | 0.4 cool | Visible |
| 5 | Deep blue-black | Moon high | 0.2 cold | Dominant warm |
| 7 | Near-black | Moon eclipsed | 0.15 cold + boss emissive | Boss-lit |

Lighting transitions interpolate smoothly across each inter-wave breather. **Single most important atmospheric system in the game; must not be cut.**

**Post-processing:**
- ACES tone mapping (always)
- Bloom (subtle, on emissives only)
- Vignette (subtle)
- Optional chromatic aberration on hit (0.5s pulse)

No motion blur, no DoF.

---

## 13. Audio Brief

Audio carries 40% of atmosphere. Disproportionate polish time here.

**Music** — three tracks, all instrumental, CC0/CC-BY:

1. **Ambient watchtower** (approach + waves 1–3) — low drone, distant bell, ~3 min loop
2. **The dark hours** (waves 4–6) — strings on the drone, building tension
3. **The Pale Sovereign's vigil** (boss) — orchestral with choir, ~4 min

Tracks crossfade at wave transitions over 3 seconds.

**SFX inventory:**

- Sword attacks: 3 swing variations, 4 impacts (flesh, armour, parry-clang, miss)
- Footsteps: 3 stone variants
- Player: roll grunt, parry success, hit vocal (2), death
- Enemy vocals: Cultist death cry (3), Wraith moan (loop ambient), **Pale Sovereign roar** (boss-only)
- Brazier crackle (loop, 3D positional)
- Wind ambience (loop, ducked under combat)
- Distant raven calls (random, every 30–90s)
- UI: menu hover, menu click, wave banner sting, victory chime, death chime

**Audio library:** miniaudio — single-header C, 3D positional, OGG/MP3 support.

**Bus structure:** Master → {Music, SFX, Ambience, UI}. Volumes saved in settings.

---

## 14. Polish & Juice Specification

This determines whether *Vigil* reads as student work or shippable. Every item below must ship.

| Effect | Trigger | Spec |
|---|---|---|
| **Hitstop** | Any landed attack | 80ms global timescale = 0 |
| **Heavy hitstop** | Heavy attack land, riposte | 250ms timescale = 0 |
| **Screen shake** | Heavy land, boss shockwave, taking damage | 0.15s, amplitude 4–8px, decays smoothly |
| **Hit flash** | Enemy taking damage | Material tint white for 60ms |
| **Player hit flash** | Player taking damage | Full-screen red vignette pulse, 200ms |
| **Damage numbers** | All damage dealt | Float upward 60px over 1s, colour-coded (white/yellow/purple/gold) |
| **Parry effect** | Successful parry | 100ms zoom + 50% slowdown for 200ms; white radial flash; signature clang |
| **Sword trail** | During attack swing | Real ribbon, last 8 frames of blade tip |
| **Footstep particles** | Walking on stone | Small dust puff every step |
| **Brazier embers** | Continuous | ~20 particles per brazier |
| **Wave transition** | Inter-wave breather | Banner fade in/out, 0.3s slow-mo, lighting interpolation begins |
| **Boss intro** | Wave 7 start | 3s camera pan from player to altar, music sting, ambient drops, vignette pulse, brazier hard flicker |
| **Death sequence** | Player HP = 0 | Slow-mo to 30%, camera drops to ground over 2s, fade to black, death screen |
| **Victory sequence** | Boss HP = 0 | Sovereign kneels, 4s pan, fade to white, victory screen |
| **Enemy death** | Enemy HP = 0 | Hit react → death anim → 2s fade-out (no ragdoll) |

---

## 15. UI / HUD Specification

### 15.1 Main Menu

The main menu is the player's first 5 seconds of *Vigil*. It carries 90% of the game's tonal weight before any combat happens. It must look like a shipped game.

**Reference target:** Omma-generated mockup, pinned as the canonical Phase 7 visual reference. The mockup is a pure visual spec — not implementation. Build path is custom Vulkan UI pass (Day 35), reproducing the mockup's design fidelity in-engine.

**Items, top to bottom:**

| Label | Rune | Action |
|---|---|---|
| Begin the Vigil | ⚔ | New run; loads approach phase |
| Codex | ✦ | Lore overlay (worldbuilding text, ~200 words) |
| Settings | ⊕ | Settings overlay (see §15.4) |
| Abandon Post | ☽ | Quit confirmation overlay |

Separators between (Begin) / (Codex, Settings) / (Abandon Post).

**Title block:**
- Eyebrow: empty for v1.0
- Title: **VIGIL** in Cinzel Decorative 700, gold metallic gradient
- Ornamental rule: line-diamond-line
- Subtitle: *Hold the Ruin — or be Forgotten* (EB Garamond italic)

**Background:** procedural sky gradient + radial "ruin glow" at lower centre + drifting ash motes + ember particles + animated fog layers + vignette + scanlines.

**Corner elements:**
- Top-right: WAVES badge showing 7 gold diamond pips + 1 red boss pip
- Bottom-left: version tag `v0.1.0 — Vulkan 1.2`
- Bottom-centre footer: `DARK FANTASY ◆ COMBAT GAUNTLET ◆ C++20 · VULKAN 1.2`

### 15.2 In-Combat HUD

Minimal, on-screen only when necessary.

**Always visible during combat:**
- HP bar (top-left, ~200px, dark frame, red fill, no number)
- Stamina bar (under HP, smaller, green fill)
- Faith bar (under Stamina, smallest, blue fill)
- Wave indicator (top-centre, small Roman numeral)

**Not visible during approach phase:** All HUD hidden until first wave begins.

**Not visible:**
- No minimap (one arena; visible as-is)
- No compass
- No score/kill counter during combat
- No enemy HP bars by default — but the **Pale Sovereign** gets a boss HP bar at bottom centre during wave 7

**Wave banner:** Centre-screen, large Cinzel serif, Roman numeral + wave title. Fades in 0.5s, holds 2s, fades out 1s.

**Boss HP bar:** Bottom centre, wide thin bar, dark frame, name "The Pale Sovereign" centred above, white-to-red gradient fill.

### 15.3 Codex Overlay

Single-page lore panel, dismissable with Esc or backdrop click. ~3 paragraphs of worldbuilding establishing the Sovereign's identity. Text only; no interactivity. Reads as in-fiction parchment.

### 15.4 Settings Overlay

| Setting | Type | Default | Notes |
|---|---|---|---|
| Master Volume | 0–100 slider | 80 | |
| Music Volume | 0–100 slider | 80 | |
| SFX Volume | 0–100 slider | 80 | |
| Brightness | 0.5–1.5 slider | 1.0 | Multiplies tone-mapping exposure |
| Render Scale | Low / Med / High | High | 0.75× / 1.0× / 1.25× internal resolution |
| Display Mode | Fullscreen / Borderless / Windowed | Borderless | |
| Resolution | enum list | 1920×1080 | Greyed out in Borderless/Fullscreen |
| V-Sync | On / Off | On | FIFO ↔ Mailbox |
| Controller | Auto / Force KB+M / Force Pad | Auto | |

Settings persist via §16 save file.

### 15.5 Death / Victory Screens

- **Death:** "You Fell" + run stats (wave reached, time, hits taken, parries landed) + "Rise Again" / "Main Menu"
- **Victory:** "The Sovereign Has Fallen" + same stats + "Main Menu"

### 15.6 Visual Token Reference

All UI uses these tokens — push constants in shaders, named constants in C++. Reproduced from the Omma mockup for traceability.

**Palette (linear sRGB, vec4):**

| Token | Hex | vec4 |
|---|---|---|
| `GOLD` | #c8a96e | `vec4(0.784, 0.663, 0.431, 1.0)` |
| `GOLD_BRIGHT` | #f0d090 | `vec4(0.941, 0.816, 0.565, 1.0)` |
| `GOLD_DIM` | #7a6040 | `vec4(0.478, 0.376, 0.251, 1.0)` |
| `BLOOD` | #8b1a1a | `vec4(0.545, 0.102, 0.102, 1.0)` |
| `BLOOD_BRIGHT` | #c0392b | `vec4(0.753, 0.224, 0.169, 1.0)` |
| `ASH` | #d4cfc8 | `vec4(0.831, 0.812, 0.784, 1.0)` |
| `ASH_DIM` | #888070 | `vec4(0.533, 0.502, 0.439, 1.0)` |
| `VOID_BG` | #08070a | `vec4(0.031, 0.027, 0.039, 1.0)` |

**Typography:**

| Use | Font | Weight | Tracking | Source |
|---|---|---|---|---|
| Title (VIGIL) | Cinzel Decorative | 700 | 0.08em | Google Fonts (SIL OFL) |
| Menu items, settings labels | Cinzel | 400 | 0.22em–0.28em | Google Fonts (SIL OFL) |
| Subtitle, body, lore | EB Garamond | 400 italic | 0.12em | Google Fonts (SIL OFL) |

**Animation timings:**

| Effect | Duration | Easing |
|---|---|---|
| Title rise on load | 1.6s | `cubic-bezier(.22,1,.36,1)` |
| Panel rise on load | 1.8s (0.3s delay) | `cubic-bezier(.22,1,.36,1)` |
| Menu item hover transition | 0.22s | linear |
| Accent bar scale-in | 0.18s | `cubic-bezier(.22,1,.36,1)` |
| Overlay open/close | 0.3s | linear |
| Wave banner fade | 0.5s in / 2s hold / 1s out | linear |
| Footer fade-in | 2.5s (1s delay) | linear |
| Version tag fade-in | 3s (1.2s delay) | linear |

**Background particles (per Omma mockup):**

| Element | Count | Notes |
|---|---|---|
| Ash motes | 180 | Drift up + lateral, flicker |
| Fog layers | 4 | Lateral drift, alternating direction |
| Ground embers | 38 | Rise + fade |

**Render path declaration:** custom Vulkan UI pass. MSDF font atlas (stb_truetype-generated). UI fragment shader implements gradient text, vignette, scanlines, ember particles. ImGui kept for debug-only overlays, stripped in release builds.

---

## 16. Meta & Scoring

Deliberately tiny. Two things persist between runs, stored at `~/Library/Application Support/Vigil/save.dat` on macOS, `%APPDATA%/Vigil/save.dat` on Windows:

1. **Best run** — highest wave reached, fastest victory time, lowest hits-taken on a victory.
2. **Settings** — see §15.4.

No unlocks, no difficulty modes (NG+ is a post-launch stretch), no cosmetics, **no mid-run save/resume**.

---

## 17. Technical Architecture

| Layer | Choice | Rationale |
|---|---|---|
| Language | C++20 | Consistency with Mythbreaker, Legionfall |
| Graphics | Vulkan 1.2 | Consistency with portfolio; MoltenVK on macOS |
| Build | CMake 3.25+ | Standard |
| Windowing/input | SDL3 | Mature, controller support, cross-platform |
| Mesh format | glTF 2.0 (.glb) via cgltf | Industry standard |
| Asset pipeline | Tripo → Blender → Mixamo → FBX2glTF | See §22 |
| Math | glm | Header-only |
| Audio | miniaudio | Single-header, 3D, mixer |
| ImGui | Dev UI only (stripped in release) | Not used for main menu |
| Main menu UI | **Custom Vulkan UI pass** | MSDF text atlas, custom fragment shader for gradients/vignette/scanlines/embers. Reproduces §15.6 token spec in-engine. Day 35 implementation. |
| ECS | entt (single-header) | Production-grade |
| Job system | Custom | Worker per core, lock-free queue |
| Renderer | Forward+ | Simple, fast for one arena |
| Shadows | CSM (3 cascades) + brazier point shadows | Cuttable |
| Animation | Skeletal, glTF skin + animations, blend tree | Week 2 |
| Post | Tone map (ACES), bloom (emissive-only), vignette | Minimal |
| Particles | CPU-side, instanced quads | <500 concurrent |

**Target performance:** Locked 60fps on M3 MacBook Air at 1440×900. Stretch: 120fps on M3 Pro / Max.

**Code organisation:**

```
vigil/
├── CMakeLists.txt
├── assets/
│   ├── characters/{knight,cultist,wraith,pale_sovereign}/
│   ├── environment/
│   ├── vfx/
│   ├── audio/
│   ├── hdri/
│   └── fonts/                      (MSDF atlases for UI)
├── shaders/                        (GLSL → SPIR-V via glslangValidator)
├── src/
│   ├── core/                       (allocators, jobs, logging, Window, VulkanContext)
│   ├── render/                     (Swapchain, Pipeline, Renderer)
│   ├── ui/                         (menu, HUD, overlays — custom Vulkan UI)
│   ├── anim/
│   ├── ecs/
│   ├── game/
│   └── main.cpp
└── third_party/
```

---

## 18. Asset Bill of Materials

**Tripo for all 3D content. Mixamo for all character animation via auto-rigging. Polyhaven for skies/HDRIs. Free packs for VFX/audio.** Detailed per-character pipeline: `vigil_tripo_mixamo_pipeline.md`.

### 18.1 Characters

| Asset | Source | Notes |
|---|---|---|
| Knight | Tripo (mesh in hand) + Mixamo | Day 6 validates pipeline end-to-end |
| Cultist | Tripo + Mixamo | Robed silhouette, 5–15k tris |
| Wraith | Tripo + Mixamo | Gaunt undead, two-handed sword |
| **Pale Sovereign** | Tripo + Mixamo | Larger humanoid, tattered crown, greatsword; **scaled 1.4× at runtime** (not in Tripo gen) |

### 18.2 Animations (all Mixamo)

Same Mixamo skeleton across all four humanoids. Per-character animation sets per `vigil_tripo_mixamo_pipeline.md` §4.

### 18.3 Environment

| Asset | Source |
|---|---|
| Ruined keep walls | Tripo (5–7 modular pieces, 4m grid) |
| Broken altar | Tripo (hero piece) |
| Broken pillars | Tripo (3 variants) |
| Brazier | Tripo (1 model, instanced ×4) |
| Outer keep wall | Tripo |
| Dead trees | Tripo (3 variants) |
| Rocks / rubble | Tripo (5–6 variants) |
| Stone floor texture | Polyhaven (4K, tileable, full PBR) |
| Dirt/snow ground | Polyhaven (optional second layer) |

### 18.4 VFX

| Asset | Source |
|---|---|
| Spark, smoke, ember, dust textures | Kenney particle pack / itch.io |
| Holy Bolt projectile mesh | Tripo (crystal/orb) |
| Blood/dust impact decals | Free decal pack |

### 18.5 Sky / HDRI

| Asset | Source |
|---|---|
| Dusk HDRI, Night HDRI | Polyhaven |
| Moon billboard texture | Free / self-made |

### 18.6 Audio

| Asset | Source |
|---|---|
| Music tracks (3) | Pixabay Music, FreePD, OpenGameArt — CC0 or CC-BY |
| SFX library | Sonniss GameAudio bundle, FreeSound, Kenney audio |
| Wind ambience, raven calls | Same |

### 18.7 UI Fonts

| Asset | Source |
|---|---|
| Cinzel Decorative (700) | Google Fonts (SIL OFL) |
| Cinzel (400) | Google Fonts (SIL OFL) |
| EB Garamond (400 italic) | Google Fonts (SIL OFL) |

Fonts baked into MSDF atlases at build time via msdf-atlas-gen, output to `assets/fonts/`.

**Attribution:** `CREDITS.md` lists every CC-BY asset by source URL + author. Tripo and Mixamo mentioned in devlog.

---

## 19. Production Scope Cuts

**Mandatory order of cuts if behind schedule.** Cut from top down — never skip a higher cut to save a lower one.

1. Bloom + vignette post-processing (ship ACES only)
2. Point-light shadows from braziers (sun CSM only)
3. Pale Sovereign phase 2 mechanics (ship phase 1 moveset only)
4. Wraith overhead attack (horizontal only)
5. Sprint mechanic (walk only)
6. Wave 6 sub-waves (treat as a single wave)
7. **Parry mechanic entirely** (block-only combat). Decision gate: end of Phase 3 (Day 15).
8. Spell entirely (drop Faith bar, drop Q binding)
9. Controller support (KB+M only)
10. Save system (in-memory session high score only)
11. Main menu approach phase (cut to wave 1 directly)
12. Custom Vulkan UI parity with mockup (fall back to a simpler styled ImGui main menu — accept the visual hit, ship)

Items 1–4 are invisible if cut cleanly. Items 5–12 are visible compromises and should be avoided.

**Never cut:** combat feel, lighting curve, audio mix, the Pale Sovereign existing.

---

## 20. Success Criteria

- [ ] Full loop runs menu → victory/death → menu, no crashes in 2hr test
- [ ] Stable 60fps on M3 MacBook Air at 1440×900
- [ ] All seven waves play, all enemy types appear, Pale Sovereign fights through phase 1 minimum
- [ ] Hitstop, screen shake, hit flash, damage numbers, parry effect all implemented
- [ ] Lighting interpolates correctly across all wave transitions
- [ ] Three music tracks play and crossfade
- [ ] At least three impact SFX variations and footsteps audible
- [ ] Best run is saved and displayed
- [ ] Settings menu functional (volumes, brightness, render scale)
- [ ] Builds cleanly on macOS via single CMake invocation
- [ ] Main menu visually matches Omma mockup reference at acceptable parity
- [ ] 60-second trailer cut from gameplay looks like a real game

**Definition of "published":**

- **itch.io page live** with description, 4 screenshots, 60-second trailer, devlog, macOS + Windows builds. Free or pay-what-you-want.
- **Steam Coming Soon page submitted** (live within Valve's 5-day review window). Capsules, trailer, screenshots, description.
- Steamworks Direct application Day 36; $100 USD fee paid.

Steam *launch* is a stretch goal for week 9–10 post-roadmap.

---

## 21. Resolved Open Questions

All open questions resolved through v1.0 → v1.2.

| # | Question | Resolution |
|---|---|---|
| 1 | Single arena or arena-with-perimeter? | **Arena with perimeter.** §11. |
| 2 | True parry, or block-only? | **True parry, Day-15 cut gate.** §19 item 7. |
| 3 | itch.io only, or push to Steam? | **Both.** Steam Coming Soon by week 8. |
| 4 | Knight gender / look | **Male.** Tripo + Mixamo. |
| 5 | Mixamo vs custom rig for boss? | **Mixamo via Tripo auto-rigger.** Scale 1.4× at runtime. |
| 6 | Languages? | **English only at v1.0.** |
| 7 | Boss name: Pale King or Pale Sovereign? | **Pale Sovereign.** §10, retconned throughout. |
| 8 | Main menu reference / build path? | **Omma mockup pinned as Phase 7 reference; custom Vulkan UI build path.** §15, §17. |
| 9 | "Continue" menu item — resume mid-run? | **Removed.** Single-run game; no resume support. |
| 10 | Settings: Validation Layers toggle? | **Stripped.** Dev-only concern; replaced with Brightness + Render Scale. §15.4. |

No further blocking questions. New questions → logged in `devlog.md`, decided within 24h, recorded in v1.3 if material.

---

## 22. Asset Pipeline Reference

Detailed in `vigil_tripo_mixamo_pipeline.md`. Summary:

**Per humanoid character:**

1. Tripo generation (mesh + diffuse, FBX)
2. Blender pass (strip Tripo auto-rig, T-pose facing -Z, scale to ~1.8m, export static FBX)
3. Mixamo auto-rig (upload, 7 markers, bind)
4. Animation pull (T-pose with skin once, then animations without skin)
5. FBX2glTF batch convert to `.glb`
6. Verify in browser glTF viewer

**Per environment asset:**

1. Tripo generation
2. Blender pass (scale, orientation, transforms)
3. FBX2glTF convert

**Time budgets:**
- Per character: 45 min — 2 hr
- Per environment piece: 10–20 min
- Total Tripo time: ~4 hr across weeks 1, 4, 5

**Critical day:** Day 6 — pipeline validation on the existing knight FBX. Recovery if Mixamo auto-rigger rejects: fall back to a Mixamo pre-made character.

---

**End of GDD v1.2.** Production day 3 (rendering bring-up) follows.
