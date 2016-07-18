#include "common.h"


/**
 * Accumulator structure.
 *   @str: The string pointer.
 *   @len: The length.
 *   @buf: The buffer.
 */
struct accum_t {
	char **str;
	size_t *len;
	struct strbuf_t buf;
};


/*
 * local declarations
 */
static size_t accum_write(void *ref, const void *data, size_t nbytes);
static size_t accum_read(void *ref, void *data, size_t nbytes);
static void accum_close(void *ref);

static size_t str_write(void *ref, const void *buf, size_t nbytes);
static size_t str_read(void *ref, void *buf, size_t nbytes);


/**
 * Write to a length.
 *   @ref: The length reference.
 *   @buf: The buffer.
 *   @nbytes: The number of bytes to write.
 *   &returns: The number of bytes write.
 */
static size_t len_write(void *ref, const void *data, size_t nbytes)
{
	*(size_t *)ref += nbytes;

	return nbytes;
}

/**
 * Read from a length.
 *   @ref: The length reference.
 *   @buf: The buffer.
 *   @nbytes: The number of bytes to read.
 *   &returns: Always zero, cannot read from length.
 */
static size_t len_read(void *ref, void *data, size_t nbytes)
{
	return 0;
}

/**
 * Close a length.
 *   @ref: The length reference.
 */
static void len_close(void *ref)
{
}

/**
 * Create a length file.
 *   @str: Ref. The destination string pointer.
 *   &returns: The file.
 */
struct io_file_t io_file_len(size_t *len)
{
	static const struct io_file_i iface = { len_read, len_write, len_close };

	return (struct io_file_t){ len, &iface };
}


/**
 * Create a string accumulator file.
 *   @str: Ref. The destination string pointer.
 *   @len: Optional. The length.
 *   &returns: The file.
 */
struct io_file_t io_file_accum(char **str, size_t *len)
{
	struct accum_t *accum;
	static const struct io_file_i iface = { accum_read, accum_write, accum_close };

	accum = malloc(sizeof(struct accum_t));
	accum->str = str;
	accum->buf = strbuf_init(256);
	accum->len = len;

	return (struct io_file_t){ accum, &iface };
}
static size_t accum_write(void *ref, const void *data, size_t nbytes)
{
	struct accum_t *accum = ref;

	strbuf_addmem(&accum->buf, data, nbytes);
	if(accum->len != NULL)
		*accum->len += nbytes;

	return nbytes;
}
static size_t accum_read(void *ref, void *data, size_t nbytes)
{
	return 0;
}
static void accum_close(void *ref)
{
	struct accum_t *accum = ref;

	*accum->str = strbuf_done(&accum->buf);
	free(accum);
}


/**
 * Write to file.
 *   @ref: The file reference.
 *   @buf: The buffer.
 *   @nbytes: The number of bytes to write.
 *   &returns: The number of bytes written.
 */
static size_t file_write(void *ref, const void *buf, size_t nbytes)
{
	return fwrite(buf, 1, nbytes, ref);
}

/**
 * Read from file.
 *   @ref: The file reference.
 *   @buf: The buffer.
 *   @nbytes: The number of bytes to read.
 *   &returns: The number of bytes read.
 */
static size_t file_read(void *ref, void *buf, size_t nbytes)
{
	return fread(buf, 1, nbytes, ref);
}

/**
 * Close file.
 *   @ref: The file reference.
 */
static void file_close(void *ref)
{
}
/**
 * Create a descriptor file.
 *   @fd: The file descriptor.
 *   &returns: The file.
 */
struct io_file_t io_file_wrap(FILE *file)
{
	static const struct io_file_i iface = { file_read, file_write, file_close };

	return (struct io_file_t){ file, &iface };
}


/**
 * Write to file descriptor.
 *   @ref: The file descriptor reference.
 *   @buf: The buffer.
 *   @nbytes: The number of bytes to write.
 *   &returns: The number of bytes written.
 */
static size_t fd_write(void *ref, const void *buf, size_t nbytes)
{
	return write((int)(intptr_t)ref, buf, nbytes);
}

/**
 * Read from file descriptor.
 *   @ref: The file descriptor reference.
 *   @buf: The buffer.
 *   @nbytes: The number of bytes to read.
 *   &returns: The number of bytes read.
 */
static size_t fd_read(void *ref, void *buf, size_t nbytes)
{
	return read((int)(intptr_t)ref, buf, nbytes);
}

/**
 * Close file descriptor.
 *   @ref: The file descriptor reference.
 */
static void fd_close(void *ref)
{
}
/**
 * Create a descriptor file.
 *   @fd: The file descriptor.
 *   &returns: The file.
 */
struct io_file_t io_file_fd(int fd)
{
	static const struct io_file_i iface = { fd_read, fd_write, fd_close };

	return (struct io_file_t){ (void *)(intptr_t)fd, &iface };
}


/**
 * Create an input file from a string.
 *   @str: The string.
 *   &returns: The file.
 */
struct io_file_t io_file_str(const char *str)
{
	const char **ptr;
	static const struct io_file_i iface = { str_read, str_write, free };

	ptr = malloc(sizeof(const char *));
	*ptr = str;

	return (struct io_file_t){ (void *)ptr, &iface };
}
static size_t str_write(void *ref, const void *buf, size_t nbytes)
{
	return 0;
}
static size_t str_read(void *ref, void *buf, size_t nbytes)
{
	size_t i;
	char **ptr = ref, *dest = buf;

	for(i = 0; i < nbytes; i++) {
		if(**ptr == '\0')
			break;

		*dest = **ptr;
		(*ptr)++;
	}

	return i;
}
