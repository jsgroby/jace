# Accepted experiment order (2026-11-23)

Reference: main ad0a3c571901f65eee9bcf20c994ab8ad5c166ab, the confirmed 21-weight
evaluation from PR #35. Rejected king-pressure PR #36 is not part of the engine.

1. Knight and bishop placement (current experiment). Fit 30 regional offsets,
   keeping all accepted weights fixed. Preserve the old tables at zero offsets;
   compile offsets into production table entries. Broaden training/validation
   with existing opponent games; hold out the entire latest king-safety match.
   Check exact implementation, fit, test held-out prediction, then play a fresh
   paired screening match if justified. Confirm promising match results before
   recommending promotion.
2. Passed-pawn advancement bonuses. Start from the accepted result of step 1
   (or retain the current baseline if rejected). Tune a small set of rank/phase
   weights, preserving doubled-pawn treatment and king-proximity behavior. Use
   fresh held-out data and a separate match; do not combine with placement.
3. Search diagnostics based on missed tactics. Review losses and build a
   reproducible position set with full move histories. Compare equal-time and
   equal-node searches; obtain independent tactical reference analysis where
   available. Choose one search change based on that evidence. Do not label
   speculative alternatives as proven best moves or assume king exposure alone
   explains the losses.

Each stage is an experiment. Lower prediction loss is not an Elo gain. Preserve
all rejected results and do not keep retrying against the same held-out sample.
No baseline promotion or release publication is implied by starting a match.
