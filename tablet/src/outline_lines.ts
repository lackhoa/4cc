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

import { OrbitCamera, camera_eye } from "./camera";
import { PlacedContourChain, SurfacePartials } from "./contour";
import { ContourWidthBlend, append_contour_chain_ribbon_with_blend } from "./contour_width";
import { Stroke, TabletDocument } from "./document";
import { SurfaceGrid } from "./patch";
import {
  BorderContours, BorderPlace, BorderRun, STROKE_T_EPSILON, append_border_run_ribbon, border_place_of_chain_end, border_run_end_place,
} from "./boundary_contour";
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

// The pieces joined into lines: first every line with two ends, from the piece whose start
// or end is free, then the closed ones.
export function outline_lines_of(pieces: OutlinePieces, border_contours: BorderContours, camera: OrbitCamera): OutlineLine[] {
  const joined = joined_piece_ends(pieces, border_contours, camera);
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

// Draws every line, each of its pieces as one ribbon in the piece's own direction.
export function append_outline_ribbons(
  pieces: OutlinePieces, lines: OutlineLine[], tablet_document: TabletDocument, camera: OrbitCamera, out: VertexSink,
): void {
  const chain_count = pieces.chains.length;
  for (const line of lines) {
    for (const line_piece of line.pieces) {
      if (line_piece.piece < chain_count) {
        const chain = pieces.chains[line_piece.piece];
        append_contour_chain_ribbon_with_blend(chain.placed, chain.blend, chain.has_lightness, camera, chain.color, out);
        continue;
      }
      append_border_run_ribbon(pieces.runs[line_piece.piece - chain_count], tablet_document, camera, out);
    }
  }
}
