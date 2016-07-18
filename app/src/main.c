#include "common.h"


/**
 * File structure.
 *   @req, path, type: The request path, file path, and content-type.
 */
struct file_t {
	const char *req, *path, *type;
};

/**
 * Database mapping.
 *   @id, path: The identifier and path.
 */
struct map_t {
	const char *id, *path;
};


/*
 * local declarations
 */
static bool serv_req(const char *path, struct http_args_t *args, void *arg);
static void serv_send(struct http_args_t *args, const char *path);

static struct file_t filelist[] = {
	{ "/code.js",   "share/code.js",   "application/javascript" },
	{ "/list.js",   "share/list.js",   "application/javascript" },
	{ "/gui.js",    "share/gui.js",    "application/javascript" },
	{ "/style.css", "share/style.css", "text/css" },
	{ NULL,         NULL,            NULL }
};

static struct map_t maplist[] = {
	{ "eng",   "db/eng" },
	{ "hir",   "db/hir" },
	{ "audio", "db/audio" },
	{ "kanji", "db/kanji" },
	{ NULL,     NULL }
};


/**
 * Main entry point.
 *   @argc: The number of arguments.
 *   @argv: The argument array.
 */
int main(int argc, char **argv)
{
	struct http_server_t *serv;

	srand(sys_utime());

	chkabort(http_server_open(&serv, 8080));

	while(true) {
		unsigned int n = http_server_poll(serv, NULL);
		struct sys_poll_t set[n+1];

		set[n] = sys_poll_fd(STDIN_FILENO, POLLIN);
		http_server_poll(serv, set);

		sys_poll(set, n+1, -1);

		if(set[n].revents)
			break;

		http_server_proc(serv, set, serv_req, NULL);
	}

	while(true) {
		int ch = fgetc(stdin);
		if((ch == '\n') || (ch == EOF))
			break;
	}

	http_server_close(serv);

	if(hax_memcnt != 0)
		fprintf(stderr, "Missing %d allocation.\n", hax_memcnt);

	return 0;
}

/**
 * Handle a server request.
 *   @path: The path.
 *   @args: The arguments.
 *   @arg: The argument.
 *   &returns: True if handled, false if unhandled.
 */
