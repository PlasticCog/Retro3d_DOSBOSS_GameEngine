# CLAUDE.md - Retro 3D Studio

## Project Overview

Retro 3D Studio (v0.3) is a self-contained, browser-based 3D game creation tool inspired by the Freescape engine and Clickteam Fusion. The entire application lives in a single HTML file (`retro3d-studio.html`) with no build system, no package manager, and no external tooling. It runs directly in any modern web browser.

## Architecture

### Single-File Monolith

The entire application is contained in `retro3d-studio.html` (~1,100 lines):
- **Lines 1-15**: HTML shell and CSS (dark retro theme, monospace fonts)
- **Lines 16-19**: CDN script tags for dependencies
- **Lines 23-1099**: `<script type="text/babel">` containing all application code
- **Line 1098**: React root render call

### Dependencies (CDN-loaded, no npm)

| Library | Version | Purpose |
|---------|---------|---------|
| React | 18.2.0 | UI framework |
| ReactDOM | 18.2.0 | DOM rendering |
| Babel Standalone | 7.23.9 | Client-side JSX compilation |
| Three.js | r128 | 3D graphics engine |

### No Build System

- No `package.json`, no `node_modules`, no webpack/vite/rollup
- No TypeScript, no linting configuration, no formatter
- JSX is compiled at runtime by Babel Standalone
- Open `retro3d-studio.html` directly in a browser to run

## Code Organization

All code is within a single `<script type="text/babel">` block. Sections are separated by comment banners:

### Constants & Configuration (Lines 24-71)
- `C` object: Color palette constants (dark theme colors)
- `baseBtn`: Shared button style object
- `uid()`: Incrementing unique ID generator (`"o1"`, `"o2"`, ...)
- `PRIM_TYPES`: Array of 6 primitive types: `box`, `cylinder`, `cone`, `sphere`, `wedge`, `plane`
- `createPrim(type, name)`: Factory function for new 3D objects
- `CONDITIONS`: Array of 13 event condition definitions with categories and parameter schemas
- `ACTIONS`: Array of 16 action definitions with categories and parameter schemas
- `Ico`: SVG icon components (React.createElement, not JSX)
- `PRIM_ICONS`: Maps primitive types to icon components

### Geometry Helpers (Lines 76-109)
- `makeGeometry(type, customVerts)`: Creates Three.js geometry for each primitive type; applies custom vertex data if present
- `getUniqueVerts(geo)`: Deduplicates vertices for vertex editing (epsilon-based comparison)
- `moveUniqueVert(geo, ui, map, np)`: Moves all buffer entries sharing a unique vertex
- `serializeVerts(geo)`: Flattens geometry position buffer to array for serialization

### AxisGizmo Class (Lines 114-226)
- ES6 class (not a React component) for 3D axis manipulation handles
- Creates X/Y/Z colored arrows with invisible fat hit cylinders for clicking
- Center cube for unconstrained movement
- `show(worldPos)`, `hide()`, `highlight(axisName)`, `updateScale(camera)`, `dispose()`
- Uses `depthTest: false` and high `renderOrder` to render on top

### GameRuntime Component (Lines 231-368)
- Full game execution environment with first-person controls
- WASD movement, mouse-drag look, space to jump, gravity
- Evaluates all event conditions each frame, executes matching actions
- HUD overlay showing variables, FPS, position, debug log
- Click-to-start splash screen with project info

### Viewport3D Component (Lines 373-675)
- Three.js scene management with orbit camera (alt-drag orbit, shift-drag pan, scroll zoom)
- Object selection via raycasting
- Gizmo-based transform: move, rotate, scale along constrained axes
- Vertex editing mode: click vertices, drag along axes
- Syncs React state to Three.js meshes each frame
- Grid and axes helper display

### PropPanel Component (Lines 679-702)
- Property editor for selected object: name, position, rotation, scale, color, visibility
- Shows custom geometry info when vertices have been edited

### ObjList Component (Lines 707-714)
- Scrollable object list with icons, selection highlight, delete button

### EventEditor Component (Lines 719-765)
- Table-based event editor with conditions (yellow) and actions (green) as tags
- Category-grouped picker dropdowns for adding conditions/actions
- Inline parameter editing within tags

