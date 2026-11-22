// The "Drawing" list of the front page (index.html): after its hand-written entries, one
// `<li>` per drawing page of src/drawing_pages.ts, then every document on the server that
// no drawing page opens, each linking to `/draw/?document=<name>`. When the server cannot
// list documents, the list shows the drawing pages only.
import { DRAWING_PAGES, DrawingPage, document_link_url, drawing_page_url } from "./drawing_pages";
import { list_documents_from_server } from "./persistence";

// `<a><strong>name</strong></a>`, the look of every entry of the front page.
function make_name_link(name: string, url: string): HTMLAnchorElement {
  const link = document.createElement("a");
  link.href = url;
  const strong = document.createElement("strong");
  strong.textContent = name;
  link.appendChild(strong);
  return link;
}

function make_drawing_page_item(page: DrawingPage): HTMLLIElement {
  const item = document.createElement("li");
  item.appendChild(make_name_link(page.page_name, drawing_page_url(page)));
  const description = document.createElement("span");
  description.innerHTML = ` — ${page.description_html}`; // trusted: written in src/drawing_pages.ts
  item.appendChild(description);
  return item;
}

// Server names go in as text, never as HTML.
function make_document_item(document_name: string): HTMLLIElement {
  const item = document.createElement("li");
  item.appendChild(make_name_link(document_name, document_link_url(document_name)));
  item.append(" — document without a drawing page of its own.");
  return item;
}

async function fill_drawing_list(list: HTMLUListElement): Promise<void> {
  for (const page of DRAWING_PAGES) list.appendChild(make_drawing_page_item(page));
  const entries = await list_documents_from_server();
  if (entries === null) return;
  const document_names_with_a_page = new Set(DRAWING_PAGES.map((page) => page.document_name));
  // Documents a page writes over as a whole: not for drawing in, so not listed.
  // `head-corrected-trial` is rewritten by the button of fit-head-to-native-head.
  const overwritten_document_names = new Set(["head-corrected-trial"]);
  const document_names = entries
    .map((entry) => entry.name)
    .filter((name) => !document_names_with_a_page.has(name) && !overwritten_document_names.has(name))
    .sort();
  for (const name of document_names) list.appendChild(make_document_item(name));
}

void fill_drawing_list(document.getElementById("drawing_list") as HTMLUListElement);
