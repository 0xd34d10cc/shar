// see https://tools.ietf.org/html/rfc8445#section-7.2.5.3

// Request priority:
// STUN binding - get agent's public IP
// TURN allocate - allocate IP:port pair  on TURN server for agent

// Gather candidates for both agents
// Perofrm connectivity checks on pairs (agent-local candidate, peer candidate)
// Connectivity check is a STUN binding request (with extension fields) sent from local candidate to remote candidate
// > which means that our application have to act like a server in some cases

// see https://networkengineering.stackexchange.com/questions/47276/how-udp-hole-punching-works