### AnimTimeline Component (Lines 770-798)
- Keyframe timeline with frame scrubber
- Per-object keyframe dots, playback at 24fps
- Double-click keyframe to delete

### App Component (Lines 803-1096)
- Root application component managing all state
- State: objects, events, keyframes, selection, transform mode, project name, undo/redo stacks
- Menu bar: File, Edit, View, Insert, Run, Tools, Help
- Tab system: Model Editor, Event Editor, Animation
- Keyboard shortcuts (Ctrl+S save, Ctrl+Z undo, F5 run, Delete, etc.)
- Undo/redo with 30-item history stack (deep JSON clone)
- File I/O: Save/Load `.r3d.json` projects, Export as HTML

## Key Patterns & Conventions

### Coding Style
- **Highly compressed code**: Single-line functions, minimal whitespace, chained expressions
- **Destructured React hooks**: `const {useState, useRef, useEffect, useCallback} = React;`
- **Inline styles everywhere**: No CSS classes, all styling via JavaScript objects
- **`React.createElement` for icons**: SVG icons use createElement directly, not JSX
- **Short variable names**: `C` for colors, `s` for scene state, `gs` for game state, `dr` for drag ref
- **Refs for mutable state**: `useRef` used extensively to avoid stale closures in callbacks (`modeRef`, `objRef`, `selRef`, `camR`, `vtxRef`, `dragRef`)

### State Management
- All state lives in the `App` component via `useState`
- No external state library (no Redux, Zustand, etc.)
- Deep cloning via `JSON.parse(JSON.stringify(...))` for undo/redo and copy/paste
- Three.js scene state synced from React state in `useEffect` hooks

### Data Model
- **Object**: `{id, name, type, position:[x,y,z], rotation:[x,y,z], scale:[x,y,z], color, visible, customVerts}`
- **Event**: `{id, enabled, conditions:[], actions:[]}`
- **Condition**: `{id, type, label, params:{}, negate}`
- **Action**: `{id, type, label, params:{}}`
- **Keyframe**: `{id, objectId, frame, position:[], rotation:[], scale:[]}`
- **Project file** (`.r3d.json`): `{version:1, name, objects, events, keyframes}`

### Three.js Integration
- Orbit camera stored in refs, not React state
- `MeshStandardMaterial` with `flatShading: true` for retro look
- Shadow mapping enabled
- `FogExp2` in runtime, `Fog` in editor
- Raycaster for mouse picking (objects, gizmo handles, vertex handles)

## Development Workflow

### Running
```bash
# Simply open the file in a browser:
open retro3d-studio.html
# or
xdg-open retro3d-studio.html
# or use any local HTTP server:
python3 -m http.server 8000
```

### Testing
- No automated tests exist
- Manual testing by opening in browser and interacting with the editor
- Test the game runtime by clicking "Run Game" (F5)

### File Format
- Project files: `.r3d.json` (JSON with version, objects, events, keyframes)
- Export: Self-contained HTML file (exports the entire editor state as HTML)

## Important Notes for AI Assistants

1. **Single file**: All changes must be made within `retro3d-studio.html`. There is no module system.
2. **No build step**: Changes are immediately effective when the file is reloaded in a browser.
3. **Dense code**: The codebase uses compact formatting. Match existing style when editing.
4. **CDN dependencies**: Do not add npm-based imports. All libraries are loaded via `<script>` tags from CDN.
5. **Babel runtime**: JSX is compiled client-side. Standard JSX syntax works inside the `<script type="text/babel">` block.
6. **Global Three.js**: Three.js is available as the global `THREE` object (not an ES module import).
7. **Global React**: React is available as global `React` and `ReactDOM` objects. Hooks are destructured at the top.
8. **No `.gitignore`**: The repo has no gitignore file. Only `retro3d-studio.html` exists as source.
9. **Rotation units**: Stored in degrees in the data model, converted to radians (`* Math.PI/180`) when applied to Three.js objects.
10. **ID generation**: Uses the `uid()` function which returns `"o1"`, `"o2"`, etc. IDs are not random/UUID.
