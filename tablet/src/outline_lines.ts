// The picture view's outline as lines (plan-contour-thick-stub-on-patch-border): the
// contour chains found inside the patches (contour.ts) and the drawn runs of the strokes
// between patches (boundary_contour.ts) are listed first, then joined end to end into
// lines, then drawn line by line.
//
// Only a chain's end and a run's end join, and only where they are the same place: the
// same stroke, the same side (a midline stroke has one), and the same curve parameter t
// (BorderPlace, both computed by boundary_contour.ts from the same numbers, compared with
// ===). And only where nothing else ends there: every piece end with a place is counted,
// ends of one stroke side closer than STROKE_T_EPSILON in t (where the runs' cuts make a
// part empty) count as one place, and a place joins only with exactly two ends, one of a
// chain and one of a run. Every other end is an end of its line: a chain end inside its
// patch, on a loft's open edge or next to a collapsed corner, an end at a stroke's end (a
// vertex), an end where three or more meet, and the ends of strokes drawn whole (those
// are drawn by the callers and never listed here).
//
// A line is capped round only at its two ends; where two of its pieces join, neither gets a
// cap, and the two ribbons meet end to end. A join where the line turns back on screen (a
// cusp, outline_join_is_cusp) is not made: there the line ends, and both pieces are capped.

import { OrbitCamera, camera_basis, camera_eye } from "./camera";
import { PlacedContourChain, SurfacePartials } from "./contour";
import { ContourWidthBlend, append_contour_chain_ribbon_with_blend, contour_chain_knot_widths } from "./contour_width";
import { Stroke, TabletDocument } from "./document";
import { SurfaceGrid } from "./patch";
import {
  BorderContours, BorderPlace, BorderRun, STROKE_T_EPSILON,
  append_border_run_ribbon, border_place_of_chain_end, border_run_end_direction, border_run_end_place, border_run_sample_widths,
} from "./boundary_contour";
import { RibbonEndCaps, RibbonEndDirection, RibbonSampleWidths, cubic_end_direction } from "./ribbon";
import { Rgb, VertexSink } from "./vertex_sink";

// A contour chain found inside one surface (its grid and that grid's partials), and how it
// is drawn: with the surface's width blend, in `color`, lightened unless `has_lightness`
// is false.
export type OutlineChain = {
  placed: PlacedContourChain;
  partials: SurfacePartials;
  grid: SurfaceGrid;
  blend: ContourWidthBlend;
  has_lightness: boolean;
  color: Rgb;
};

// Every piece of one frame's outline. A piece is named by its index: the chains first,
// then the runs (chain_count + run index).
export type OutlinePieces = { chains: OutlineChain[]; runs: BorderRun[] };

// One piece of a line, gone through from its end to its start when `is_reversed`.
export type OutlineLinePiece = { piece: number; is_reversed: boolean };

// Pieces joined end to end; `is_closed` when the last one's end joins the first one's start.
export type OutlineLine = { pieces: OutlineLinePiece[]; is_closed: boolean };

export function create_outline_pieces(): OutlinePieces {
  return { chains: [], runs: [] };
}

// Lists the chains of one surface.
export function append_outline_chains(
  pieces: OutlinePieces, placed_chains: PlacedContourChain[], partials: SurfacePartials, grid: SurfaceGrid,
  blend: ContourWidthBlend, has_lightness: boolean, color: Rgb,
): void {
  for (const placed of placed_chains) pieces.chains.push({ placed, partials, grid, blend, has_lightness, color });
}

// The place of every piece end, piece p's start at 2p and its end at 2p + 1 (null: no place).
function piece_end_places(pieces: OutlinePieces, border_contours: BorderContours, camera: OrbitCamera): (BorderPlace | null)[] {
  const eye = camera_eye(camera);
  const places: (BorderPlace | null)[] = [];
  for (const chain of pieces.chains) {
    const knot_places = chain.placed.knot_places;
    places.push(border_place_of_chain_end(border_contours, chain.partials, chain.grid, knot_places[0], eye));
    places.push(border_place_of_chain_end(border_contours, chain.partials, chain.grid, knot_places[knot_places.length - 1], eye));
  }
  for (const run of pieces.runs) {
    places.push(border_run_end_place(run, false));
    places.push(border_run_end_place(run, true));
  }
  return places;
}

