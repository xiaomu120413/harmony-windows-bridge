# Settings controlled-service redesign QA

**Source visual truth**

- `C:\Users\mu\Desktop\code\demo\artifacts\design-reference\settings-controlled-service-v2\selected-option-2.png`
- Source pixels: 1487 × 1058.
- Normalized app crop: `C:\Users\mu\Desktop\code\demo\artifacts\design-audit\2026-08-07-controlled-service-help\29-qa-source-app-normalized.png`, 1404 × 920.

**Rendered implementation**

- Expanded help: `C:\Users\mu\Desktop\code\demo\artifacts\design-audit\2026-08-07-controlled-service-help\22-final-copy-state-expanded.jpeg`
- Compact help and copy-success state: `C:\Users\mu\Desktop\code\demo\artifacts\design-audit\2026-08-07-controlled-service-help\28-final-help-copy-compact.jpeg`
- Expanded screenshot pixels: 3120 × 2080; app window viewport: 2223 × 1447 px at `(526, 76)`.
- Compact app launch viewport: 1200 × 1000 px inside the same 3120 × 2080 display capture.
- Normalized implementation crop: `C:\Users\mu\Desktop\code\demo\artifacts\design-audit\2026-08-07-controlled-service-help\30-qa-implementation-app-normalized.png`, 1404 × 920.
- Density normalization: source app crop was kept at 1404 × 920; the 2223 × 1447 implementation app crop was bicubic-resampled to 1404 × 920. Desktop chrome outside the app was excluded.
- State: Project Help, controlled service running on port 3390, permissions granted, connection address available.

**Findings**

- No actionable P0/P1/P2 differences remain.
- Fonts and typography: HarmonyOS system typography preserves the source hierarchy; headings, connection address, step titles, supporting text, wrapping, and optical weight remain readable at both Expanded and Compact sizes.
- Spacing and layout rhythm: the left navigation, content margins, primary connection card, three-step row, troubleshooting block, and secondary direction entry follow the selected design. Compact mode intentionally stacks the card controls and steps without clipping.
- Colors and visual tokens: existing product tokens retain the white/light-gray surfaces, blue action color, green running state, purple help selection, borders, and radii from the source direction.
- Image and icon quality: all visible icons use existing app media resources; no emoji, placeholder, CSS art, custom SVG approximation, or degraded raster asset was introduced.
- Copy and content: the implementation intentionally removes the source mock's controlled-side verification code because the product rule is that the code is set by the controller. The help text states this explicitly and keeps the address, port, service state, permissions, and troubleshooting guidance coherent.
- Interaction and accessibility: settings navigation, remote-settings entry, help direction entry, scroll behavior, and connection-address copy were exercised. The copy button reached the visible `已复制` state. Controls retain native focus/tap behavior and practical target sizes.
- Responsiveness: Expanded uses a three-column step layout; Compact uses vertical cards. No overlap, clipping, hidden primary action, or broken text wrapping was observed in the final captures.

**Full-view comparison evidence**

- The normalized 1404 × 920 source and implementation were opened together in one comparison input.
- The implementation keeps the same information hierarchy and above-the-fold task flow. Differences are intentional product corrections: no controlled-side verification code, an explicit troubleshooting block, and existing product icon/token usage.

**Focused region comparison evidence**

- A separate focused crop was not required because the normalized full-app captures keep the connection address, copy action, status, step copy, icons, and typography legible at 1404 × 920.
- The Compact copy-success capture separately verifies the most important interaction state and stacked layout.

**Open Questions**

- A real tablet was not connected during this pass. Tablet isolation is covered by ArkTS policy tests and App Pack module/package verification, but a fresh tablet screenshot remains a device-availability test gap.

**Comparison History**

- Pass 1: no P0/P1/P2 visual issue was found after normalization. The apparent verification-code difference was classified as an intentional correction required by the user, not design drift. No visual fix iteration was necessary.

**Implementation Checklist**

- [x] Remove local network information from Basic Settings.
- [x] Put connection address, service state, permissions, and copy action in controlled-device surfaces.
- [x] Remove controlled-side verification-code UI and explain controller ownership.
- [x] Gate controlled-service help to the 2in1 capability.
- [x] Verify Expanded and Compact layouts plus copy-success state.
- [x] Build and verify the multi-device App Pack.

**Follow-up Polish**

- P3: capture the tablet-only help screen on real tablet hardware when one is available, to complement the package and policy evidence.

final result: passed
