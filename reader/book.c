#include "book.h"

#include <errno.h>
#include <string.h>

#include <SDL.h>
#include <cJSON.h>

#include "file.h"

/* The version of panels.json this reader understands. */
#define FORMAT_VERSION 1

/* Reads a whole number called name from a JSON object. Returns 0 on success. */
static int read_int(const cJSON *object, const char *name, int *value)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, name);
    if (!cJSON_IsNumber(item)) {
        SDL_Log("panels.json: \"%s\" is missing or not a number", name);
        return -1;
    }
    *value = item->valueint;
    return 0;
}

static int read_panel(const cJSON *json, Panel *panel)
{
    if (read_int(json, "x", &panel->x) != 0 || read_int(json, "y", &panel->y) != 0 ||
        read_int(json, "width", &panel->width) != 0 ||
        read_int(json, "height", &panel->height) != 0) {
        return -1;
    }
    return 0;
}

static int read_page(const cJSON *json, Page *page)
{
    const cJSON *file = cJSON_GetObjectItemCaseSensitive(json, "file");
    const cJSON *panels = cJSON_GetObjectItemCaseSensitive(json, "panels");
    if (!cJSON_IsString(file) || !cJSON_IsArray(panels)) {
        SDL_Log("panels.json: a page has no \"file\" or no \"panels\"");
        return -1;
    }
    if (read_int(json, "width", &page->width) != 0 ||
        read_int(json, "height", &page->height) != 0) {
        return -1;
    }
    page->file = SDL_strdup(file->valuestring);
    page->panel_count = cJSON_GetArraySize(panels);
    page->panels = SDL_calloc(page->panel_count > 0 ? page->panel_count : 1, sizeof(Panel));
    if (page->file == NULL || page->panels == NULL) {
        SDL_Log("panels.json: out of memory");
        return -1;
    }
    for (int i = 0; i < page->panel_count; i++) {
        if (read_panel(cJSON_GetArrayItem(panels, i), &page->panels[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

static int read_book(const cJSON *json, Book *book)
{
    int format = 0;
    if (read_int(json, "format", &format) != 0) {
        return -1;
    }
    if (format != FORMAT_VERSION) {
        SDL_Log("panels.json has format %d; this reader understands format %d", format,
                FORMAT_VERSION);
        return -1;
    }
    const cJSON *pages = cJSON_GetObjectItemCaseSensitive(json, "pages");
    if (!cJSON_IsArray(pages) || cJSON_GetArraySize(pages) == 0) {
        SDL_Log("panels.json: \"pages\" is missing or empty");
        return -1;
    }
    book->page_count = cJSON_GetArraySize(pages);
    book->pages = SDL_calloc(book->page_count, sizeof(Page));
    if (book->pages == NULL) {
        SDL_Log("panels.json: out of memory");
        return -1;
    }
    for (int i = 0; i < book->page_count; i++) {
        if (read_page(cJSON_GetArrayItem(pages, i), &book->pages[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

int book_load(Book *book, const char *book_dir)
{
    char path[1024];
    SDL_snprintf(path, sizeof(path), "%s/panels.json", book_dir);

    book->page_count = 0;
    book->pages = NULL;

    /* file_read adds a zero byte at the end, which makes the file a string. */
    size_t size;
    char *text = file_read(path, &size);
    if (text == NULL) {
        SDL_Log("Cannot read %s: %s", path, strerror(errno));
        return -1;
    }
    cJSON *json = cJSON_Parse(text);
    SDL_free(text);
    if (json == NULL) {
        SDL_Log("%s is not valid JSON", path);
        return -1;
    }
    int result = read_book(json, book);
    cJSON_Delete(json);
    if (result != 0) {
        book_free(book);
    }
    return result;
}

void book_free(Book *book)
{
    for (int i = 0; i < book->page_count; i++) {
        SDL_free(book->pages[i].file);
        SDL_free(book->pages[i].panels);
    }
    SDL_free(book->pages);
    book->pages = NULL;
    book->page_count = 0;
}
