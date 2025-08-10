//
// NOTE(kv) "NBI" = Notebook Internet
//

typedef u32 IPv4;

// ;about_mask_length Number of leading ones in a bit mask
// (it's easier to work with)

struct CIDR
{
 IPv4 address;
 i32 mask_length; // @about_mask_length
};

struct Host;

struct Route
{
 IPv4 address;  // NOTE(kv) "address" is already anded with the mask
 i32 mask_length;  // @about_mask_length
 Host *next_hop;
};

struct Host
{
 Stringz name;
 CIDR cidr;
 darray(Route) route_table;
};

enum NBI_Event_Type
{
 NBI_Event_Type_Message,
 NBI_Event_Type_Send,
};

struct NBI_Packet
{
 IPv4 source;
 IPv4 destination;
 String data;
};

// NOTE(kv) Since a packet is gonna be passed around a lot, through several hops.
// Putting it behind a pointer is probably fair?
struct NBI_Event_Send
{
 Host *sender;
 Host *receiver;
 NBI_Packet *packet;
};

struct NBI_Event_Message
{
 Stringz message;
};

struct NBI_Event
{
 NBI_Event_Type type;
 void *data;
};

struct NBI_Event_State
{
 Arena *arena;
 darray(NBI_Event) event_queue;
};

function Stringz
event_to_string(Arena *arena, NBI_Event &event0)
{
 Stringz result = strlit("Unknown event");
 switch(event0.type)
 {
  case NBI_Event_Type_Message:
  {
   NBI_Event_Message *event = (NBI_Event_Message *)event0.data;
   result = event->message;
  }break;
  
  case NBI_Event_Type_Send:
  {
   NBI_Event_Send *event = (NBI_Event_Send *)event0.data;
   result = push_stringf(arena, "'%S' sends '%S' to '%S'",
                         event->sender->name,
                         event->packet->data,
                         event->receiver->name);
  }break;
 }
 
 return result;
}

// NOTE(kv) Print to ImGui window
function void
print_event_queue(sarray(NBI_Event) events)
{
 Scratch_Block tmp;
 
 for_i32(i, 0, events.count)
 {
  arena_clear(tmp);
  NBI_Event &event = events[i];
  Stringz string = event_to_string(tmp, event);
  ImGui::TextUnformatted(to_cstring(string));
 }
}

function void
nbi_update_world(NBI_Event_State *e)
{
 
}

function IPv4
mk_ipv4(u8 a, u8 b, u8 c, u8 d)
{
 return (a << 24) | (b << 16) | (c << 8) | (d << 0);
}

function CIDR
mk_cidr(u8 a, u8 b, u8 c, u8 d,   u32 mask)
{
 CIDR result;
 result.address = mk_ipv4(a,b,c,d);
 result.mask_length = mask;
 return result;
}

function u32
to_bit_mask(i32 mask_length)
{// NOTE(kv) See @about_mask_length
 u64 zeros_count = 32 - mask_length;
 u32 shifted = u32((u64)1 << zeros_count);
 u32 result = ~(shifted - 1);
 return result;
}

function Host *
lookup_route(sarray(Route) table, IPv4 key)
{
 for_i32(ri, 0, table.count)
 {
  Route &route = table[ri];
  u32 masked_key = key & to_bit_mask(route.mask_length);
  if(masked_key == route.address)
  {
   return route.next_hop;
  }
 }
 
 return 0;
}

function void
send_message_to_address(NBI_Event_State *e,
                        Host *sender, IPv4 receiver, String message)
{
 Host *receiver_host = lookup_route(sender->route_table, receiver);
 
 if(receiver_host)
 {
  NBI_Event event0 = {};
  event0.type = NBI_Event_Type_Send;
  
  NBI_Event_Send *send = push_struct(e->arena, NBI_Event_Send);
  send->sender = sender;
  send->receiver = receiver_host;
  
  NBI_Packet *packet = push_struct(e->arena, NBI_Packet);
  packet->source = sender->cidr.address;
  packet->destination = mk_ipv4(192,168,0,3);
  packet->data = message;
  send->packet = packet;
  
  event0.data = (void *)send;
  
  push(&e->event_queue, event0);
 }
 else
 {
  NBI_Event event0 = {};
  event0.type = NBI_Event_Type_Message;
  NBI_Event_Message *msg = push_struct(e->arena, NBI_Event_Message);
  msg->message = strlit("Failed to send");
  event0.data = msg;
  push(&e->event_queue, event0);
 }
}

function void
nb_internet_main()
{
 Scratch_Block tmp;
 
 NBI_Event_State event_state = {};
 NBI_Event_State *e = &event_state;
 // NOTE(kv) Initialize event state
 e->arena = tmp;
 init_dynamic(e->event_queue, e->arena, 128);
 
 Host *my_computer = push_struct(tmp, Host);
 my_computer->name = strlit("my_computer");
 my_computer->cidr = mk_cidr(192,168,0,2,   24);
 
 Host *my_router = push_struct(tmp, Host);
 my_router->name = strlit("my_router");
 my_router->cidr = mk_cidr(192,168,0,1,   24);
 
 {
  darray(Route) table;
  init_dynamic(table, tmp);
  
  // NOTE(kv) Default gateway
  Route route = {};
  route.address = mk_ipv4(0,0,0,0);
  route.mask_length = 0;
  route.next_hop = my_router;
  push(&table, route);
  
  my_computer->route_table = table;
 }
 
 {// NOTE(kv) Sending an event
  send_message_to_address(e, my_computer, my_router->cidr.address,
                          strlit("Alice"));
 }
 
 {// NOTE(kv) Print out the event queue
  ImGui::Begin("nb_internet");
  print_event_queue(e->event_queue);
  ImGui::End();
 }
}
//