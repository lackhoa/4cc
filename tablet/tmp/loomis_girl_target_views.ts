// The Loomis school girl plate for the node scripts in tmp/: the target views and
// landmarks from src/, plus where the plate image is on this machine.
import os from "node:os";
import path from "node:path";

export * from "../src/loomis_girl_target_views";

export const LOOMIS_GIRL_IMAGE_PATH = path.join(os.homedir(), "Downloads/autodraw-loomis-girl/loomis-girl-front-and-profile.webp");
