// Snap distances the user tunes on the page `snap-tuning` (pages/snap-tuning/). The values
// live in localStorage, so they are per browser, shared by every sketchpad page of that
// browser; an open sketchpad tab picks a change up at once (`storage` event), no reload.
// SNAP_TUNING_FIELDS is the single list: the type, the defaults and the page's sliders
// all come from it.

export type SnapTuningUnit = "world" | "pixels";
export type SnapTuningField = {
  key: string;
  label: string;
  description: string;
  unit: SnapTuningUnit;
  default_value: number;
  min: number;
  max: number;
  step: number;
};

export const SNAP_TUNING_FIELDS = [
  {
    key: "vertex_weld_radius_world",
    label: "vertex to vertex (weld)",
    description: "A dragged line end welds into another vertex when released closer than this to it, in 3D. 0 = never weld by dragging.",
    unit: "world", default_value: 0.05, min: 0, max: 0.1, step: 0.002,
  },
  {
    key: "vertex_pin_radius_world",
    label: "vertex to line (pin)",
    description: "A dragged line end is pinned onto another line when released closer than this to its curve, in 3D. 0 = never pin by dragging.",
    unit: "world", default_value: 0.05, min: 0, max: 0.1, step: 0.002,
  },
  {
    key: "point_drag_start_pixels",
    label: "tap or drag of a point",
    description: "The pen landed on a point: moving less than this is still a tap (never welds or pins), more is a drag (welds or pins on release).",
    unit: "pixels", default_value: 4, min: 1, max: 30, step: 1,
  },
] as const satisfies readonly SnapTuningField[];

export type SnapTuning = { [Key in (typeof SNAP_TUNING_FIELDS)[number]["key"]]: number };

const SNAP_TUNING_STORAGE_KEY = "autodraw_tablet_snap_tuning";

export function default_snap_tuning(): SnapTuning {
  return Object.fromEntries(SNAP_TUNING_FIELDS.map((field) => [field.key, field.default_value])) as SnapTuning;
}

// Defaults, overlaid with whatever valid numbers are stored. Node checks have no
// localStorage and run on the defaults.
function read_stored_snap_tuning(): SnapTuning {
  const values = default_snap_tuning();
  if (typeof localStorage === "undefined") return values;
  let stored: unknown = null;
  try {
    stored = JSON.parse(localStorage.getItem(SNAP_TUNING_STORAGE_KEY) ?? "null");
  } catch (error) {
    console.error("snap tuning: stored values unreadable, using defaults", error);
  }
  if (typeof stored !== "object" || stored === null) return values;
  for (const field of SNAP_TUNING_FIELDS) {
    const value = (stored as Record<string, unknown>)[field.key];
    if (typeof value === "number" && Number.isFinite(value) && value >= 0) values[field.key] = value;
  }
  return values;
}

let current_snap_tuning = read_stored_snap_tuning();
if (typeof window !== "undefined") {
  window.addEventListener("storage", (event) => {
    if (event.key === SNAP_TUNING_STORAGE_KEY) current_snap_tuning = read_stored_snap_tuning();
  });
}

export function snap_tuning(): SnapTuning {
  return current_snap_tuning;
}

export function store_snap_tuning(values: SnapTuning): void {
  current_snap_tuning = { ...values };
  try {
    localStorage.setItem(SNAP_TUNING_STORAGE_KEY, JSON.stringify(current_snap_tuning));
  } catch (error) {
    console.error("snap tuning: could not store the values", error);
  }
}
