# Project Development Rules

This document is the mandatory engineering constitution for every future task.

Every implementation, bug fix, optimization, refactor, or new feature must comply with these rules.

We should write the code in the simplest and most concise way possible.

## 1. Legacy Parity

Legacy behavior is the highest priority.

Never replace Legacy behavior with assumptions.

If Legacy behavior is unknown:

- investigate
- audit
- compare
- document

before writing code.

Never invent behavior.

## 2. Source Audit First

Before implementing any feature:

- search existing source
- search Dev_Log
- search previous implementations
- search shared core
- search existing utilities

Reuse existing systems whenever possible.

Avoid duplicate implementations.

## 3. Single Source of Truth

Never duplicate rendering, map loading, animation, SPK decoding, coordinate conversion, packet handling, or resource systems.

Client and tools must use the same shared implementation.

## 4. Performance First

Performance regressions are functional bugs.

Do not implement features that create unnecessary runtime cost.

Every expensive operation must have:

- owner
- cache
- lifetime
- invalidation condition

## 5. No Per-Frame Expensive Work

Never perform every frame:

- allocations
- vector rebuilding
- texture uploads
- SPK decoding
- frame-pack parsing
- minimap generation
- draw-list rebuilding
- map traversal
- archive scanning

unless absolutely required.

## 6. Cache Rules

Everything expensive must be cached.

Examples:

- decoded sprites
- textures
- frame metadata
- minimap
- draw lists
- object lookup
- tile lookup
- animation metadata

Every cache must document:

- creation
- owner
- lifetime
- invalidation trigger

## 7. Invalidation Rules

Never invalidate caches without a real reason.

Camera movement must not rebuild:

- minimap
- draw lists
- textures
- decoded sprites

Zoom should only invalidate viewport calculations.

## 8. Memory

Avoid:

- unnecessary heap allocations
- temporary vectors
- repeated string creation
- repeated file reads

Reuse memory whenever possible.

## 9. GPU

Never upload identical textures twice.

Never recreate GPU resources every frame.

Reuse:

- SRVs
- textures
- buffers

until resources actually change.

## 10. Map Editor

Viewport rendering must be independent from GUI.

UI interaction must never rebuild world data.

The minimap must use its own cached rendering path.

## 11. Debug

Debug builds may include:

- diagnostics
- profiling
- timing
- assertions
- validation

Release builds must not contain:

- profiling
- diagnostics
- debug logging
- audit instrumentation

## 12. Before Writing Code

Always ask:

Can this reuse an existing system?

Can this reuse an existing cache?

Can this avoid allocations?

Can this avoid rebuilding data?

Can this avoid decoding again?

## 13. Before Marking Complete

Before considering work complete, verify:

- runtime behavior
- cache behavior
- invalidation behavior
- allocation behavior
- GPU upload behavior
- Legacy parity
- build status

Compilation alone is not sufficient.

## 14. Documentation

Every new subsystem must document:

- purpose
- owner
- dependencies
- cache lifetime
- performance considerations
- known limitations

## 15. Dev Log

Before every task, read Dev_Log.

After every completed task, update Dev_Log.

Never repeat previously solved problems.

## 16. Never Do These

Never:

- invent Legacy behavior
- hardcode temporary fixes
- bypass validation
- silently ignore failures
- rebuild everything because it is easier
- duplicate code
- implement parallel versions of existing systems
- claim completion without verification

## 17. Code Quality

Prefer:

- small functions
- clear ownership
- deterministic behavior
- const correctness
- RAII
- stable interfaces
- minimal coupling

## 18. Project Goal

The objective is not simply to make the code compile.

The objective is to build a stable, performant, maintainable, and fully Legacy-compatible client and toolchain.

Every implementation should improve long-term maintainability rather than only solving the immediate problem.

## 19. Root Cause First

Never fix symptoms.

Always identify and fix the root cause.

Temporary workarounds, fallbacks, silent retries, forced refreshes, and brute-force rebuilds are prohibited unless explicitly approved.

Every bug fix must explain:

- why the bug occurred
- why the chosen fix is correct
- why it does not introduce regressions
- why alternative fixes were rejected

This document is mandatory for all future work. Read and follow it before every implementation.
