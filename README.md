wnet
========

Basic message-passing framework

- nodes are identified by a node id (nid\_t)

- detects definite node failures and calls onNodeFailed(nid\_t), to be defined
by the application

- works with any POSIX.1-2001 compliant system with glibc. Systems without
glibc should only need to implement **poll** or replace the **poll** code with
**select** code, but honestly your servers should be running Linux.

- will support IPv6

Development state: pre-alpha (as of November 2014)
