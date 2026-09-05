# Settings remote-control feedback QA

> Historical visual QA for the 2026-08-07 build. Screenshot paths and pass results describe that local run; they are not a verification of the current working tree. Some artifacts are local/ignored and may not exist in a fresh clone.

## Source visual truth

- Selected direction: `C:\Users\mu\Desktop\code\demo\artifacts\design-reference\settings-controlled-service-v2\selected-option-2.png`.
- Normalized source: `C:\Users\mu\Desktop\code\demo\artifacts\design-audit\2026-08-07-controlled-service-help\29-qa-source-app-normalized.png`, 1404 × 920 px.
- Latest user requirements override the mock where they conflict: Project Help must not repeat the address, the verification card stays in Remote Settings, and troubleshooting must be split into controller/controlled-device responsibilities.

## Rendered implementation

- Overview: `C:\Users\mu\Desktop\code\demo\artifacts\design-audit\2026-08-07-controlled-service-help\31-feedback-overview-expanded.jpeg`.
- Remote Settings, passive-control top: `C:\Users\mu\Desktop\code\demo\artifacts\design-audit\2026-08-07-controlled-service-help\32-feedback-remote-top-expanded.jpeg`.
- Remote Settings, verification and active-control section: `C:\Users\mu\Desktop\code\demo\artifacts\design-audit\2026-08-07-controlled-service-help\33-feedback-remote-bottom-expanded.jpeg`.
- Project Help, expanded: `C:\Users\mu\Desktop\code\demo\artifacts\design-audit\2026-08-07-controlled-service-help\34-feedback-help-expanded.jpeg`.
- Project Help, compact: `C:\Users\mu\Desktop\code\demo\artifacts\design-audit\2026-08-07-controlled-service-help\36-feedback-help-compact-final.jpeg`.
- Normalized implementation: `C:\Users\mu\Desktop\code\demo\artifacts\design-audit\2026-08-07-controlled-service-help\37-qa-feedback-implementation-normalized.png`, 1404 × 920 px.
- Expanded app viewport: 2223 × 1447 px at `(526, 76)` in a 3120 × 2080 capture. Compact launch viewport: 1200 × 1000 px.
- State: controlled service running; permissions granted; Remote Settings and Project Help scrolled through their complete relevant content.

## Findings

- No actionable P0/P1/P2 visual differences remain.
- Overview: the current-state card now uses the shared hover/press elevation, translation, scale, shadow, and surface transition pattern.
- Remote Settings: service, screen permission, and input permission are grouped under `被动控制`. The shared-directory and verification cards are grouped under `主动控制`, with verification below the shared directory. The address is display-only without copy feedback.
- Project Help: the duplicate address block is gone. Service status, connection steps, role-labelled troubleshooting, and the controller-direction entry remain legible and visually ordered.
- Motion: service status and all connection-step cards use the same hover/press transition language as other settings cards. Static captures show the resting states; ArkTS policy tests verify the hover handlers and shared animation calls.
- Typography and spacing: headings, labels, step copy, and role-specific troubleshooting wrap cleanly at both tested sizes.
- Colors and icons: existing app tokens and media resources are retained; no placeholder or approximate asset was introduced.
- Responsiveness: Expanded keeps a three-column step row; Compact stacks the cards. No clipping, overlap, inaccessible action, or broken scroll region was observed.

## Comparison evidence

- The normalized source and latest expanded implementation were opened together in one comparison input.
- The implementation preserves the selected direction's navigation, title hierarchy, status-first structure, steps, and secondary connection direction.
- Intentional differences follow the latest user requirements: no address in Project Help, no introductory troubleshooting sentence, explicit `主控端`/`被控端` guidance, and clearer passive/active control grouping in Remote Settings.

## Comparison history

- Pass 1: implemented the selected second direction and established Expanded/Compact behavior.
- Pass 2: corrected the overview card motion, restored the verification card, clarified passive/active control, removed the help address, animated help cards, and rewrote troubleshooting by role.
- Pass 3: removed address-copy UI and moved verification below the shared-directory card in active control.
- Pass 2 result: no P0/P1/P2 issue found in the final captures.

## Verification gaps

- Pointer-hover state could not be captured through the connected device test driver; implementation and policy tests verify the same proven shared motion pattern used elsewhere in Settings.
- No physical tablet was connected. Tablet isolation remains covered by policy tests and multi-device App Pack verification; a physical-tablet screenshot is still desirable when hardware is available.

final result: passed
