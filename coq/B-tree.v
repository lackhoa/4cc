Require Import Coq.Lists.List.
Import ListNotations.

Inductive tree : Type :=
| Nil
| T (keys : list nat)
    (children : list tree).

(*NOTE(kv): This is not yet a B-tree *)
Inductive valid_tree : tree -> Prop :=
| B_tree_nil : valid_tree Nil
| B_Tree_T : forall keys children,
  length keys > 0 ->
  length children = 1 + length keys ->
  Forall valid_tree children ->
  valid_tree (T keys children).

Record diagram_entry := mk_diagram_entry {
  key : nat;
  height : nat;
}.

(*NOTE(kv) keys and children are assumed to have equal number of elements*)
Fixpoint interleave {A : Type} (keys : list A) (children : list (list A)) : list A :=
  match keys, children with
  | [], _ => []
  | _, [] => []
  | x :: xs, y :: ys => (x :: y) ++ (interleave xs ys)
  end.

(*bookmark(kv) We need to recurse down,
figure out the height,
and put it in the keys*)
Fixpoint tree_to_diagram (t : tree)
: (list diagram_entry) * height
:=
match t with
| Nil => pair [] 0
| T keys children =>
  let recurse : list (list diagram_entry) :=
    (map tree_to_diagram children) in
  
  let keys_2 : list diagram_entry :=
    map (fun key => mk_diagram_entry key height) keys in
  
  interleave keys_2 recurse
end.
















(*END*)