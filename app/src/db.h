#ifndef DB_H
#define DB_H

// 0: 0       -- new
// 1: 5 min   -- started
// 2: 1 hour  -- recognize
// 3: 1 day   -- recall
// 4: 1 week  -- learned
// 5: 1 month -- mastered

/**
 * Database structure.
 *   @entry: The main entry.
 */
struct db_t {
	struct db_entry_t *entry;
};

/**
 * Database entry structure.
 *   @id: The identifier.
 *   @next: The next entry.
 *   @score: The current score.
 *   @time: The time until next
 *   @eng, rom, hir, kanji, audio: The entry strings.
 */
struct db_entry_t {
	unsigned int id;
	struct db_entry_t *next;

	uint8_t score;
	uint64_t time;
	char *eng, *rom, *hir, *kanji, *audio;
};


/*
 * database declarations
 */
char *db_open(struct db_t **ret, const char *path);
void db_close(struct db_t *db);

void db_save(struct db_t *db, const char *path);

struct db_entry_t *db_get(struct db_t *db, unsigned int idx);
struct db_entry_t *db_rand(struct db_t *db);

/*
 * entry declarations
 */
void db_entry_inc(struct db_entry_t *entry);
void db_entry_dec(struct db_entry_t *entry);
void db_entry_zero(struct db_entry_t *entry);
void db_entry_reset(struct db_entry_t *entry);

#endif