// For every piece end (2p, 2p + 1), the piece end it joins, or -1.
export function joined_piece_ends(pieces: OutlinePieces, border_contours: BorderContours, camera: OrbitCamera): Int32Array {
  const places = piece_end_places(pieces, border_contours, camera);
  const first_run_end = 2 * pieces.chains.length;
  const joined = new Int32Array(places.length).fill(-1);
  // The ends with a place, by stroke and side.
  const ends_by_stroke_side = new Map<Stroke, number[][]>();
  places.forEach((place, end) => {
    if (place === null) return;
    let by_side = ends_by_stroke_side.get(place.stroke);
    if (by_side === undefined) {
      by_side = [[], []];
      ends_by_stroke_side.set(place.stroke, by_side);
    }
    by_side[place.is_mirrored_side ? 1 : 0].push(end);
  });
  const t_of = (end: number): number => places[end]!.t;
  for (const by_side of ends_by_stroke_side.values()) {
    for (const ends of by_side) {
      ends.sort((a, b) => t_of(a) - t_of(b));
      let group_start = 0;
      while (group_start < ends.length) {
        let group_end = group_start + 1;
        while (group_end < ends.length && t_of(ends[group_end]) - t_of(ends[group_end - 1]) < STROKE_T_EPSILON) group_end++;
        if (group_end - group_start === 2) {
          const end_a = ends[group_start];
          const end_b = ends[group_start + 1];
          const is_chain_and_run = (end_a < first_run_end) !== (end_b < first_run_end);
          if (is_chain_and_run && t_of(end_a) === t_of(end_b)) {
            joined[end_a] = end_b;
            joined[end_b] = end_a;
          }
        }
        group_start = group_end;
      }
    }
  }
  return joined;
}

// Piece end `end` (2p: piece p's start, 2p + 1: its end) and the way its ribbon leaves it.
function piece_end_direction(pieces: OutlinePieces, end: number, tablet_document: TabletDocument): RibbonEndDirection {
  const piece = end >> 1;
  const at_end = (end & 1) === 1;
  const chain_count = pieces.chains.length;
  if (piece < chain_count) {
    const chain = pieces.chains[piece].placed.chain;
    return cubic_end_direction(at_end ? chain[chain.length - 1] : chain[0], at_end);
  }
  return border_run_end_direction(pieces.runs[piece - chain_count], tablet_document, at_end);
}

// The way a line leaving `end.point` in `end.direction` goes on screen (any length): the
// derivative of the camera's projection there, without the 1 / depth² both share.
function screen_direction_of(end: RibbonEndDirection, camera: OrbitCamera): { x: number; y: number } {
  const { right, up, forward } = camera_basis(camera);
  const direction = end.direction;
  const direction_right = direction.x * right.x + direction.y * right.y + direction.z * right.z;
  const direction_up = direction.x * up.x + direction.y * up.y + direction.z * up.z;
  if (camera.is_orthographic === true) return { x: direction_right, y: direction_up };
  const eye = camera_eye(camera);
  const from_eye = { x: end.point.x - eye.x, y: end.point.y - eye.y, z: end.point.z - eye.z };
  const depth = from_eye.x * forward.x + from_eye.y * forward.y + from_eye.z * forward.z;
  const direction_depth = direction.x * forward.x + direction.y * forward.y + direction.z * forward.z;
  const from_eye_right = from_eye.x * right.x + from_eye.y * right.y + from_eye.z * right.z;
  const from_eye_up = from_eye.x * up.x + from_eye.y * up.y + from_eye.z * up.z;
  return {
    x: direction_right * depth - from_eye_right * direction_depth,
    y: direction_up * depth - from_eye_up * direction_depth,
  };
}

