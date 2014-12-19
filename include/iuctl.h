enum ctl_t {
    LEARN_PORT,
    STATUS,
};
void init_iuctl();
void join_iuctl();
void destroy_iuctl();

// called by only iuctl
void status_iuctl();

// called by only service
void handle_iuctls();
