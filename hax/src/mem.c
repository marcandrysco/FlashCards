#include "common.h"

/*
 * global variables
 */
ssize_t hax_memcnt = 0;

/*
 * local variables
 */
#if DEBUG
static sys_mutex_t lock;
#endif


/**
 * Allocate memory.
 *   @nbytes: The number of bytes.
 *   &returns: The allocated memory.
 */
void *hax_malloc(size_t nbytes)
{
#undef malloc
	void *ptr;

	ptr = malloc(nbytes ?: 1);
	if(ptr == NULL)
		fatal("Memory allocation failed, %s.", strerror(errno));

#if DEBUG
	sys_mutex_lock(&lock);
	hax_memcnt++;
	sys_mutex_unlock(&lock);
#endif

	return ptr;
}

/**
 * Free memory.
 *   @ptr: The pointer.
 */
void hax_free(void *ptr)
{
#undef free
	if(ptr == NULL)
		fatal("Attempted to free null pointer,");

#if DEBUG
	sys_mutex_lock(&lock);
	hax_memcnt--;
	sys_mutex_unlock(&lock);
#endif

	free(ptr);
}

/**
 * Duplicate a string.
 *   @str: The string.
 *   &returns: The duplicated string.
 */
char *hax_strdup(const char *str)
{
#undef strdup
	char *dup;

	dup = hax_malloc(strlen(str) + 1);
	strcpy(dup, str);

	return dup;
}


/**
 * Increment the resource usage.
 */
void hax_inc(void)
{
#if DEBUG
	sys_mutex_lock(&lock);
	hax_memcnt++;
	sys_mutex_unlock(&lock);
#endif
}

/**
 * Decrement the resource usage.
 */
void hax_dec(void)
{
#if DEBUG
	sys_mutex_lock(&lock);
	hax_memcnt--;
	sys_mutex_unlock(&lock);
#endif
}