// Whether the line turns back on screen where piece ends `end_a` and `end_b` join (a cusp):
// the two ribbons leave the join in screen directions less than 30 degrees apart, so the
// line turns by more than 150 degrees there. A ribbon with no screen direction at the join
// (it leaves it straight toward the camera) counts as a cusp: the join is not made.
// NOTE: from the camera, so a line breaks or joins anew as the view turns.
export function outline_join_is_cusp(
  pieces: OutlinePieces, end_a: number, end_b: number, tablet_document: TabletDocument, camera: OrbitCamera,
): boolean {
  const max_turn_degrees = 150;
  const screen_a = screen_direction_of(piece_end_direction(pieces, end_a, tablet_document), camera);
  const screen_b = screen_direction_of(piece_end_direction(pieces, end_b, tablet_document), camera);
  const length_a = Math.hypot(screen_a.x, screen_a.y);
  const length_b = Math.hypot(screen_b.x, screen_b.y);
  if (length_a === 0 || length_b === 0) return true;
  const cosine_between = (screen_a.x * screen_b.x + screen_a.y * screen_b.y) / (length_a * length_b);
  // The turn is 180 degrees less the angle between the two leaving directions.
  return cosine_between > Math.cos(((180 - max_turn_degrees) * Math.PI) / 180);
}

// The pieces joined into lines: first every line with two ends, from the piece whose start
// or end is free, then the closed ones. A join at a cusp is not made.
export function outline_lines_of(
  pieces: OutlinePieces, border_contours: BorderContours, tablet_document: TabletDocument, camera: OrbitCamera,
): OutlineLine[] {
  const joined = joined_piece_ends(pieces, border_contours, camera);
  for (let end = 0; end < joined.length; end++) {
    const other_end = joined[end];
    if (other_end < end) continue;
    if (!outline_join_is_cusp(pieces, end, other_end, tablet_document, camera)) continue;
    joined[end] = -1;
    joined[other_end] = -1;
  }
  const piece_count = pieces.chains.length + pieces.runs.length;
  const is_in_line = new Uint8Array(piece_count);
  const lines: OutlineLine[] = [];
  const follow_line = (first_piece: number, is_first_reversed: boolean): void => {
    const line: OutlineLine = { pieces: [], is_closed: false };
    let piece = first_piece;
    let is_reversed = is_first_reversed;
    for (;;) {
      is_in_line[piece] = 1;
      line.pieces.push({ piece, is_reversed });
      const leaving_end = is_reversed ? 2 * piece : 2 * piece + 1;
      const entered_end = joined[leaving_end];
      if (entered_end === -1) break;
      const next_piece = entered_end >> 1;
      if (is_in_line[next_piece] === 1) {
        line.is_closed = true;
        break;
      }
      piece = next_piece;
      // Entered at its end: gone through backwards.
      is_reversed = (entered_end & 1) === 1;
    }
    lines.push(line);
  };
  for (let piece = 0; piece < piece_count; piece++) {
    if (is_in_line[piece] === 1) continue;
    if (joined[2 * piece] === -1) follow_line(piece, false);
    else if (joined[2 * piece + 1] === -1) follow_line(piece, true);
  }
  for (let piece = 0; piece < piece_count; piece++) {
    if (is_in_line[piece] === 0) follow_line(piece, false);
  }
  return lines;
}

// How the half-width runs along a line. Each rule takes the widths a line's knots are drawn
// with today (stroke width values, as in Stroke.radii: a chain's knots, a run's ribbon
// samples) and the knots' world length along the line, and gives the widths to draw:
// - as_today: the same widths (the picture as before);
// - even: one width for the whole line, the median of its widths weighted by length;
// - slow_swell: the widths smoothed along the line over a length (gaussian, by length).
export type OutlineWidthRule = "as_today" | "even" | "slow_swell";

// The rule every outline is drawn with; the page's ?outline_width_rule= sets it
// (set_outline_width_rule_from_url).
let outline_width_rule: OutlineWidthRule = "as_today";

