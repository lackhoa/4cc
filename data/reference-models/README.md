# Reference models

## `z-anatomy-head-*.obj` (skull, mandible, teeth-upper, eyeball, facial_muscles, skin_head)

Source: Z-Anatomy (https://github.com/Z-Anatomy/Models-of-human-anatomy), whose human
model derives from BodyParts3D (Database Center for Life Science, Japan,
http://dbarchive.biosciencedbc.jp/en/bodyparts3d/desc.html).

What that means for measurements taken on these meshes (as of 2026-09-26):

- **One adult male**, a 22-year-old Japanese volunteer. Every number on the tablet
  `skull-*` pages is one data point from one skull, not a population average.
- **MRI-derived, not CT.** Bone was segmented from MRI, so fine bony edges (gonion tip,
  alveolar margin, nasal spine) are smoother than on a dry skull. Landmarks are good to
  a millimeter or two, not better.
- Young, fully erupted dentition; the mouth is closed in the scan (lower incisor tip
  +0.6 mm above the upper tip).

`*.landmarks.txt` files next to the meshes hold hand-picked or confirmed landmarks in
mesh coordinates (picked on the tablet pages, see `tablet/src/reference_skull_view.ts`).

## `female-skull-hangar79-10k.obj`

A second skull, female, decimated to 10k faces. Not yet used by the tablet pages; the
natural second data point for the `skull-*` measurements.

## `skull.obj`

TODO provenance unknown; older reference, predates the Z-Anatomy meshes.
