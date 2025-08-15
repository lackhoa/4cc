//
// NOTE(kv) "NBI" = Notebook Internet
//

typedef u32 IPv4;

// ;about_mask_length Number of leading ones in a bit mask
// (it's easier to work with)

typedef i32 Mask_Length;

struct CIDR
{
 IPv4 address;
 Mask_Length mask_length;
};

struct Net_Node;
struct Route
{
 IPv4 address;  // NOTE(kv) "address" is already anded with the mask
 Mask_Length mask_length;
 Net_Node *next_hop;
};

enum Host_Type
{
 Host_Type_PC,
 Host_Type_Router,
 Host_Type_Server,
 Host_Type_Load_Balancer
};

struct Load_Balancer
{
 darray(IPv4) targets;
};

struct Pod
{
 IPv4 address;
};

struct K8S_Node
{
 darray(Pod) pods;
};

// NOTE(kv) Map from IP address to interface
// (== MAC address, in reality)
typedef sarray(struct Layer_2_Map_Entry) Layer_2_Map;

struct Interface_2
{
 IPv4 address;
 struct Network *network;
};

struct Interface
{
 Net_Node *node;
 Interface_2 *interface;
};

struct Layer_2_Map_Entry
{
 IPv4 address;
 Net_Node *node;  // NOTE(kv) Actually is an interface, but we don't care rn.
};

struct Network
{
 IPv4 address;  // NOTE(kv) Network address
 
 // NOTE(kv) Crappy DHCP
 i32 next_address;
 Net_Node *default_gateway;  // NOTE(kv) Optional
 
 darray(Layer_2_Map_Entry) layer_2_map;
 Mask_Length mask_length;
};

struct Net_Node
{
 Host_Type type;
 
 Stringz name;
 darray(Interface_2) interfaces;
 darray(Route) route_table;
 
 union
 {
  Load_Balancer load_balancer;
  K8S_Node k8s_node;
 };
};

// NOTE(kv) One might say: "Events are overkill",
// but idk, once we get to simulating BGP, and spanning tree,
// it's gonna be useful to process events randomly out of order.
enum NBI_Event_Type
{
 NBI_Event_Type_Message,
 NBI_Event_Type_Send,
};

struct Packet
{
 IPv4 source;
 IPv4 destination;
 String data;
};