// NOTE: slow_swell's smoothing length (the gaussian's sigma), world units (a grid cell is 1).
// About the median line length on head-corrected (0.14-0.16; quarters 0.07 and 0.3, longest
// 2.3), so a short line comes out nearly even and a long one still swells along its length.
const SLOW_SWELL_LENGTH = 0.15;

export function set_outline_width_rule(rule: OutlineWidthRule): void {
  outline_width_rule = rule;
}

// Reads ?outline_width_rule=as_today|even|slow_swell from the page's address (browser only).
// Anything else leaves as_today.
export function set_outline_width_rule_from_url(): void {
  const asked = new URLSearchParams(window.location.search).get("outline_width_rule");
  if (asked === null) return;
  if (asked === "as_today" || asked === "even" || asked === "slow_swell") {
    outline_width_rule = asked;
    return;
  }
  console.warn(`outline_width_rule=${asked} is not a rule (as_today, even, slow_swell): drawing as_today`);
}

// The widths `rule` gives a line whose knots have `knot_widths` at `knot_arc_lengths`
// (world length from the line's start, never decreasing). Each knot stands for half the
// length to each of its neighbours. A line of no length keeps its widths.
export function outline_widths_by_rule(
  rule: OutlineWidthRule, knot_widths: Float64Array, knot_arc_lengths: Float64Array, slow_swell_length: number,
): Float64Array {
  const knot_count = knot_widths.length;
  const widths = Float64Array.from(knot_widths);
  if (rule === "as_today") return widths;
  const knot_lengths = new Float64Array(knot_count);
  for (let knot = 0; knot < knot_count; knot++) {
    const before = knot_arc_lengths[Math.max(knot - 1, 0)];
    const after = knot_arc_lengths[Math.min(knot + 1, knot_count - 1)];
    knot_lengths[knot] = (after - before) / 2;
  }
  const total_length = knot_arc_lengths[knot_count - 1] - knot_arc_lengths[0];
  if (total_length === 0) return widths;
  if (rule === "even") {
    const by_width = Array.from({ length: knot_count }, (_, knot) => knot).sort((a, b) => knot_widths[a] - knot_widths[b]);
    let covered = 0;
    let median = knot_widths[by_width[knot_count - 1]];
    for (const knot of by_width) {
      covered += knot_lengths[knot];
      if (covered < total_length / 2) continue;
      median = knot_widths[knot];
      break;
    }
    widths.fill(median);
    return widths;
  }
  // slow_swell: each knot's width is the gaussian-weighted mean of the knots within three
  // sigmas of it along the line.
  const reach = 3 * slow_swell_length;
  let window_start = 0;
  let window_end = 0;
  for (let knot = 0; knot < knot_count; knot++) {
    const at = knot_arc_lengths[knot];
    while (knot_arc_lengths[window_start] < at - reach) window_start++;
    while (window_end < knot_count && knot_arc_lengths[window_end] <= at + reach) window_end++;
    let weighted_sum = 0;
    let weight_sum = 0;
    for (let other = window_start; other < window_end; other++) {
      const distance = knot_arc_lengths[other] - at;
      const weight = knot_lengths[other] * Math.exp(-(distance * distance) / (2 * slow_swell_length * slow_swell_length));
      weighted_sum += weight * knot_widths[other];
      weight_sum += weight;
    }
    if (weight_sum > 0) widths[knot] = weighted_sum / weight_sum;
  }
  return widths;
}

// The piece's knots as one line has them: a chain's knots (lengths along its cubics'
// chords, the cubics being short cell segments), a run's ribbon samples.
function piece_knot_widths(pieces: OutlinePieces, piece: number, tablet_document: TabletDocument): RibbonSampleWidths {
  const chain_count = pieces.chains.length;
  if (piece >= chain_count) return border_run_sample_widths(pieces.runs[piece - chain_count], tablet_document);
  const chain = pieces.chains[piece];
  const cubics = chain.placed.chain;
  const arc_lengths = new Float64Array(cubics.length + 1);
  cubics.forEach((cubic, index) => {
    arc_lengths[index + 1] = arc_lengths[index] + Math.hypot(cubic.p3.x - cubic.p0.x, cubic.p3.y - cubic.p0.y, cubic.p3.z - cubic.p0.z);
  });
  return { widths: contour_chain_knot_widths(chain.placed, chain.blend), arc_lengths };
}