static bool serv_req(const char *path, struct http_args_t *args, void *arg)
{
	char name[32], deck[16], act[8];
	unsigned int n = 0, id;
	struct file_t *file;

	if(path[0] != '/')
		return false;

	for(file = filelist; file->req != NULL; file++) {
		if(strcmp(file->req, path) == 0)
			break;
	}

	if(file->req != NULL) {
		serv_send(args, file->path);
		http_head_add(&args->resp, "Content-Type", file->type);
	}
	else if(strcmp(path, "/debug") == 0) {
		hprintf(args->file, "Debug");
		http_head_add(&args->resp, "Content-Type", "text/plaintext");
	}
	else if(sscanf(path, "/mp3/%31[a-z-_].mp3%n", name, &n) == 1) {
		char mp3[64];

		if(path[n] != '\0')
			return false;

		sprintf(mp3, "db/mp3/%s.mp3", name);
		if(access(mp3, F_OK) != 0)
			return false;

		serv_send(args, mp3);
		http_head_add(&args->resp, "Content-Type", "audio/mpeg");
	}
	else if(sscanf(path, "/%15[a-z]%n", deck, &n) == 1) {
		struct map_t *map;
		struct db_t *db;

		for(map = maplist; map->id != NULL; map++) {
			if(strcmp(map->id, deck) == 0)
				break;
		}

		if(map->id == NULL)
			return false;

		path += n;

		if(path[0] == '\0') {
			serv_send(args, "share/index.xhtml");
			http_head_add(&args->resp, "Content-Type", "application/xhtml+xml");
		}
		else if(strcmp(path, "/list") == 0) {
			serv_send(args, "share/list.xhtml");
			http_head_add(&args->resp, "Content-Type", "application/xhtml+xml");
		}
		else if(strcmp(path, "/check") == 0) {
			char *err;

			http_head_add(&args->resp, "Content-Type", "text/plaintext;charset=utf-8");

			err = db_open(&db, map->path);
			if(err == NULL) {
				struct db_entry_t *entry;

				for(entry = db->entry; entry != NULL; entry = entry->next) {
					char path[strlen(entry->audio) + 10];

					sprintf(path, "db/mp3/%s", entry->audio);
					if(access(path, F_OK) != 0)
						hprintf(args->file, "%u,%s: missing audio (%s)\n", entry->id, entry->eng, path);
				}

				db_close(db);
			}

			hprintf(args->file, "done\n");
		}
		else if(strcmp(path, "/all") == 0) {
			bool sep = false;
			struct db_entry_t *entry;

			chkabort(db_open(&db, map->path));

			hprintf(args->file, "[");
			for(entry = db->entry; entry != NULL; entry = entry->next) {
				if(entry->score == 255)
					continue;

				if(sep)
					hprintf(args->file, ",");

				sep = true;
				hprintf(args->file, "{\"id\":%u,\"score\":%u,\"time\":%lu,\"eng\":\"%s\",\"rom\":\"%s\",\"hir\":\"%s\",\"kanji\":\"%s\",\"audio\":\"%s\"}", entry->id, entry->score, entry->time, entry->eng, entry->rom, entry->hir, entry->kanji, entry->audio);
			}
			hprintf(args->file, "]");

			db_close(db);
			http_head_add(&args->resp, "Content-Type", "application/json;charset=utf-8");
		}
		else if(strcmp(path, "/rand") == 0) {
			struct db_entry_t *entry;

			chkabort(db_open(&db, map->path));
			entry = db_rand(db);

			hprintf(args->file, "{\"id\":%u,\"score\":%u,\"time\":%lu,\"eng\":\"%s\",\"rom\":\"%s\",\"hir\":\"%s\",\"kanji\":\"%s\",\"audio\":\"%s\"}", entry->id, entry->score, entry->time, entry->eng, entry->rom, entry->hir, entry->kanji, entry->audio);
			http_head_add(&args->resp, "Content-Type", "application/json;charset=utf-8");

			db_close(db);
		}
		else if((sscanf(path, "/%7[a-z]/%u%n", act, &id, &n) == 2) && (path[n] == '\0')) {
			struct db_entry_t *entry;
			void (*func)(struct db_entry_t *);

			if(strcmp(act, "inc") == 0)
				func = db_entry_inc;
			else if(strcmp(act, "dec") == 0)
				func = db_entry_dec;
			else if(strcmp(act, "zero") == 0)
				func = db_entry_zero;
			else if(strcmp(act, "reset") == 0)
				func = db_entry_reset;
			else
				return false;

			chkabort(db_open(&db, map->path));

			entry = db_get(db, id);
			if(entry != NULL)
				func(entry);

			db_save(db, map->path);
			db_close(db);

			if(entry == NULL)
				return false;

			hprintf(args->file, "ok", act, id, deck);
			http_head_add(&args->resp, "Content-Type", "text/plaintext;charset=utf-8");
		}
		else
			return false;
	}
	else
		return false;

	return true;
}

/**
 * Send a file.
 *   @args: The arguments.
 *   @path: The path.
 */
static void serv_send(struct http_args_t *args, const char *path)
{
	FILE *file;
	ssize_t rd;
	uint8_t buf[32*1024];

	file = fopen(path, "r");
	if(file == NULL)
		fatal("Missing file '%s'.", path);

	while(!feof(file)) {
		rd = fread(buf, 1, sizeof(buf), file);
		if((rd < 0) || ((rd == 0) && !feof(file)))
			fatal("Failure to read from '%s'.", path);

		io_file_write(args->file, buf, rd);
	}

	fclose(file);
}
