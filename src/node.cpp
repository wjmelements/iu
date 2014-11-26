static nid_t me;
nid_t me() {
    return me;
}
void init_node(nid_t nid) {
    me = nid;
    init_server();
}
