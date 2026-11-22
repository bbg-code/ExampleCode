# World-space icons and interactions for Unreal Engine 5

A code sample taken from my personal project, an online PvPvE extraction shooter built on a source build of Unreal Engine 5.8. It contains two of the project's plugins:

- **RCCore**: small containers and a lambda-based tick function used across the project.
- **RCGUI**: a world subsystem that shows icons over world objects (pickups, doors, vehicles) to the local player, and the components for server-validated timed interactions with them.

Everything here is my own code. The rest of the project (characters, items, abilities, vehicle physics, audio) is not included.

## Layout

```
RCCore/
  Public/RCDoubleBufferArray.h      write one buffer while reading the last frame's
  Public/RCMappedDataArray.h        key -> index map over a dense array of (key, data), O(1) swap-remove
  Public/RCMappedArray.h            the same for keys only
  Public/RCCoreTickFunction.h       an FTickFunction that runs a lambda, for subsystems
RCGUI/
  Private/RCIconsSubsystem.*        owns every icon, time-sliced update, screen layout, focus
  Public/RCIconOwnerInterface.h     what the subsystem needs from an icon's owner
  Public/RCIconComponent.*          a scene component that registers an icon
  Public/RCWidgetReference.*        a widget created from editor data (class, size), with MVVM view models
  Public/RCIconViewModel.*          the icon's state for its widget
  Public/Interactions/
    RCIconInteractionComponent.*    an icon that can be interacted with: timed, validated where it is decided
    RCIconInteractionViewModel.*    the interaction's state for the widget
    RCInteractorComponent.*         on the pawn: starts the focused interaction, mirrors it to the server
```

## How the icon system works

**Ownership.** `URCIconsSubsystem` owns the data of every icon. A component registers in `BeginPlay`, gets a `uint32` handle back and unregisters in `EndPlay`. Callers never hold pointers into the subsystem. The subsystem keeps a weak interface pointer to the owner, so an owner destroyed without unregistering is skipped, not dereferenced.

**Two updates per frame, each where its data is.**

1. *Game tick* (`UpdateTick`, its own `FRCCoreTickFunction` in `TG_DuringPhysics`):
   - Updates the transform and range of at most `IconUpdatesPerFrame` icons (default 50), round-robin. The per-frame cost stays flat however many icons the level registers; a large level only takes longer to refresh each icon.
   - Collects the visible icons of a full pass into a double buffer. The list read this frame is always a complete pass, never one being written.
   - Fills the view models of the visible icons, and offers the focused icon's interaction to the local pawn's interactor.
2. *Slate pre-tick* (`PreSlateUpdateTick`, right before Slate lays out):
   - Resolves the view and projection once for all icons.
   - Fades widgets in and out, creating a widget when its icon becomes visible and destroying it once it has faded out, so only icons near the player cost a widget.
   - Projects each icon to the screen. An icon behind the camera is mirrored in front of it and pinned to the screen edge. An on-screen icon is only clamped when it would cross the edge.
   - Picks the focused icon: the one closest to the screen center, within a third of the screen. The next game tick reads it and clears it.

**Widgets through MVVM.** An icon's state (focused, distance, interaction progress) goes into its view models, and the widget Blueprint binds to them. The view models only notify on a change: the distance text is formatted again only when the whole meters change, not every frame. The subsystem itself only sets the widget's position and opacity.

**Machines.** Icons are shown to, and focused by, the first local player. On a dedicated server there is no viewer and Slate is not initialized, so no widget is ever created and the Slate hook is never registered.

**Why the icon data is shared.** Each icon's data is a `TSharedPtr`, held by the icon map and by the visible list of the current pass. An icon unregistered in the middle of a pass is removed from the map at once. The visible list still holds its data safely until the pass ends, and finds its owner gone. The cost is one heap allocation per icon. Storing the data by value with index handles would be more cache friendly, and would need the visible lists to survive swap-removes.

## Interactions

`URCIconInteractionComponent` is an icon the player can interact with for `InteractionTime` seconds within `InteractionRange`.

- `URCInteractorComponent` on the pawn starts the focused interaction on the controlling machine and mirrors it to the server with a reliable RPC (`RPC_C2S_StartInteraction`, `RPC_C2S_CancelInteraction`).
- The interaction component re-checks every running interaction's range each tick. On the server that is the validation: a client that walks away, or claims an interaction it is not near, is cancelled there.
- Started, Completed and Canceled fire only on the machine that decides the interaction (`InteractOnAuthority`, `InteractOnAutonomousProxy`). A pickup, for example, completes on the server, which then adds the item to the player's inventory.
- Interactors are held by weak pointers: an interactor destroyed mid-interaction is cancelled on the next tick instead of leaving a key the garbage collector nulled.
- The component switches its tick off whenever no interaction runs, and back on when one starts.

## The containers

All three are documented in their headers with exactly what invalidates what.

- `FRCDoubleBufferArray<T>`: `Add` writes one buffer, `GetArray` reads the one filled before the last `Flip`. Adding never invalidates a view of the read buffer. A view must not be kept across a `Flip`, which resets the buffer it becomes.
- `FRCMappedDataArray<K, D>` and `FRCMappedArray<K>`: a `TMap` from key to index over a dense `TArray`, for O(1) lookup, add and remove with cache-friendly iteration. Remove swaps the last element into the hole and fixes its index.
  - A key is taken by value on purpose: `RemoveIdx` passes a key stored in the array, and a reference to it would read the swapped element.
  - A raw pointer as the data type is rejected with a `static_assert`: the container is not a `UPROPERTY`, so the garbage collector would not see it.
  - The map and the array are checked against each other in slow-guard builds only, so `Remove` stays O(1) everywhere else.

## Using it

1. Copy `RCCore` and `RCGUI` into a project's `Plugins` folder. RCGUI needs the engine's ModelViewViewModel plugin, which its `.uplugin` enables.
2. Add a `URCIconComponent` (or `URCIconInteractionComponent`) to an actor. Set its widget class, its desired size in the viewport and its visible range.
3. In the widget Blueprint, add `URCIconViewModel` (and `URCIconInteractionViewModel` for interactions) with the creation type Manual, and bind the widget to them.
4. For interactions, add a `URCInteractorComponent` to the player's pawn and call `TryToInteract` and `CancelInteraction` from input. Bind `OnInteractionCompleted` on the interactable.
5. Optionally, set the update budget in `DefaultGame.ini`:

```ini
[/Script/RCGUI.RCIconsSubsystem]
IconUpdatesPerFrame=50
```

## Known limitations

- One viewer: icons follow the first local player only; split screen would need one set of widgets per local player.
- An interactor needs a player controller, so AI cannot use interactions yet.
- The fade speed and the focus radius are constants in `URCIconsSubsystem`, not settings.
- `IconUpdatesPerFrame` is limited to at least 1 in the editor only; a 0 written in the ini would stop icon updates.

## Conventions

- Members and functions are PascalCase, locals and parameters camelCase. Project booleans have no `b` prefix.
- Client-to-server RPCs are named `RPC_C2S_*`.
- Debug logging stays in the code as `/* UE_LOG(...) //*/` blocks, switched on by changing `/*` to `//*`.

---

Copyright Bejan Bogdan Gabriel. All rights reserved. Shared as a code sample.
