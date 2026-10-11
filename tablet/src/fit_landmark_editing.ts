// The landmark editing shared by the landmark pages (loomis-girl-fit-landmarks,
// fit-hairline-to-loomis-man): the file as a whole is the unit of undo, with a selected
// landmark that can be renamed or removed, "add landmark" that waits for a template point
// to be picked (on the head model pane), and reset to the starting file. Written against
// what both files have, `landmarks` with a `name` and a `template_point` each; the page
// decides what a new landmark's marks are and what to do after an edit (save, draw).
//
// The bottom bar's elements are found by fixed ids (one editor per page):
// add_landmark_button, landmark_name_field, remove_landmark_button,
// undo_landmark_edit_button, reset_landmarks_button.
import { TemplatePoint } from "./loomis_girl_plate";

export type EditableLandmark = { name: string; template_point: TemplatePoint };
export type EditableLandmarksFile<L extends EditableLandmark> = { landmarks: L[] };

export type FitLandmarkEditorListeners<F extends EditableLandmarksFile<EditableLandmark>> = {
  starting_file: () => F;
  // The landmark "add" makes for a picked template point, null when it can't (the point
  // has no position in the loaded document). Its name is given by the editor.
  new_landmark: (template_point: TemplatePoint, name: string) => F["landmarks"][number] | null;
  // After the file changed (an edit, an undo, a reset): save it, redraw.
  on_edited: () => void;
  // After the selection or the "adding" state changed without the file changing: redraw.
  on_view_changed: () => void;
};

export type FitLandmarkEditor<F extends EditableLandmarksFile<EditableLandmark>> = {
  file: F; // replaced as a whole by undo and reset: read it through the editor, don't keep a copy
  selected_landmark_index: number | null;
  is_adding_landmark: boolean; // "add landmark" is armed: the next template point picked becomes one
  // Call before changing `file`, then `landmarks_edited` after.
  remember_for_undo: () => void;
  landmarks_edited: () => void;
  undo_landmark_edit: () => void;
  select_landmark: (landmark_index: number | null) => void;
  add_landmark_at: (template_point: TemplatePoint) => void;
  remove_selected_landmark: () => void;
  rename_selected_landmark: (name: string) => void;
  reset_landmarks: () => void;
  // Enables / labels the bottom bar's buttons and name field for the current state. The
  // page calls it after its own state changes too (it is cheap).
  update_bottom_bar: () => void;
};

export function create_fit_landmark_editor<F extends EditableLandmarksFile<EditableLandmark>>(file: F, listeners: FitLandmarkEditorListeners<F>): FitLandmarkEditor<F> {
  const undo_stack: F[] = []; // the file as it was before each edit, oldest first
  const add_landmark_button = document.getElementById("add_landmark_button") as HTMLButtonElement;
  const landmark_name_field = document.getElementById("landmark_name_field") as HTMLInputElement;
  const remove_landmark_button = document.getElementById("remove_landmark_button") as HTMLButtonElement;
  const undo_button = document.getElementById("undo_landmark_edit_button") as HTMLButtonElement;
  const reset_button = document.getElementById("reset_landmarks_button") as HTMLButtonElement;

  const editor: FitLandmarkEditor<F> = {
    file,
    selected_landmark_index: null,
    is_adding_landmark: false,

    remember_for_undo: () => {
      undo_stack.push(structuredClone(editor.file));
    },

    landmarks_edited: () => {
      editor.update_bottom_bar();
      listeners.on_edited();
    },

    undo_landmark_edit: () => {
      const before = undo_stack.pop();
      if (before === undefined) return;
      editor.file = before;
      if (editor.selected_landmark_index !== null && editor.selected_landmark_index >= editor.file.landmarks.length) editor.selected_landmark_index = null;
      editor.landmarks_edited();
    },

    select_landmark: (landmark_index) => {
      editor.selected_landmark_index = landmark_index;
      editor.update_bottom_bar();
      listeners.on_view_changed();
    },

    // The new landmark is "landmark N" for the first free N, and becomes the selection.
    add_landmark_at: (template_point) => {
      const names = new Set(editor.file.landmarks.map((landmark) => landmark.name));
      let number = editor.file.landmarks.length + 1;
      while (names.has(`landmark ${number}`)) number++;
      const landmark = listeners.new_landmark(template_point, `landmark ${number}`);
      if (landmark === null) return;
      editor.remember_for_undo();
      editor.file.landmarks.push(landmark);
      editor.selected_landmark_index = editor.file.landmarks.length - 1;
      editor.is_adding_landmark = false;
      editor.landmarks_edited();
    },

    remove_selected_landmark: () => {
      if (editor.selected_landmark_index === null) return;
      editor.remember_for_undo();
      editor.file.landmarks.splice(editor.selected_landmark_index, 1);
      editor.selected_landmark_index = null;
      editor.landmarks_edited();
    },

    rename_selected_landmark: (name) => {
      const trimmed = name.trim();
      if (editor.selected_landmark_index === null || trimmed === "" || trimmed === editor.file.landmarks[editor.selected_landmark_index].name) {
        editor.update_bottom_bar();
        return;
      }
      editor.remember_for_undo();
      editor.file.landmarks[editor.selected_landmark_index].name = trimmed;
      editor.landmarks_edited();
    },

    reset_landmarks: () => {
      editor.remember_for_undo();
      editor.file = listeners.starting_file();
      editor.selected_landmark_index = null;
      editor.landmarks_edited();
    },

    update_bottom_bar: () => {
      const selected = editor.selected_landmark_index === null ? null : editor.file.landmarks[editor.selected_landmark_index];
      if (document.activeElement !== landmark_name_field) landmark_name_field.value = selected === null ? "" : selected.name;
      landmark_name_field.disabled = selected === null;
      remove_landmark_button.disabled = selected === null;
      undo_button.disabled = undo_stack.length === 0;
      add_landmark_button.classList.toggle("armed", editor.is_adding_landmark);
      add_landmark_button.textContent = editor.is_adding_landmark ? "tap a point on the head model…" : "add landmark";
    },
  };

  const toggle_adding = (is_adding: boolean): void => {
    if (editor.is_adding_landmark === is_adding) return;
    editor.is_adding_landmark = is_adding;
    editor.update_bottom_bar();
    listeners.on_view_changed();
  };

  add_landmark_button.addEventListener("click", () => toggle_adding(!editor.is_adding_landmark));
  remove_landmark_button.addEventListener("click", editor.remove_selected_landmark);
  undo_button.addEventListener("click", editor.undo_landmark_edit);
  reset_button.addEventListener("click", editor.reset_landmarks);
  landmark_name_field.addEventListener("change", () => editor.rename_selected_landmark(landmark_name_field.value));
  landmark_name_field.addEventListener("keydown", (e) => {
    if (e.key === "Enter" || e.key === "Escape") landmark_name_field.blur();
  });

  // Ctrl+Z undoes, Delete removes the selected landmark, Escape disarms "add landmark".
  window.addEventListener("keydown", (e) => {
    if (document.activeElement === landmark_name_field) return; // the keys belong to the name being typed
    if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "z") {
      e.preventDefault();
      editor.undo_landmark_edit();
    } else if (e.key === "Delete") {
      e.preventDefault();
      editor.remove_selected_landmark();
    } else if (e.key === "Escape") {
      toggle_adding(false);
    }
  });

  editor.update_bottom_bar();
  return editor;
}
