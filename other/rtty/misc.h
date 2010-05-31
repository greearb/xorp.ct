void	cat_v(FILE *, const u_char *, int);
int	install_ttyios(int, const struct termios *);
void	prepare_term(struct termios *, cc_t);
int	tty_nonblock(int, int);

void	*safe_malloc(size_t);
void	*safe_calloc(size_t, size_t);
void	*safe_realloc(void *, size_t);
char	*safe_strdup(const char *);
