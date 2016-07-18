#include "common.h"


/**
 * File reader structure.
 *   @path: The path.
 *   @file: The file.
 *   @ch: The current character.
 *   @line, col: The line and column.
 */
struct read_t {
	const char *path;
	FILE *file;
	int ch;

	unsigned int line, col;
};


/*
 * local declarations
 */
static struct read_t *read_open(const char *path);
static void read_close(struct read_t *read);

static int read_next(struct read_t *read);
static uint64_t read_num(struct read_t *read);

struct io_chunk_t read_chunk(const struct read_t *read);
static void read_proc(struct io_file_t file, void *arg);


/**
 * Open a reader.
 *   @path: The path.
 *   &returns: The reader.
 */
static struct read_t *read_open(const char *path)
{
	struct read_t *read;

	read = malloc(sizeof(struct read_t));
	read->path = path;
	read->file = fopen(path, "r");
	if(read->file == NULL)
		fatal("Cannot open '%s' for reading. %s.", path, strerror(errno));

	read->ch = fgetc(read->file);
	read->line = read->col = 1;

	return read;
}

/**
 * Close a reader.
 *   @read: The reader.
 */
static void read_close(struct read_t *read)
{
	fclose(read->file);
	free(read);
}


/**
 * Read the next character from the reader.
 *   @read: The reader.
 *   &returns: The next character.
 */
static int read_next(struct read_t *read)
{
	read->ch = fgetc(read->file);

	if(read->ch == '\n')
		read->line++, read->col = 1;
	else
		read->col++;

	return read->ch;
}

/**
 * Read a number from a reader.
 *   @read: The reader.
 *   &returns: The number.
 */
static uint64_t read_num(struct read_t *read)
{
	uint64_t num;
	struct strbuf_t buf;

	while(isspace(read->ch))
		read_next(read);

	if(!isdigit(read->ch))
		fatal("%C: Expected number.", read_chunk(read));

	buf = strbuf_init(16);

	do
		strbuf_addch(&buf, read->ch);
	while(isdigit(read_next(read)));

	strbuf_addch(&buf, '\0');

	errno = 0;
	num = strtoull(buf.arr, NULL, 10);
	if(errno != 0)
		fatal("%C: Invalid number. %s.", read_chunk(read), strerror(errno));

	strbuf_destroy(&buf);

	return num;
}

/**
 * Read a string from a reader.
 *   @read: The reader.
 *   &returns: The string.
 */
char *read_str(struct read_t *read)
{
	char *str;
	struct strbuf_t buf;

	while(isspace(read->ch))
		read_next(read);

	if(read->ch == '"')
		fatal("stub");
	else if(read->ch == '\'')
		fatal("stub");
	else if((read->ch != ',') && (read->ch != ';') && (read->ch != EOF)) {
		buf = strbuf_init(16);

		do {
			strbuf_addch(&buf, read->ch);
			read_next(read);
		} while((read->ch != EOF) && (strchr(",;\n", read->ch) == NULL));

		str = strbuf_done(&buf);
	}
	else
		fatal("%C: Invalid character '%c' where string expected.", read_chunk(read), read->ch);

	return str;
}


/**
 * Create a chunk for the current reader position.
 *   @read: The reader.
 */
struct io_chunk_t read_chunk(const struct read_t *read)
{
	return (struct io_chunk_t){ read_proc, (void *)read };
}
static void read_proc(struct io_file_t file, void *arg)
{
	const struct read_t *read = arg;

	hprintf(file, "%s:%u:%u", read->path, read->line, read->col);
}


/**
 * Open a database.
 *   @path: The path.
 *   &returns: The database.
 */
char *db_open(struct db_t **ret, const char *path)
{
#define onexit free(db);
	struct db_t *db;
	struct db_entry_t **entry;
	struct read_t *read;
	unsigned int id = 0;

	db = malloc(sizeof(struct db_t));
	db->entry = NULL;
	entry = &db->entry;

	read = read_open(path);

	while(true) {
		unsigned int i;
		uint64_t score, time;
		char *str[5];
		
		while(isspace(read->ch))
			read_next(read);

		if(read->ch == EOF)
			break;

		if(read->ch != '-') {
			score = read_num(read);
			if(score > 5)
				fail("%C: Score too large.", read_chunk(read));
		}
		else {
			score = 255;
			read_next(read);
		}

		if(read->ch != ',')
			fail("%C: Expected ','.", read_chunk(read));

		read_next(read);
		time = read_num(read);

		for(i = 0; i < 5; i++) {
			if(read->ch != ',')
				fail("%C: Expected ','.", read_chunk(read));

			read_next(read);
			str[i] = read_str(read);
		}

		if(read->ch != ';')
			fail("%C: Expected ';'.", read_chunk(read));

		read_next(read);

		*entry = malloc(sizeof(struct db_entry_t));
		(*entry)->id = id++;
		(*entry)->score = score;
		(*entry)->time = time;
		(*entry)->eng = str[0];
		(*entry)->rom = str[1];
		(*entry)->hir = str[2];
		(*entry)->kanji = str[3];
		(*entry)->audio = str[4];
		(*entry)->next = NULL;
		entry = &(*entry)->next;
	}

	read_close(read);
	*entry = NULL;
	*ret = db;

	return NULL;
#undef onexit 
}

