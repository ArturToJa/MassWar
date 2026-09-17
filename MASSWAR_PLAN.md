# MassWar — UE 5.7.4 Modular Mass-Entity RTS Plugin Suite

## Context

The user is building a plugin suite, **MassWar**, as the foundation for a large-scale war/RTS game:
handle huge numbers of units, network-replicate them, support mouse selection, drive AI with
StateTree, support fog-of-war-gated replication (enemies out of sight of any friendly unit are not
replicated to a client at all — simulation always keeps running on the server regardless of
visibility), and abstract AI/rendering so a unit can be a lightweight Mass entity *or* a full
`Pawn`/`Character`, with visual embodiment swappable by distance. The user also asked that this be
split into a required core plugin plus optional feature plugins users can add individually, and asked
for the work to proceed as **one small, independently testable pass at a time** — implement, verify it
compiles, hand over a precise in-editor test recipe, wait for the user to confirm it works, then move
to the next pass. This file is both the architecture reference and the ordered pass list.

**Project/plugin already exist**, prepared by the user ahead of this session:
- Project: `D:\MassWarExample\MassWarExample.uproject` (UE 5.7.4, builds already). Game module
  `MassWarExample` depends on `Core, CoreUObject, Engine, InputCore, EnhancedInput`.
- `D:\MassWarExample\Plugins\MassWar` — currently the unmodified stock empty-plugin wizard output.
  This becomes the **Core** plugin below; the other plugins are new siblings under `Plugins\`.
- `.uproject` already enables `MassAI`, `MassCrowd`, `MassEntity` (deprecated wrapper, harmless),
  `MassGameplay`, `StateGraph` (unrelated generic FSM plugin, new in 5.7 — not used here),
  `GameplayStateTree`, `ModelingToolsEditorMode`. `StateTree` will be added explicitly.
- `Content/` is empty; a minimal flat demo level will be created (Pass 1) and set as the default map.
- Demo/test-harness content (two-team spawn logic, the test level) lives in the **project's own game
  module**, not in any plugin — plugins stay reusable/game-agnostic; only a consuming project should
  contain "spawn 200v200 at start" logic.

Engine facts confirmed directly against the 5.7.4 install (not guessed) — see prior research: MassEntity
is now engine-core; `MassGameplay` ships `MassCommon/MassSpawner/MassSimulation/MassMovement/
MassRepresentation/MassReplication/MassSignals/MassLOD`; `MassAI` ships `MassNavigation` (has
`FMassMoveTargetFragment`), `MassAIBehavior` (has the Mass↔StateTree bridge:
`MassStateTreeTrait/Schema/Processors`), `MassAIReplication`; click-selection hook `UMassAgentComponent`
(module `MassActors`) maps an actor hit back to `FMassEntityHandle`; replication building blocks
`UMassReplicationTrait`, `MassClientBubbleInfoBase/Handler/SerializerBase`,
`MassReplicationTransformHandlers` already exist and are what MassWarReplication builds on.

## Plugin architecture

```
MassWar (Core, required)
 ├─ MassWarCombat        (optional, needs: Core)
 ├─ MassWarStateTreeAI   (optional, needs: Core)
 ├─ MassWarSelection     (optional, needs: Core)
 ├─ MassWarReplication   (optional, needs: Core)
 ├─ MassWarFogOfWar      (optional, needs: Core, MassWarReplication)
 ├─ MassWarEmbodiment    (optional, needs: Core, MassWarStateTreeAI)
 ├─ MassWarNavigation    (optional, needs: Core)
 ├─ MassWarFormations    (optional, needs: Core, MassWarSelection)
 ├─ MassWarMinimap       (optional, needs: Core, MassWarFogOfWar)
 ├─ MassWarWeapons       (optional, needs: Core, MassWarCombat)
 └─ MassWarBallistics    (optional, needs: Core, MassWarWeapons)