// NOTE(kv) Since a packet is gonna be passed around a lot, through several hops.
// Putting it behind a pointer is probably fair?
struct NBI_Event_Send
{// NOTE(kv) Would be nice to be able to hide the sender from the event loop,
 // but probably don't care.
 Net_Node *sender;  
 Net_Node *receiver;
 Packet *packet;
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

function Stringz
ipv4_to_string(Arena *a, IPv4 ip)
{
 i32 cap = 16;
 u8 *buffer = push_size(a, cap);
 Printer printer = make_printer_buffer(buffer, cap);
 printf(printer, "%d.%d.%d.%d",
        (ip >> 24) & 0xFF,
        (ip >> 16) & 0xFF,
        (ip >> 8)  & 0xFF,
        (ip >> 0)  & 0xFF);
 return printer_get_string(printer);
}

function u32
to_bit_mask(Mask_Length mask_length)
{
 u64 zeros_count = 32 - mask_length;
 u32 shifted = u32((u64)1 << zeros_count);
 u32 result = ~(shifted - 1);
 return result;
}

function void
setup_default_route(Net_Node *node)
{
 if(node->type != Host_Type_Router)
 {
  b32 has_default_route = 0;
  for_i32(ri, 0, node->route_table.count)
  {
   Route route = node->route_table[ri];
   if(route.address == 0 and
      route.mask_length == 0)
   {
    has_default_route = 1;
    break;
   }
  }
  
  if(not has_default_route)
  {// NOTE(kv) We have to find the interface with
   // a default gateway configured.
   for_i32(ii, 0, node->interfaces.count)
   {
    Interface_2 *interface = &node->interfaces[ii];
    Network *network = interface->network;
    if(network->default_gateway)
    {
     Route route = {};
     route.address     = 0;
     route.mask_length = 0;
     route.next_hop = network->default_gateway;
     push(&node->route_table, route);
     
     break;
    }
   }
  }
 }
}

// NOTE(kv) I guess returning only the node is fine,
// since we don't care which interface handles the packet.
function Net_Node *
lookup_route(Net_Node *node, IPv4 key)
{
 Net_Node *result = 0;
 
 setup_default_route(node);
 
 // NOTE(kv) Local address?
 for_i32(ii, 0, node->interfaces.count)
 {
  Interface_2 interface = node->interfaces[ii];
  Network *network = interface.network;
  u32 mask = to_bit_mask(network->mask_length);
  if((key & mask) == network->address)
  {// NOTE(kv) The key address is local to this interface
   // Then sending this packet is a layer 2 problem.
   for_i32(mi, 0, network->layer_2_map.count)
   {
    Layer_2_Map_Entry entry = network->layer_2_map[mi];
    if(entry.address == key)
    {
     result = entry.node;
     goto end_local;
    }
   }
  }
 }
 end_local:
 
 if(result == 0)
 {// NOTE(kv): Route table bookmark
  for_i32(ri, 0, node->route_table.count)
  {
   Route route = node->route_table[ri];
   u32 mask = to_bit_mask(route.mask_length);
   if(route.address == (key & mask))
   {// NOTE(kv) Let's pick this route
    // TODO(kv) We need to sort routes by mask length,
    // if we wanna match real behavior.
    result = route.next_hop;
    goto end_route_table;
   }
  }
 }
 end_route_table:
 
 return result;
}

function void
send_packet_to_address(NBI_Event_State *e,
                       Net_Node *sender, IPv4 receiver, Packet *packet)
{
 Net_Node *receiver_host = lookup_route(sender, receiver);
 if(packet->source == 0)
 {// NOTE(kv) Packet doesn't have a source, this means
  // we're not doing a packet relay.
  // TODO(kv) The route table needs an outgoing interface
 }
 
 if(receiver_host)
 {
  NBI_Event event0 = {};
  event0.type = NBI_Event_Type_Send;
  
  NBI_Event_Send *send = push_struct(e->arena, NBI_Event_Send);
  send->sender = sender;
  send->receiver = receiver_host;
  
  send->packet = packet;
  
  event0.data = (void *)send;
  
  push(&e->event_queue, event0);
 }
 else
 {
  NBI_Event event0 = {};
  event0.type = NBI_Event_Type_Message;
  NBI_Event_Message *msg = push_struct(e->arena, NBI_Event_Message);
  msg->message = push_stringf(e->arena, "Failed to send from %S to %S",
                              sender->name, ipv4_to_string(e->arena, receiver));
  event0.data = msg;
  push(&e->event_queue, event0);
 }
}

function IPv4
get_default_address(Net_Node *node)
{
 if(node->interfaces.count > 0)
 {
  return node->interfaces[0].address;
 }
 
 return 0;
}

function Interface_2 *
get_interface_2_tmp(Net_Node *host)
{// TODO(kv) remove me
 return &host->interfaces[0];
}

function Interface
get_interface_tmp(Net_Node *node)
{// TODO(kv) remove me
 return Interface{.node = node, .interface = &node->interfaces[0]};
}

function void
send_message_to_address(NBI_Event_State *e,
                        Net_Node *sender, IPv4 receiver, String message)
{
 Packet *packet = push_struct0(e->arena, Packet);
 packet->source = get_interface_2_tmp(sender)->address;
 packet->destination = receiver;
 packet->data = message;
 
 send_packet_to_address(e, sender, receiver, packet);
}

function void
nbi_update_world(NBI_Event_State *e)
{// NOTE(kv) The event queue will get more items as we process it.
 for(i32 event_index = 0;
     event_index < e->event_queue.count;
     event_index++)
 {
  NBI_Event &event0 = e->event_queue[event_index];
  switch(event0.type)
  {
   case NBI_Event_Type_Send:
   {
    NBI_Event_Send *event = (NBI_Event_Send *)event0.data;
    Net_Node *receiver = event->receiver;
    Packet *packet = event->packet;
    
    switch(receiver->type)
    {
     case Host_Type_Router:
     {
      if(packet->destination != get_interface_2_tmp(receiver)->address)
      {// NOTE(kv) Relay the packet
       send_packet_to_address(e, receiver,
                              packet->destination, packet);
      }
     }break;
     
     case Host_Type_Load_Balancer:
     {
      // NOTE(kv) Relay the packet to a target in its target group
      // TODO(kv) Actually implement the load balancing logic.
      sarray(IPv4) targets = receiver->load_balancer.targets;
      if(targets.count > 0)
      {
       IPv4 target = targets[0];
       send_packet_to_address(e, receiver, target, packet);
      }
     }break;
    } // switch receiver type
   } break;
  } // switch event type
 }
}

function Net_Node *
mk_node(Arena *a, Stringz name, Host_Type type)
{
 Net_Node *node = push_struct0(a, Net_Node);
 node->type = type;
 node->name = name;
 
 init_dynamic(node->interfaces, a);
 init_dynamic(node->route_table, a);
 
 return node;
}

// NOTE(kv) Return the address
function IPv4
add_node_to_network(Network *network, Net_Node *node)
{
 Interface_2 *iface = push_zero(&node->interfaces);
 iface->address = network->next_address++;  // TODO(kv) Yeah we need better things here
 iface->network = network;
 
 push(&network->layer_2_map, Layer_2_Map_Entry{
       .address = iface->address,
       .node    = node,
      });
 
 return iface->address;
}

function void
add_default_gateway(Network *network, Net_Node *gateway)
{
 add_node_to_network(network, gateway);
 network->default_gateway = gateway;
}

struct IPv4_Range
{
 IPv4 begin;
 IPv4 end;
};

struct Subnetter
{
 // ;does_not_contain_empty_range
 darray(IPv4_Range) ranges;
};

function IPv4
allocate_address(Subnetter *s)
{
 IPv4 result = 0;  // NOTE(kv) IP addresses are never all zeros
 
 if(s->ranges.count > 0)
 {
  IPv4_Range *range = &s->ranges[0];
  // @does_not_contain_empty_range
  kv_assert(range->end - range->begin > 0);
  result = range->begin;
  range->begin += 1;
  
  if(range->begin == range->end)
  {
   remove_at_fast(&s->ranges, 0);
  }
 }
 
 return result;
}

function Network
mk_network(Arena *a, IPv4 address, Mask_Length mask_length)
{
 Network network = {};
 init_dynamic(network.layer_2_map, a);
 network.address     = address;
 network.mask_length = mask_length;
 network.next_address = address+1;
 return network;
}

// -------- Examples -------- 

function void
example_router(Arena *tmp, NBI_Event_State *e)
{
 Network home_net = mk_network(tmp, mk_ipv4(192,168,0,0), 24);
 Network server_net = mk_network(tmp, mk_ipv4(10,0,0,0), 24);
 
 Net_Node *my_pc = mk_node(tmp, strlit("my_pc"), Host_Type_PC);
 add_node_to_network(&home_net, my_pc);
 
 Net_Node *server = mk_node(tmp, strlit("server"), Host_Type_PC);
 add_node_to_network(&server_net, server);
 
 Net_Node *router = mk_node(tmp, strlit("router"), Host_Type_Router);
 add_default_gateway(&home_net, router);
 add_default_gateway(&server_net, router);
 
 // NOTE(kv) Send!
 send_message_to_address(e, my_pc, get_default_address(server),
                         strlit("Alice"));
}

function void
example_load_balancing(Arena *tmp, NBI_Event_State *e)
{
 Network network = mk_network(tmp, mk_ipv4(192,168,0,0), 24);
 Net_Node *my_pc = mk_node(tmp, strlit("my_pc"), Host_Type_PC);
 add_node_to_network(&network, my_pc);
 
 Net_Node *load_balancer = mk_node(tmp, strlit("load_balancer"), Host_Type_Load_Balancer);
 add_node_to_network(&network, load_balancer);
 
 Net_Node *server_0 = mk_node(tmp, strlit("server_0"), Host_Type_PC);
 add_node_to_network(&network, server_0);
 //
 Net_Node *server_1 = mk_node(tmp, strlit("server_1"), Host_Type_PC);
 add_node_to_network(&network, server_1);
 
 {// NOTE(kv) Load balancer targets
  darray(IPv4) *targets = &load_balancer->load_balancer.targets;
  init_dynamic(*targets, tmp);
  push(targets, get_interface_2_tmp(server_0)->address);
  push(targets, get_interface_2_tmp(server_1)->address);
 }
 
 send_message_to_address(e, my_pc, get_interface_2_tmp(load_balancer)->address,
                         strlit("I want Conan"));
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
 
 example_router(tmp, e);
 
 nbi_update_world(e);
 
 {// NOTE(kv) Print out the event queue
  ImGui::Begin("nb_internet");
  print_event_queue(e->event_queue);
  ImGui::End();
 }
 
 if(0)
 {// NOTE(kv) Test subnetter
  Subnetter subnetter = {};
  init_dynamic(subnetter.ranges, tmp);
  IPv4_Range range = {
   mk_ipv4(192,168,0,0),
   mk_ipv4(192,168,0,255) + 1,
  };
  push(&subnetter.ranges, range);
  
  IPv4 address_0 = allocate_address(&subnetter);
  IPv4 address_1 = allocate_address(&subnetter);
  Stringz address_1_string = ipv4_to_string(tmp, address_1);
  breakhere;
 }
}