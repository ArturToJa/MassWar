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
 ├─ MassWarEmbodiment    (optional, needs: Core)
 ├─ MassWarFormations    (optional, needs: Core, MassWarSelection)
 ├─ MassWarMinimap       (optional, needs: Core, MassWarFogOfWar)
 ├─ MassWarWeapons       (optional, needs: Core, MassWarCombat)
 └─ MassWarBallistics    (optional, needs: Core, MassWarWeapons)
```

Five plugins now depend on another optional plugin instead of only Core — same documented-exception
pattern throughout, just more entries: FogOfWar→Replication,
Formations→Selection (formation orders are issued from Selection's group-order flow),
Minimap→FogOfWar (so the minimap can hide what a team can't see; without FogOfWar installed the
minimap just shows every unit, no fog), Weapons→Combat (ammo/reload gates Combat's existing damage
processor rather than replacing it), and Ballistics→Weapons (a flying bullet entity needs to know
its muzzle origin, travel speed, and visual from the weapon that fired it). Navigation is not a plugin at
all - it is part of Core, next to the movement it extends (see its writeup below). Dedicated-server
packaging (added below) is not a plugin — it's a validation pass over the whole suite.

**MassWar (Core)** — the only hard requirement. Owns: `FMassWarTeamFragment` (+ affiliation helper:
Owned/Allied/Enemy/Neutral), `FMassWarOrderFragment` (Idle/Move/Attack + destination + target), a
stable cross-network entity id fragment, a life-state fragment (`FMassWarLifeFragment`: Alive/Dying - a
dead unit lingers as a Dying entity, replicated to clients, so its puppet can play a death animation before
`UMassWarDeathProcessor` destroys it), an attack-feedback counter (`FMassWarAttackFeedbackFragment`, bumped
by MassWarCombat on every landed attack and replicated, so each machine's puppet can play an attack montage),
`FMassWarUnitStateView` (the shared, non-`UObject` view
struct AI/gameplay code is written against, backed by Mass fragments), `UMassWarUnitTraitBase`, base spawn conventions. No combat, no AI, no
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
- **Navigation (part of Core, not a plugin)**: unit routing around static obstacles lives in Core next to
  the movement it extends. A unit opts in with `bUseNavMesh` on the MassWar Unit trait, which adds
  `FMassWarNavPathFragment`; `UMassWarNavPathProcessor` computes navmesh paths (game thread, capped per
  frame, a straight-line navmesh raycast first and real pathfinding only when blocked) and
  `UMassWarOrderMovementProcessor` steers along the waypoints - stop/arrival/facing rules are unchanged, only
  the steering point differs. NavMesh (Recast) was chosen over ZoneGraph after checking the 5.7.4 install:
  ZoneGraph is lane-based (baked lane network, City Sample-style crowds), while this game's units move
  point-to-point over open ground; the engine's own MassAI navigation/steering stack was likewise skipped
  (force-based crowd movement, far beyond "static geometry only"). No StateTree task is needed: the existing
  `MoveTo` writes an order, and how a unit walks it is the movement processor's business. Core's only new
  dependency is the engine `NavigationSystem` module.
- **MassWarFormations**: turns a group-move order issued from Selection into per-unit destinations that
  preserve a formation shape (a couple of fixed presets — line and grid/box — not a formation editor)
  instead of everyone converging on one point. Hooks into Selection's existing order-dispatch path,
  computing individual offsets before calling the same Core order RPCs Selection already uses.
- **MassWarMinimap**: a simple 2D overlay reading `UMassWarUnitRegistrySubsystem` (position + team
  affiliation) to draw unit blips; camera-view frustum outline for click-free orientation. View-only —
  no click-to-move/select from the minimap in this pass. Binds FogOfWar's already-filtered client-side
  data when that plugin is installed (so the minimap can never show more than the 3D view already
  does); shows every unit unfiltered if FogOfWar isn't installed.
- **MassWarEmbodiment**: cosmetic near-LOD Actor puppets for ordinary Mass-simulated units. The Mass
  entity is always the single source of truth (health, order, StateTree, replication never leave it);
  near the camera Mass's own representation system spawns an Actor as a *view* of it and
  `UMassWarVisualSyncProcessor` keeps that Actor placed/oriented on its entity every frame (data flows
  entity → actor only, so swapping representations transfers nothing). The plugin is framework-only:
  it ships the abstract `AMassWarUnitVisualCharacter` base and the `IMassWarVisualPuppet` contract, and
  the game defines the concrete Actor class (meshes, skeleton, anim blueprint) and configures it as the
  unit config's `HighResTemplateActor`. There is deliberately no separate "hero" unit type: every unit is
  a Mass entity. Pass 7 also gets a skeletal-mesh/animation upgrade (real skeleton + anim blueprint
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
7. **MassWarEmbodiment** — the abstract puppet base + `IMassWarVisualPuppet` + `UMassWarVisualSyncProcessor`
   (near-LOD puppet swap, entity stays authoritative), plus the skeletal-mesh/animation upgrade (real
   skeleton + anim blueprint for the near-LOD Character puppet, animation-to-texture for far ISM units).
   **Test:** Play; zoom the camera in on a cluster of ordinary Mass units and confirm they visually swap to
   animated Characters up close (playing walk/attack/death, not a static pose) and back to simple
   ISM shapes from a distance — and that even the distant ISM units are visibly animating (walk cycle
   at minimum), not sliding around rigidly.
8. **Navigation (in Core)** — obstacle-aware pathfinding on the level's NavMesh, opt-in per unit type
   (`bUseNavMesh`), replacing straight-line movement for units that enable it.
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
    overlay excluded from the server target; Core (incl. navigation)/Combat/StateTreeAI/Replication/
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

**MassWarFormations** (redesigned 2026-09-25 - supersedes the earlier "group-move shape only" idea; depends on Core +
Perception; Selection now depends on it): a formation is the unit of selection, orders and shared knowledge. Units carry
Core's `FMassWarFormationMemberFragment::FormationId` (replicated, so clients can group units); formation records
live server-side in `UMassWarFormationSubsystem`. Selecting any unit selects its whole formation; orders travel as
formation ids and the server checks ownership. Move: slots laid out for the shape (Blob/Line/Grid) around the
destination, each member walks to its own slot (loose crowd, not a marching line); several formations are placed side
by side. Attack an enemy formation: the formation assigns targets (nearest, per-target cap) and re-assigns as they die.
Creation: the spawner (project) groups units at spawn; player merge/split is phase 3. Phase 2 (done): members'
perception is pooled per formation every 0.25s (`FMassWarFormation::Knowledge`, up to 32 entries, freshest stimulus per
unit; anything one member sees/hears/is hurt by, all know), an idle formation auto-engages the nearest enemy formation
in its pooled sight within `AutoEngageRadius` and otherwise investigates a fresh noise/hit once (both per-formation
settings), and the StateTree "Perceived Enemy" evaluator can use the pooled knowledge (`Use Formation Knowledge`).
Player orders keep priority: auto behaviour only runs while the formation is idle.
**Deferred - phase 3, player merge/split (skipped by the user on 2026-09-25, to be picked up later).** The user wants
creation "both": spawner-defined (done) AND player-driven. Planned design: `UMassWarFormationSubsystem::Merge(ids)` -
all selected formations become one (settings from the largest, stays idle, members' `FormationId` rewritten, the
replicated id updates clients automatically); `Split(id)` - cut a formation in two along its widest spread axis
(each half gets its own id, both keep the old settings). Needs server RPCs on `UMassWarUnitOrderComponent`
(`ServerMergeFormations`, `ServerSplitFormation`, owner-checked like the orders) and two input actions bound in the
player controller - the input actions/mapping are editor assets, so the user creates those from a recipe. Open
questions to settle then: which keys, whether merge should refuse formations of different teams (yes, planned),
and whether a formation with an active order keeps or drops it on merge/split (planned: drops to idle).
The formation records already support it: membership is the units' `FormationId`, and `Refresh()` drops members
whose id no longer matches, so re-pointing ids is enough.

**Pass 10 - line of sight + ghosts (done 2026-09-25).** Core owns a shared `UMassWarLineOfSightSubsystem` (one ray from
eye height to the target on the Visibility channel; game-thread only; a per-frame budget `MassWar.LOS.RaycastsPerFrame`,
default 400, that callers check first - when it is spent they keep their last answer). MassWarFogOfWar: the visibility
processor no longer loops all pairs; it uses one hashed grid PER TEAM (a shared grid made lookups wade through the
candidate's own dense team) plus a per-team bounding-box early-out, then tests the nearest few in-range viewers for line
of sight, caching answers for 0.6s. Measured at 10,000 units: ~0.3 ms per 0.2s update with separated teams, ~1-3 ms with
both crowds converging (the first grid version was ~150 ms). MassWarPerception: sight now also needs line of sight
(`Require Line Of Sight`, default on, per-entry recheck interval 1s, same budget); it considers the 16 nearest
candidates so cover doesn't hog the 8 slots. Ghosts: `UMassWarGhostSubsystem` keeps the last known position/heading of
an enemy that leaves the local player's sight alive (removed when it is seen again or after `MassWar.Ghosts.Lifetime`,
default 10s); remote clients are fed by a new Replication notification (`OnClientAgentRemoved/Added` on the setup
subsystem), the listen-server/standalone player by visibility changes. No marker content ships: bind `OnGhostAdded` /
`OnGhostRemoved` or read `GetGhosts()`; `MassWar.Ghosts.Draw 1` draws a debug marker. Not done: ghosts are not cleared
when a friendly unit walks up and finds the spot empty (timeout only), and a unit that dies while unseen still leaves a ghost.

**MassWarPerception** (added 2026-09-23, depends only on Core; Combat and StateTreeAI depend on it): per-unit
senses in the spirit of Unreal's perception system - Sight (radius, lose-sight radius, vision cone, staggered
scans through a hashed grid), Hearing (noise events reported via UMassWarPerceptionSubsystem; Combat reports one
per landed attack, range `AttackNoiseRange`) and Damage (victim learns attacker + location, even out of sight).
Each unit keeps up to 8 perceived units (`FMassWarPerceptionFragment`: entity, last known location, strength,
last sense, in-sight flag, last damage/heard time) and forgets them after `MemoryDuration`; config is one const
shared fragment per unit type. Consumed by the StateTree evaluator "MassWar Perceived Enemy", the perception-based
counterpart of the omniscient "Find Nearest Enemy". Server-only, not replicated. Debug: `MassWar.Perception.Debug 1`.
Not included: line-of-sight raycasts (shared with the Pass 10 FogOfWar LOS work), team-level shared knowledge.

Explicit scope notes for the new passes (to avoid quiet scope creep): Navigation covers static-geometry
avoidance only; unit-vs-unit avoidance is a separate, also Core-owned mechanism (UMassWarSeparationProcessor:
hashed-grid local separation driven by FMassWarAvoidanceFragment, opt-in via the MassWar Unit trait's
"Avoid Other Units") - no prediction/RVO, so dense head-on crowds jostle rather than flow perfectly;
Formations ships two fixed presets, not a formation-editor UI; Minimap is view-only, no click-to-order;
Weapons ships one generic weapon archetype with configurable numbers, not a full weapon-catalog/itemization
system; Ballistics is a cosmetic-only projectile (no penetration, ricochet, or bullet-drop-affecting-hit-
chance — travel time and arc are visual, hit/miss and damage are decided before the bullet ever spawns).
Still not included anywhere in this plan: resource/production systems, cross-platform (Linux) dedicated
server packaging (Win64 only for Pass 12).

## Starting point

Begin with **Pass 1** now. I'll report back with the build result and the exact test steps before
touching Pass 2.