// The widths `rule` gives each piece of the line, per knot in the piece's own direction.
// The pieces' knots are put end to end in the line's order; the two knots at a join (a
// chain's end and a run's, a small distance apart) are both kept, with no length between
// them. A closed line is smoothed as if open at its first piece's start.
function line_piece_widths(
  pieces: OutlinePieces, line: OutlineLine, tablet_document: TabletDocument, rule: OutlineWidthRule,
): Float64Array[] {
  const piece_knots = line.pieces.map((line_piece) => piece_knot_widths(pieces, line_piece.piece, tablet_document));
  const knot_count = piece_knots.reduce((sum, knots) => sum + knots.widths.length, 0);
  const line_widths = new Float64Array(knot_count);
  const line_arc_lengths = new Float64Array(knot_count);
  let first_knot = 0;
  let line_length = 0;
  line.pieces.forEach((line_piece, index) => {
    const { widths, arc_lengths } = piece_knots[index];
    const last = widths.length - 1;
    for (let knot = 0; knot <= last; knot++) {
      const own_knot = line_piece.is_reversed ? last - knot : knot;
      const along_piece = line_piece.is_reversed ? arc_lengths[last] - arc_lengths[own_knot] : arc_lengths[own_knot];
      // A width below 0 is drawn as 0 (the stroke has faded out there), so the rule sees 0.
      line_widths[first_knot + knot] = Math.max(0, widths[own_knot]);
      line_arc_lengths[first_knot + knot] = line_length + along_piece;
    }
    first_knot += widths.length;
    line_length += arc_lengths[last];
  });
  const drawn_widths = outline_widths_by_rule(rule, line_widths, line_arc_lengths, SLOW_SWELL_LENGTH);
  first_knot = 0;
  return line.pieces.map((line_piece, index) => {
    const own_count = piece_knots[index].widths.length;
    const piece_widths = drawn_widths.slice(first_knot, first_knot + own_count);
    first_knot += own_count;
    if (line_piece.is_reversed) piece_widths.reverse();
    return piece_widths;
  });
}

// Draws every line, each of its pieces as one ribbon in the piece's own direction, capped
// only at the line's two ends (a closed line nowhere), with the widths outline_width_rule
// gives it.
export function append_outline_ribbons(
  pieces: OutlinePieces, lines: OutlineLine[], tablet_document: TabletDocument, camera: OrbitCamera, out: VertexSink,
): void {
  const chain_count = pieces.chains.length;
  const rule = outline_width_rule;
  for (const line of lines) {
    const last_index = line.pieces.length - 1;
    // as_today draws with each piece's own widths, as before there were rules.
    const piece_widths = rule === "as_today" ? null : line_piece_widths(pieces, line, tablet_document, rule);
    line.pieces.forEach((line_piece, index) => {
      const given_widths = piece_widths === null ? null : piece_widths[index];
      // Where the line enters this piece and where it leaves it.
      const is_entered_capped = index === 0 && !line.is_closed;
      const is_left_capped = index === last_index && !line.is_closed;
      const end_caps: RibbonEndCaps = line_piece.is_reversed
        ? { at_start: is_left_capped, at_end: is_entered_capped }
        : { at_start: is_entered_capped, at_end: is_left_capped };
      if (line_piece.piece < chain_count) {
        const chain = pieces.chains[line_piece.piece];
        append_contour_chain_ribbon_with_blend(chain.placed, chain.blend, chain.has_lightness, camera, chain.color, out, end_caps, given_widths);
        return;
      }
      append_border_run_ribbon(pieces.runs[line_piece.piece - chain_count], tablet_document, camera, out, end_caps, given_widths);
    });
  }
}