```

Six plugins now depend on another optional plugin instead of only Core — same documented-exception
pattern throughout, just more entries: FogOfWar→Replication, Embodiment→StateTreeAI,
Formations→Selection (formation orders are issued from Selection's group-order flow),
Minimap→FogOfWar (so the minimap can hide what a team can't see; without FogOfWar installed the
minimap just shows every unit, no fog), Weapons→Combat (ammo/reload gates Combat's existing damage
processor rather than replacing it), and Ballistics→Weapons (a flying bullet entity needs to know
its muzzle origin, travel speed, and visual from the weapon that fired it). MassWarNavigation stays
Core-only on purpose (see its writeup below) even though it adds a StateTree task, to avoid another
exception. Dedicated-server packaging (added below) is not a plugin — it's a validation pass over
the whole suite.

**MassWar (Core)** — the only hard requirement. Owns: `FMassWarTeamFragment` (+ affiliation helper:
Owned/Allied/Enemy/Neutral), `FMassWarOrderFragment` (Idle/Move/Attack + destination + target), a
stable cross-network entity id fragment, `FMassWarUnitStateView` (the shared, non-`UObject` view
struct AI/gameplay code is written against — Mass-fragment-backed for now; an actor-backed constructor
is added by Embodiment), `UMassWarUnitTraitBase`, base spawn conventions. No combat, no AI, no
replication, no selection UI — those are add-ons that only ever depend on Core, never on each other
directly, which is what makes them pick-and-choose:

- **MassWarCombat**: Health + combat-params fragments, `MassWarDamageProcessor` (server-only) that
  resolves an active Attack order into periodic damage + death. Knows nothing about AI or players —
  just reacts to Core's Order fragment.
- **MassWarStateTreeAI**: `MoveTo` / `HasEnemyInRange` / `FindNearestEnemy` / `AttackTarget` StateTree
  nodes, written once against `FMassWarUnitStateView`. `AttackTarget` only ever calls
  `View.RequestAttack(...)` (a Core concept) — it does not depend on MassWarCombat. If Combat isn't
  installed, that call is a harmless no-op; if it is, Combat's processor picks up the resulting Order.
- **MassWarSelection**: selection subsystem, marquee-box + click selection, RTS camera pawn, the order
  RPC component. Only needs Core's Order dispatch + stable ids — works fine single-player with no
  Replication plugin installed at all (RPCs just resolve locally on a listen server).
- **MassWarReplication**: the Mass client-bubble/handler/fast-array machinery that actually
  synchronizes Core's stable ids across the wire, replicating transform/team/health. Exposes a
  **relevancy-filter delegate** (default: always relevant) that other plugins can bind to without
  Replication ever knowing they exist.
- **MassWarFogOfWar**: server-only `MassWarVisibilitySubsystem` computing, per team, which enemy
  entities are within sight radius of any of that team's own units (radius check first; a
  line-of-sight raycast pass is added later, see Pass 10 below). Binds MassWarReplication's relevancy
  delegate: a candidate is bubble-relevant only if it's the viewer's own team **or** currently visible
  to that team. Losing sight removes it from the bubble immediately — no data reaches a client for
  anything it can't see, which is the correct anti-cheat property (unlike a client-side visual mask).
  Simulation never consults visibility — the server ticks every unit's Mass processors regardless,
  always. Pass 10 additionally gives each team a **last-seen memory**: instead of an out-of-sight enemy
  vanishing immediately, the last known position/pose is cached client-side and shown as a static
  "ghost" marker until real sight is regained or a configurable timeout expires — still driven entirely
  by what the relevancy delegate already revealed, never new information the server wouldn't have sent.
- **MassWarNavigation**: pathfinding-aware movement for Mass units, replacing/augmenting Core's
  straight-line `MassWarOrderMovementProcessor` with real obstacle avoidance around static geometry
  (NavMesh or ZoneGraph — confirm which fits Mass agents better against the 5.7.4 install before
  committing, both are plausible; ZoneGraph is Mass-native and more likely). Adds its own StateTree
  `MoveTo`-style task (its own `FMassWarSTTask_MoveToNav` or similar) directly against
  `StateTreeModule`/`MassAIBehavior`, the same way MassWarStateTreeAI's plain `MoveTo` does — so it
  only ever needs Core, not the StateTreeAI plugin itself; a tree built with the StateTree Builder API
  commandlet can pick whichever `MoveTo` task variant is available.
- **MassWarFormations**: turns a group-move order issued from Selection into per-unit destinations that
  preserve a formation shape (a couple of fixed presets — line and grid/box — not a formation editor)
  instead of everyone converging on one point. Hooks into Selection's existing order-dispatch path,
  computing individual offsets before calling the same Core order RPCs Selection already uses.
- **MassWarMinimap**: a simple 2D overlay reading `UMassWarUnitRegistrySubsystem` (position + team
  affiliation) to draw unit blips; camera-view frustum outline for click-free orientation. View-only —
  no click-to-move/select from the minimap in this pass. Binds FogOfWar's already-filtered client-side
  data when that plugin is installed (so the minimap can never show more than the 3D view already
  does); shows every unit unfiltered if FogOfWar isn't installed.
- **MassWarEmbodiment**: `MassWarUnitCharacter` (always-Pawn hero/vehicle units — real,
  server-authoritative, actor-replicated `ACharacter`s that opt out of Mass entirely, running
  `ST_MassWarUnit_Actor` via `UStateTreeComponent` on `MassWarUnitAIController`), and
  `MassWarUnitVisualCharacter` (a **cosmetic-only** near-LOD puppet for ordinary Mass-simulated units —
  reuses Mass's existing near-LOD "spawn a representation actor" mechanism, just upgraded to a real
  animated Character; mirrors entity fragments into itself each tick, has zero simulation authority).
  Adds the actor-side `UMassWarUnitStateComponent` backing for `FMassWarUnitStateView`. Both embodiment
  paths reuse the identical StateTree node code from MassWarStateTreeAI — that's the actual abstraction
  being delivered. Pass 7 also gets a skeletal-mesh/animation upgrade (real skeleton + anim blueprint
  driving idle/walk/attack/death on the near-LOD Character puppet; a baked animation-to-texture crowd
  technique — the same one Epic's own City Sample uses, and a natural fit since `MassCrowd` is already
  enabled — for far units so they visibly animate without ever spawning a skeleton per entity).
- **MassWarWeapons**: adds a per-unit loadout fragment (weapon type, magazine size, current ammo,
  reload time, rate of fire) that *gates* Combat's existing damage-per-interval processor rather than
  replacing it — a unit can only deal damage on a tick where it has ammo and isn't mid-reload, and
  firing decrements the magazine, empty triggers an automatic reload. Combat still owns health/death;
  Weapons only owns whether/when a shot is allowed to land.
- **MassWarBallistics**: a lightweight, Mass-entity-based cosmetic projectile system — firing a shot
  (once Weapons has already authoritatively decided it hits, server-side, same tick as today) spawns a
  short-lived bullet entity with its own tiny processor doing simple ballistic integration (velocity +
  gravity) from muzzle to target over real travel time, purely for visual flavor. Deliberately **not**
  used for hit resolution — damage timing and amount are decided instantly by Weapons/Combat as before,
  so a slow/dodged-looking bullet can never desync from the authoritative outcome. Only spawned for
  shots within a viewer's actual view (near/visible engagements); everything else stays a plain instant
  damage tick with no visual projectile at all, keeping the "huge numbers of units" cost bounded.

## Implementation passes — build, verify compile, test, confirm, then next

Each pass: I implement the C++ (and author any unavoidable editor assets via a headless Python script
against the existing project), verify a clean command-line build
(`D:\UE_5.7\Engine\Build\BatchFiles\Build.bat MassWarExampleEditor Win64 Development
-Project="D:\MassWarExample\MassWarExample.uproject"`), then hand you the exact PIE steps below to
confirm before I start the next pass. I have no GUI control of the Editor, so all Play-in-Editor
verification is done by you.

1. **Core bootstrap + spawn smoke test** — `MassWar` plugin: Team/Order/NetId fragments,
   `FMassWarUnitStateView` (Mass-only ctor), `UMassWarUnitTraitBase`, a `UMassEntityConfigAsset` using
   stock Mass movement + ISM representation traits (placeholder cube/capsule), a temporary demo
   GameMode in the project module spawning ~300 units scattered across a flat test level.
   **Test:** Play — see ~300 simple instances scattered in the level; nothing needs to move or fight
   yet. Confirms the build, the Mass pipeline, and Core's trait don't break anything.
2. **MassWarCombat** — Health/combat-params fragments, `MassWarDamageProcessor` (resolves an Attack
   order into periodic damage + death while target is in range, regardless of who issued the order).
   Demo GameMode gets a debug key binding that issues one Attack order from Team A to the nearest Team
   B unit. **Test:** Play, press the bound debug key, watch Team A's targeted units take repeated
   damage and Team B units die and disappear; an on-screen debug count of remaining units drops.
3. **MassWarSelection** — RTS camera pawn, drag-box/click selection with a selection ring, right-click
   move order, right-click-on-enemy attack order (replaces the Pass 2 debug key with real play).
   **Test:** Play, drag-select a group of Team A units, right-click open ground (they walk there),
   right-click a Team B unit (they walk over and fight it using Pass 2's damage pipeline).
4. **MassWarStateTreeAI** — the four StateTree nodes, `ST_MassWarUnit_Mass` asset (schema
   `UMassStateTreeSchema`), wired onto the unit config via `UMassStateTreeTrait`.
   **Test:** Play, give no orders at all — Team A and Team B (spawned at opposite corners) walk toward
   each other and fight entirely on their own, reusing the exact same damage/order pipeline already
   proven in Passes 2–3.
5. **MassWarReplication** — stable id sync, client bubble/handler/fast-array replicating
   transform/team/health, `UMassReplicationTrait` on the unit config.
   **Test:** Play with PIE set to 2 players (listen server + 1 client window). Confirm the client
   window mirrors the server window's battle in real time, and that selecting/ordering from the
   *client* window's player correctly commands the server-authoritative units.
6. **MassWarFogOfWar** — `MassWarVisibilitySubsystem`, binds the Replication relevancy delegate.
   **Test:** Same 2-player PIE, teams on opposing sides. Confirm each client only sees enemy units
   within sight of its own units — advance a squad toward the enemy and watch enemies appear as they
   come into range and disappear when out of range, independently on each client window.
7. **MassWarEmbodiment** — `MassWarUnitCharacter` (hero, `ST_MassWarUnit_Actor` +
   `UStateTreeComponent`), `MassWarUnitVisualCharacter` (near-LOD puppet swap), the actor-side
   `UMassWarUnitStateComponent`, plus the skeletal-mesh/animation upgrade (real skeleton + anim
   blueprint for the near-LOD Character puppet, animation-to-texture for far ISM units).
   **Test:** Play, spawn one hero-type unit among the Mass units and confirm it fights using the same
   AI logic; zoom the camera in on a cluster of ordinary Mass units and confirm they visually swap to
   animated Characters up close (playing walk/attack/death, not a static pose) and back to simple
   ISM shapes from a distance — and that even the distant ISM units are visibly animating (walk cycle
   at minimum), not sliding around rigidly.
8. **MassWarNavigation** — obstacle-aware pathfinding (NavMesh or ZoneGraph, TBD against the engine
   install) replacing straight-line movement; its own StateTree `MoveTo`-equivalent task.
   **Test:** Add a couple of blocking obstacles between the two teams' spawns. Play with no orders
   given (same as Pass 4) — units path around the obstacles to reach each other instead of walking
   into them or getting stuck at the edge.
9. **MassWarFormations** — group-move orders keep a line/grid formation shape instead of collapsing
   onto one point.
   **Test:** Play, drag-select a large group of Team A units, right-click open ground. Confirm they
   arrive roughly preserving a line or grid shape rather than all converging on the exact same spot.
10. **MassWarFogOfWar enhancements** — line-of-sight raycasts (an enemy within radius but hidden behind
    an obstacle is not visible) and last-seen memory (a ghost marker at the last known position/pose
    instead of instant vanish on lost sight).
    **Test:** Same 2-player PIE setup as Pass 6. Confirm a nearby-but-behind-cover enemy stays hidden
    until direct sight is clear, and that walking a squad away from a spotted enemy leaves a static
    ghost marker at its last seen position instead of it disappearing outright.
11. **MassWarMinimap** — 2D overlay showing unit blips (team-colored) and the camera frustum.
    **Test:** Play, confirm the minimap shows your own units and only the enemies your fog of war
    already reveals (matching the 3D view exactly); pan/zoom the main camera and confirm the frustum
    outline on the minimap tracks it.
12. **Dedicated-server packaging** — not a new plugin; validates that a true headless dedicated server
    build works with this plugin set (client-only plugins like Selection's camera/HUD and the Minimap
    overlay excluded from the server target; Core/Combat/StateTreeAI (or Navigation)/Replication/
    FogOfWar all still function headless).
    **Test:** Build the `MassWarExampleServer` target, launch it headless (`-server -log`), connect a
    normal client build to it, and confirm the same autonomous-battle-plus-orders behavior from the
    earlier passes plays out correctly with the dedicated server authoritative and the client purely a
    viewer/input source.
13. **MassWarWeapons** — loadout fragment (weapon type, magazine size, current ammo, reload time, fire
    rate) gating Combat's existing damage processor; automatic reload on empty magazine.
    **Test:** Play, give one unit a small magazine size via its entity config and watch (via the
    existing diagnostic log or a debug widget) its ammo count tick down while attacking, stop dealing
    damage during a visible reload pause once empty, then resume firing afterward — all still resolved
    through the same damage/death pipeline proven in Pass 2.
14. **MassWarBallistics** — Mass-entity-based cosmetic bullet projectiles (ballistic integration from
    muzzle to target over real travel time) for visible/near engagements only; damage timing/amount
    stays authoritative and instant via Weapons/Combat regardless of whether a bullet is spawned.
    **Test:** Play, zoom the camera in close on an active firefight and confirm visible bullets
    physically travel from shooter to target (with a slight arc/delay, not instant); zoom back out to
    a large-scale battle and confirm no visible slowdown and that far-away kills still happen on the
    same timing as before (i.e. bullets are cosmetic, never gating when damage actually lands).

Explicit scope notes for the new passes (to avoid quiet scope creep): Navigation covers static-geometry
avoidance only, not dynamic obstacle avoidance between moving units (that's still an implicit follow-up);
Formations ships two fixed presets, not a formation-editor UI; Minimap is view-only, no click-to-order;
Weapons ships one generic weapon archetype with configurable numbers, not a full weapon-catalog/itemization
system; Ballistics is a cosmetic-only projectile (no penetration, ricochet, or bullet-drop-affecting-hit-
chance — travel time and arc are visual, hit/miss and damage are decided before the bullet ever spawns).
Still not included anywhere in this plan: resource/production systems, cross-platform (Linux) dedicated
server packaging (Win64 only for Pass 12).

## Starting point

Begin with **Pass 1** now. I'll report back with the build result and the exact test steps before
touching Pass 2.