/**
 * Close a database.
 *   @db: The database.
 */
void db_close(struct db_t *db)
{
	struct db_entry_t *entry;

	while(db->entry != NULL) {
		entry = db->entry;
		db->entry = entry->next;

		free(entry->eng);
		free(entry->rom);
		free(entry->hir);
		free(entry->kanji);
		free(entry->audio);
		free(entry);
	}

	free(db);
}


static void str_proc(struct io_file_t file, void *arg);

struct io_chunk_t str_chunk(const char *str)
{
	return (struct io_chunk_t){ str_proc, (void *)str };
}
static void str_proc(struct io_file_t file, void *arg)
{
	const char *str = arg;

	if(strpbrk(str, "\"\t") != NULL) {
		hprintf(file, "\"");

		while(*str != '\0') {
			if(strchr("\"\t", *str) != NULL)
				hprintf(file, "\\");

			hprintf(file, "%c", *str++);
		}

		hprintf(file, "\"");
	}
	else
		hprintf(file, "%s", str);
}


/**
 * Save the database to a path.
 *   @db: The database.
 *   @path: The path.
 */
void db_save(struct db_t *db, const char *path)
{
	FILE *file;
	struct db_entry_t *entry;

	file = fopen(path, "w");
	if(file == NULL)
		fatal("Cannot open '%s' for writing. %s.", path, strerror(errno));

	for(entry = db->entry; entry != NULL; entry = entry->next) {
		if(entry->score == 255)
			fprintf(file, "-,%lu,%C,%C,%C,%C,%C;\n", entry->time, str_chunk(entry->eng), str_chunk(entry->rom), str_chunk(entry->hir), str_chunk(entry->kanji), str_chunk(entry->audio));
		else
			fprintf(file, "%u,%lu,%C,%C,%C,%C,%C;\n", entry->score, entry->time, str_chunk(entry->eng), str_chunk(entry->rom), str_chunk(entry->hir), str_chunk(entry->kanji), str_chunk(entry->audio));
	}

	fclose(file);
}


/**
 * Get the entry with a given index.
 *   @db: The database.
 *   @idx: The index.
 */
struct db_entry_t *db_get(struct db_t *db, unsigned int idx)
{
	struct db_entry_t *entry;

	for(entry = db->entry; entry != NULL; entry = entry->next) {
		if(idx-- == 0)
			return entry;
	}

	return NULL;
}

/**
 * Retrieve a random entry from the database.
 *   @db: The database.
 */
struct db_entry_t *db_rand(struct db_t *db)
{
	struct avltree_t tree;
	struct db_entry_t *entry;
	uint64_t tm;
	unsigned int i, n;
	struct avltree_inst_t *inst;

	tm = sys_utime();
	tree = avltree_init(compare_ptr, delete_noop);

	for(entry = db->entry; entry != NULL; entry = entry->next) {
		if((entry->score == 255) || (entry->time > tm))
			continue;

		avltree_insert(&tree, entry, entry);
	}

	if(tree.root.count > 0) {
		n = rand() % tree.root.count;
		inst = avltree_first(&tree);
		for(i = 0; i < n; i++)
			inst = avltree_next(inst);

		entry = inst->val;
	}
	else
		entry = NULL;

	avltree_destroy(&tree);

	return entry;
}


/**
 * Increment the entry score.
 *   @entry: The entry.
 */
void db_entry_inc(struct db_entry_t *entry)
{
	if(entry->score < 5)
		entry->score++;

	db_entry_reset(entry);
}

/**
 * Decrement the entry score.
 *   @entry: The entry.
 */
void db_entry_dec(struct db_entry_t *entry)
{
	if(entry->score > 0)
		entry->score--;

	db_entry_reset(entry);
}

/**
 * Zero the entry score.
 *   @entry: The entry.
 */
void db_entry_zero(struct db_entry_t *entry)
{
	entry->score = 0;
	entry->time = 0;
}

/**
 * Reset the entry time.
 *   @entry: The entry.
 */
void db_entry_reset(struct db_entry_t *entry)
{
	uint64_t off = 0;

	switch(entry->score) {
	case 0: off = 30; break;            // new       -- 30 sec
	case 1: off = 5*60; break;          // started   --  5 min
	case 2: off = 60*60; break;         // recognize --  1 hr
	case 3: off = 24*60*60; break;      // recall    --  1 day
	case 4: off = 7*24*60*60; break;    // learned   --  1 week
	case 5: off = 4*7*24*60*60; break;  // mastered  --  1 month
	}

	entry->time = sys_utime() + off * 1000000;
}
