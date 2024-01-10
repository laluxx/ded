#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "./editor.h"
#include "./common.h"
#include "./free_glyph.h"
#include "./file_browser.h"
#include "lexer.h"
#include "simple_renderer.h"
#include <ctype.h> // For isalnum

EvilMode current_mode = NORMAL;
float zoom_factor = 3.0f;
float min_zoom_factor = 1.0;
float max_zoom_factor = 10.0;

bool isAnimated = true;
bool isWave = false;
int indentation = 4;

bool showLineNumbers = false;
bool highlightCurrentLineNumber = true;
bool relativeLineNumbers = false;

bool showWhitespaces = false;
bool copiedLine = false;
bool matchParenthesis = true; //TODO segfault and highlight size

bool hl_line = false;

bool showMinibuffer = true;


void editor_new_line_down(Editor *editor) {
    size_t row = editor_cursor_row(editor);
    size_t line_begin = editor->lines.items[row].begin;
    size_t line_end = editor->lines.items[row].end;

    editor_move_to_line_end(editor);
    editor_insert_char(editor, '\n');

    // Copy indentation
    for (size_t i = line_begin; i < line_end; ++i) {
        char c = editor->data.items[i];
        if (c == ' ' || c == '\t') {
            editor_insert_char(editor, c);
        } else {
            break;
        }
    }
}

void editor_new_line_up(Editor *editor) {
    size_t row = editor_cursor_row(editor);

    // Determine the current line's start and end for capturing indentation
    size_t line_begin = editor->lines.items[row].begin;
    size_t line_end = editor->lines.items[row].end;

    // Capture the indentation of the current line in a local array
    char indentation[128]; // Assuming 128 characters is enough for indentation
    size_t indentIndex = 0;
    for (size_t i = line_begin; i < line_end && indentIndex < sizeof(indentation) - 1; ++i) {
        char c = editor->data.items[i];
        if (c == ' ' || c == '\t') {
            indentation[indentIndex++] = c;
        } else {
            break;
        }
    }
    indentation[indentIndex] = '\0'; // Null-terminate the string

    // Insert a newline at the beginning of the current line
    editor_move_to_line_begin(editor);
    editor_insert_char(editor, '\n');
    editor_move_line_up(editor);

    // Apply the captured indentation
    for (size_t i = 0; i < indentIndex; ++i) {
        editor_insert_char(editor, indentation[i]);
    }
}


// TODO bad implementation
bool extractWordUnderCursor(Editor *editor, char *word) {
    // Make a copy of cursor position to avoid modifying the actual cursor
    size_t cursor = editor->cursor;

    // Move left to find the start of the word.
    while (cursor > 0 && isalnum(editor->data.items[cursor - 1])) {
        cursor--;
    }

    // Check if the cursor is on a word or on whitespace/special character.
    if (!isalnum(editor->data.items[cursor])) return false;

    int start = cursor;

    // Move right to find the end of the word.
    while (cursor < editor->data.count && isalnum(editor->data.items[cursor])) {
        cursor++;
    }

    int end = cursor;

    // Copy the word to the provided buffer.
    // Make sure not to overflow the buffer and null-terminate the string.
    int length = end - start;
    strncpy(word, &editor->data.items[start], length);
    word[length] = '\0';

    return true;
}



// TODO
void move_camera(Simple_Renderer *sr, const char* direction, float amount) {
    if(sr == NULL) return;

    // Check the direction and adjust the camera position accordingly.
    if(strcmp(direction, "up") == 0) {
        sr->camera_pos.y -= amount;
    } else if(strcmp(direction, "down") == 0) {
        sr->camera_pos.y += amount;
    } else if(strcmp(direction, "left") == 0) {
        sr->camera_pos.x -= amount;
    } else if(strcmp(direction, "right") == 0) {
        sr->camera_pos.x += amount;
    } else {
        printf("Invalid direction '%s'\n", direction);
    }
}







int currentThemeIndex = 0;
Theme themes[6];

void initialize_themes() {

    // Nature
    themes[0] = (Theme) {
        .cursor = hex_to_vec4f(0x658B5FFF),
        .insert_cursor = hex_to_vec4f(0x514B8EFF),
        .emacs_cursor = hex_to_vec4f(0x834EB6FF),
        .text = hex_to_vec4f(0xC0ACD1FF),
        .background = hex_to_vec4f(0x090909FF),
        .comment = hex_to_vec4f(0x867892FF),
        .hashtag = hex_to_vec4f(0x658B5FFF),
        .logic = hex_to_vec4f(0x658B5FFF),
        .string = hex_to_vec4f(0x4C6750FF),
        .selection = hex_to_vec4f(0x262626FF),
        .search = hex_to_vec4f(0x262626FF),
        .todo = hex_to_vec4f(0x565663FF),
        .line_numbers = hex_to_vec4f(0x171717FF),
        .current_line_number = hex_to_vec4f(0xC0ACD1FF),
        .fixme = hex_to_vec4f(0x444E46FF),
        .note = hex_to_vec4f(0x4C6750FF),
        .bug = hex_to_vec4f(0x867892FF),
        .not_equals = hex_to_vec4f(0x867892FF),
        .exclamation = hex_to_vec4f(0x4C6750FF),
        .equals = hex_to_vec4f(0xC0ACD1FF),
        .equals_equals = hex_to_vec4f(0x658B5FFF),
        .greater_than = hex_to_vec4f(0x834EB6FF),
        .less_than = hex_to_vec4f(0x834EB6FF),
        .marks = hex_to_vec4f(0x565663FF),
        .fb_selection = hex_to_vec4f(0x262626FF),
        .plus = hex_to_vec4f(0x658B5FFF),
        .minus = hex_to_vec4f(0x658B5FFF),
        .truee = hex_to_vec4f(0x4C6750FF),
        .falsee = hex_to_vec4f(0x867892FF),
        .arrow = hex_to_vec4f(0x834EB6FF),
        .open_square = hex_to_vec4f(0xC0ACD1FF),
        .close_square = hex_to_vec4f(0xC0ACD1FF),
        .array_content = hex_to_vec4f(0x4C6750FF),
        .link = hex_to_vec4f(0x565663FF),
        .logic_or = hex_to_vec4f(0x658B5FFF),
        .pipe = hex_to_vec4f(0x565663FF),
        .ampersand = hex_to_vec4f(0x658B5FFF),
        .logic_and = hex_to_vec4f(0x658B5FFF),
        .pointer = hex_to_vec4f(0x514B8EFF),
        .multiplication = hex_to_vec4f(0x867892FF),
        .matching_parenthesis = hex_to_vec4f(0x262626FF),
        .hl_line = hex_to_vec4f(0x070707FF),
        .type = hex_to_vec4f(0x565663FF),
        .function_definition = hex_to_vec4f(0x564F96FF),
        .anchor = hex_to_vec4f(0x564F96FF),
        .minibuffer = hex_to_vec4f(0x090909FF),
    };

    // DOOM one
    themes[1] = (Theme) {
        .cursor = hex_to_vec4f(0x51AFEFFF), //#51AFEF
        .insert_cursor = hex_to_vec4f(0x51AFEFFF),
        .emacs_cursor = hex_to_vec4f(0xECBE7BFF), //#ECBE7B
        .text = hex_to_vec4f(0xBBC2CFFF),
        .background = hex_to_vec4f(0x282C34FF),
        .comment = hex_to_vec4f(0x5B6268FF),
        .hashtag = hex_to_vec4f(0x51AFEFFF),
        .logic = hex_to_vec4f(0x51AFEFFF),
        .string = hex_to_vec4f(0x98BE65FF), //#98BE65
        .selection = hex_to_vec4f(0x42444AFF),
        .search = hex_to_vec4f(0x387AA7FF), //#387AA7
        .todo = hex_to_vec4f(0xECBE7BFF),
        .line_numbers = hex_to_vec4f(0x3F444AFF),
        .current_line_number = hex_to_vec4f(0xBBC2CFFF),
        .fixme = hex_to_vec4f(0xFF6C6BFF), //#FF6C6B
        .note = hex_to_vec4f(0x98BE65FF),
        .bug = hex_to_vec4f(0xFF6C6BFF),
        .not_equals = hex_to_vec4f(0xFF6C6BFF),
        .exclamation = hex_to_vec4f(0x51AFEFFF),
        .equals = hex_to_vec4f(0x98BE65FF),
        .equals_equals = hex_to_vec4f(0x98BE65FF),
        .greater_than = hex_to_vec4f(0x98BE65FF),
        .less_than = hex_to_vec4f(0xFF6C6BFF),
        .marks = hex_to_vec4f(0x387AA7FF),
        .fb_selection = hex_to_vec4f(0x42444AFF),
        .plus = hex_to_vec4f(0x98BE65FF),
        .minus = hex_to_vec4f(0xFF6C6BFF),
        .truee = hex_to_vec4f(0x98BE65FF),
        .falsee = hex_to_vec4f(0xFF6C6BFF),
        .arrow = hex_to_vec4f(0xBBC2CFFF),
        .open_square = hex_to_vec4f(0xBBC2CFFF),
        .close_square = hex_to_vec4f(0xBBC2CFFF),
        .array_content = hex_to_vec4f(0xA9A1E1FF),
        .link = hex_to_vec4f(0xA9A1E1FF), //#A9A1E1
        .matching_parenthesis = hex_to_vec4f(0x42444AFF),
        .type = hex_to_vec4f(0xECBE7BFF),
        .function_definition = hex_to_vec4f(0xC678DDFF), //#C678DD
        .anchor = hex_to_vec4f(0xA9A1E1FF),
        .hl_line = hex_to_vec4f(0x21242BFF),//#21242B
        .multiplication = hex_to_vec4f(0x98BE65FF),
        .pointer = hex_to_vec4f(0xA9A1E1FF),
        .logic_and = hex_to_vec4f(0x98BE65FF),
        .logic_or = hex_to_vec4f(0xFF6C6BFF),
        .ampersand = hex_to_vec4f(0x51AFEFFF),
        .pipe = hex_to_vec4f(0x98BE65FF),
        .minibuffer = hex_to_vec4f(0x21242BFF),
    };

    // Dracula
    themes[2] = (Theme) {
        .cursor = hex_to_vec4f(0xBD93F9FF), //#BD93F9
        .insert_cursor = hex_to_vec4f(0xBD93F9FF),
        .emacs_cursor = hex_to_vec4f(0xF1FA8CFF), //#F1FA8C
        .text = hex_to_vec4f(0xF8F8F2FF),
        .background = hex_to_vec4f(0x282A36FF),
        .comment = hex_to_vec4f(0x6272A4FF),
        .hashtag = hex_to_vec4f(0xBD93F9FF),
        .logic = hex_to_vec4f(0xFF79C6FF), //#FF79C6
        .string = hex_to_vec4f(0xF1FA8CFF),
        .selection = hex_to_vec4f(0x44475AFF),
        .search = hex_to_vec4f(0x8466AEFF), //#8466AE
        .todo = hex_to_vec4f(0xF1FA8CFF),
        .line_numbers = hex_to_vec4f(0x6272A4FF),
        .current_line_number = hex_to_vec4f(0xF8F8F2FF),
        .fixme = hex_to_vec4f(0xFF5555FF), //#FF5555
        .note = hex_to_vec4f(0x50FA7BFF), //#50FA7B
        .bug = hex_to_vec4f(0xFF5555FF),
        .not_equals = hex_to_vec4f(0xFF5555FF),
        .exclamation = hex_to_vec4f(0xBD93F9FF),
        .equals = hex_to_vec4f(0x50FA7BFF),
        .equals_equals = hex_to_vec4f(0x50FA7BFF),
        .greater_than = hex_to_vec4f(0x50FA7BFF),
        .less_than = hex_to_vec4f(0xFF5555FF),
        .marks = hex_to_vec4f(0x8466AEFF),
        .fb_selection = hex_to_vec4f(0x44475AFF),
        .plus = hex_to_vec4f(0x50FA7BFF),
        .minus = hex_to_vec4f(0xFF5555FF),
        .truee = hex_to_vec4f(0x50FA7BFF),
        .falsee = hex_to_vec4f(0xFF5555FF),
        .arrow = hex_to_vec4f(0x8BE9FDFF), //#8BE9FD
        .open_square = hex_to_vec4f(0xF8F8F2FF),
        .close_square = hex_to_vec4f(0xF8F8F2FF),
        .array_content = hex_to_vec4f(0xBD93F9FF),
        .link = hex_to_vec4f(0x8BE9FDFF),
        .matching_parenthesis = hex_to_vec4f(0x44475AFF),
        .type = hex_to_vec4f(0xBD93F9FF),
        .function_definition = hex_to_vec4f(0x50FA7BFF),
        .anchor = hex_to_vec4f(0xFF79C6FF),
        .hl_line = hex_to_vec4f(0x1E2029FF), //#1E2029
        .multiplication = hex_to_vec4f(0x50FA7BFF),
        .pointer = hex_to_vec4f(0xFFC9E8FF), //#FFC9E8
        .logic_and = hex_to_vec4f(0x50FA7BFF),
        .logic_or = hex_to_vec4f(0xFF5555FF),
        .ampersand = hex_to_vec4f(0x8BE9FDFF),
        .pipe = hex_to_vec4f(0x50FA7BFF),
        .minibuffer = hex_to_vec4f(0x1E2029FF), //#1E2029
    };

    // DOOM city lights
    themes[3] = (Theme){
        .cursor = hex_to_vec4f(0x5EC4FFFF),        // #5EC4FF
        .insert_cursor = hex_to_vec4f(0xE27E8DFF), // #E27E8D
        .emacs_cursor = hex_to_vec4f(0xEBBF83FF),  // #EBBF83
        .text = hex_to_vec4f(0xA0B3C5FF),
        .background = hex_to_vec4f(0x1D252CFF),
        .comment = hex_to_vec4f(0x41505EFF),
        .hashtag = hex_to_vec4f(0x5EC4FFFF),
        .logic = hex_to_vec4f(0x5EC4FFFF),
        .string = hex_to_vec4f(0x539AFCFF), // #539AFC
        .selection = hex_to_vec4f(0x28323BFF),
        .search = hex_to_vec4f(0x4189B2FF),
        .todo = hex_to_vec4f(0xEBBF83FF),
        .line_numbers = hex_to_vec4f(0x384551FF),
        .current_line_number = hex_to_vec4f(0xA0B3C5FF),
        .fixme = hex_to_vec4f(0xD95468FF), // #D95468
        .note = hex_to_vec4f(0x8BD49CFF),  // #8BD49C
        .bug = hex_to_vec4f(0xD95468FF),
        .not_equals = hex_to_vec4f(0xD95468FF),
        .exclamation = hex_to_vec4f(0x5EC4FFFF),
        .equals = hex_to_vec4f(0x8BD49CFF),
        .equals_equals = hex_to_vec4f(0x8BD49CFF),
        .greater_than = hex_to_vec4f(0x8BD49CFF),
        .less_than = hex_to_vec4f(0xD95468FF),
        .marks = hex_to_vec4f(0x4189B2FF),
        .fb_selection = hex_to_vec4f(0x28323BFF),
        .plus = hex_to_vec4f(0x8BD49CFF),
        .minus = hex_to_vec4f(0xD95468FF),
        .truee = hex_to_vec4f(0x8BD49CFF),
        .falsee = hex_to_vec4f(0xD95468FF),
        .arrow = hex_to_vec4f(0xA0B3C5FF),
        .open_square = hex_to_vec4f(0xA0B3C5FF),
        .close_square = hex_to_vec4f(0xA0B3C5FF),
        .array_content = hex_to_vec4f(0x539AFCFF),
        .link = hex_to_vec4f(0x539AFCFF),
        .matching_parenthesis = hex_to_vec4f(0x28323BFF),
        .type = hex_to_vec4f(0xEBBF83FF),
        .function_definition = hex_to_vec4f(0x33CED8FF), // #33CED8
        .anchor = hex_to_vec4f(0xE27E8DFF),
        .hl_line = hex_to_vec4f(0x181E24FF),
        .multiplication = hex_to_vec4f(0x8BD49CFF),
        .pointer = hex_to_vec4f(0x539AFCFF),
        .logic_and = hex_to_vec4f(0x8BD49CFF),
        .logic_or = hex_to_vec4f(0xD95468FF),
        .ampersand = hex_to_vec4f(0x5EC4FFFF),
        .pipe = hex_to_vec4f(0x8BD49CFF),
        .minibuffer = hex_to_vec4f(0x181E24FF),
    };


    // DOOM molokai
    themes[4] = (Theme) {
        .cursor = hex_to_vec4f(0xFB2874FF), //#FB2874
        .insert_cursor = hex_to_vec4f(0xFB2874FF),
        .emacs_cursor = hex_to_vec4f(0xE2C770FF), //#E2C770
        .text = hex_to_vec4f(0xD6D6D4FF),
        .background = hex_to_vec4f(0x1C1E1FFF),
        .comment = hex_to_vec4f(0x555556FF),
        .hashtag = hex_to_vec4f(0x9C91E4FF), //#9C91E4
        .logic = hex_to_vec4f(0xFB2874FF),
        .string = hex_to_vec4f(0xE2C770FF),
        .selection = hex_to_vec4f(0x4E4E4EFF),
        .search = hex_to_vec4f(0x9C91E4FF),
        .todo = hex_to_vec4f(0xE2C770FF),
        .line_numbers = hex_to_vec4f(0x555556FF),
        .current_line_number = hex_to_vec4f(0xCFC0C5FF),
        .fixme = hex_to_vec4f(0xE74C3CFF), //#E74C3C
        .note = hex_to_vec4f(0xB6E63EFF), //#B6E63E
        .bug = hex_to_vec4f(0xE74C3CFF),
        .not_equals = hex_to_vec4f(0xE74C3CFF),
        .exclamation = hex_to_vec4f(0x9C91E4FF),
        .equals = hex_to_vec4f(0xB6E63EFF),
        .equals_equals = hex_to_vec4f(0xB6E63EFF),
        .greater_than = hex_to_vec4f(0xB6E63EFF),
        .less_than = hex_to_vec4f(0xE74C3CFF),
        .marks = hex_to_vec4f(0xB6E63EFF),
        .fb_selection = hex_to_vec4f(0x4E4E4EFF),
        .plus = hex_to_vec4f(0xB6E63EFF),
        .minus = hex_to_vec4f(0xE74C3CFF),
        .truee = hex_to_vec4f(0xB6E63EFF),
        .falsee = hex_to_vec4f(0xE74C3CFF),
        .arrow = hex_to_vec4f(0xD6D6D4FF),
        .open_square = hex_to_vec4f(0xD6D6D4FF),
        .close_square = hex_to_vec4f(0xD6D6D4FF),
        .array_content = hex_to_vec4f(0x9C91E4FF),
        .link = hex_to_vec4f(0x9C91E4FF),
        .matching_parenthesis = hex_to_vec4f(0x4E4E4EFF),
        .type = hex_to_vec4f(0x66D9EFFF),
        .function_definition = hex_to_vec4f(0xB6E63EFF),
        .anchor = hex_to_vec4f(0x9C91E4FF),
        .hl_line = hex_to_vec4f(0x222323FF),
        .multiplication = hex_to_vec4f(0xB6E63EFF),
        .pointer = hex_to_vec4f(0x9C91E4FF),
        .logic_and = hex_to_vec4f(0xB6E63EFF),
        .logic_or = hex_to_vec4f(0xE74C3CFF),
        .ampersand = hex_to_vec4f(0x9C91E4FF),
        .pipe = hex_to_vec4f(0xB6E63EFF),
        .minibuffer = hex_to_vec4f(0x222323FF),
    };

    

    // Palenight
    themes[5] = (Theme) {
        .cursor = hex_to_vec4f(0xC792EAFF), //#C792EA
        .insert_cursor = hex_to_vec4f(0xC792EAFF),
        .emacs_cursor = hex_to_vec4f(0xFFCB6BFF), //#FFCB6B
        .text = hex_to_vec4f(0xEEFFFFFF),
        .background = hex_to_vec4f(0x292D3EFF),
        .comment = hex_to_vec4f(0x676E95FF),
        .hashtag = hex_to_vec4f(0x89DDFFFF), //#89DDFF
        .logic = hex_to_vec4f(0x89DDFFFF),
        .string = hex_to_vec4f(0xC3E88DFF),  //#C3E88D
        .selection = hex_to_vec4f(0x3C435EFF),
        .search = hex_to_vec4f(0x4E5579FF),
        .todo = hex_to_vec4f(0xFFCB6BFF),
        .line_numbers = hex_to_vec4f(0x676E95FF),
        .current_line_number = hex_to_vec4f(0xEEFFFFFF),
        .fixme = hex_to_vec4f(0xFF5370FF), //#FF5370
        .note = hex_to_vec4f(0xC3E88DFF),
        .bug = hex_to_vec4f(0xFF5370FF),
        .not_equals = hex_to_vec4f(0xFF5370FF),
        .exclamation = hex_to_vec4f(0x89DDFFFF),
        .equals = hex_to_vec4f(0xC3E88DFF),
        .equals_equals = hex_to_vec4f(0xC3E88DFF),
        .greater_than = hex_to_vec4f(0xC3E88DFF),
        .less_than = hex_to_vec4f(0xFF5370FF),
        .marks = hex_to_vec4f(0x4E5579FF),
        .fb_selection = hex_to_vec4f(0x3C435EFF),
        .plus = hex_to_vec4f(0xC3E88DFF),
        .minus = hex_to_vec4f(0xFF5370FF),
        .truee = hex_to_vec4f(0xC3E88DFF),
        .falsee = hex_to_vec4f(0xFF5370FF),
        .arrow = hex_to_vec4f(0xFFCB6BFF),
        .open_square = hex_to_vec4f(0xEEFFFFFF),
        .close_square = hex_to_vec4f(0xEEFFFFFF),
        .array_content = hex_to_vec4f(0x82AAFFFF), //#82AAFF
        .link = hex_to_vec4f(0x89DDFFFF),
        .logic_or = hex_to_vec4f(0xFF5370FF),
        .pipe = hex_to_vec4f(0xC3E88DFF),
        .ampersand = hex_to_vec4f(0x89DDFFFF),
        .logic_and = hex_to_vec4f(0xC3E88DFF),
        .pointer = hex_to_vec4f(0xF78C6CFF), //#F78C6C
        .multiplication = hex_to_vec4f(0xC3E88DFF),
        .matching_parenthesis = hex_to_vec4f(0x3C435EFF),
        .hl_line = hex_to_vec4f(0x242837FF),
        .type = hex_to_vec4f(0xC792EAFF),
        .function_definition = hex_to_vec4f(0x82AAFFFF),
        .anchor = hex_to_vec4f(0xFF5370FF),
        .minibuffer = hex_to_vec4f(0x292D3EFF),
    };
 }

void theme_next(int *currentThemeIndex) {
    const int themeCount = sizeof(themes) / sizeof(themes[0]);
    *currentThemeIndex += 1;
    if (*currentThemeIndex >= themeCount) {
        *currentThemeIndex = 0;  // wrap around
    }
}

void theme_previous(int *currentThemeIndex) {
    *currentThemeIndex -= 1;
    if (*currentThemeIndex < 0) {
        const int themeCount = sizeof(themes) / sizeof(themes[0]);
        *currentThemeIndex = themeCount - 1;  // wrap around to the last theme
    }
}

void editor_backspace(Editor *e) {
    // If in search mode, reduce the search query length
    if (e->searching) {
        if (e->search.count > 0) {
            e->search.count -= 1;
        }
    } else {
        // Check if the cursor is at the beginning or at the beginning of a line
        if (e->cursor == 0) return; // Cursor at the beginning, nothing to delete

        size_t cursor_pos = e->cursor;
        size_t row = editor_cursor_row(e);

        if (cursor_pos > e->data.count) {
            cursor_pos = e->data.count;
        }

        // Determine the characters before and after the cursor
        char char_before_cursor = (cursor_pos > 0) ? e->data.items[cursor_pos - 1] : '\0';
        char char_after_cursor = (cursor_pos < e->data.count) ? e->data.items[cursor_pos] : '\0';

        // Smart parentheses: delete both characters if they match
        if ((char_before_cursor == '(' && char_after_cursor == ')') ||
            (char_before_cursor == '[' && char_after_cursor == ']') ||
            (char_before_cursor == '{' && char_after_cursor == '}') ||
            (char_before_cursor == '\'' && char_after_cursor == '\'') ||
            (char_before_cursor == '"' && char_after_cursor == '"')) {
            memmove(&e->data.items[cursor_pos - 1], &e->data.items[cursor_pos + 1], e->data.count - cursor_pos);
            e->cursor -= 1;
            e->data.count -= 2;
        } else if (editor_is_line_empty(e, row)) {
          if (row > 0) {
            // If it's not the first line, delete the newline character from the previous line
            size_t newline_pos = e->lines.items[row - 1].end; // Position of newline character
            memmove(&e->data.items[newline_pos], &e->data.items[newline_pos + 1], e->data.count - newline_pos - 1);
            e->cursor = newline_pos; // Move cursor to the end of the previous line
            e->data.count -= 1;
          } else if (e->lines.count > 1) {
            // If it's the first line but there are more lines, delete the newline character at the end of this line
            size_t newline_pos = e->lines.items[row].end; // Position of newline character
            memmove(&e->data.items[newline_pos], &e->data.items[newline_pos + 1], e->data.count - newline_pos - 1);
            e->data.count -= 1;
            // Cursor stays at the beginning of the next line (which is now the first line)
          }
        } else if (editor_is_line_whitespaced(e, row)) {
            /* // If the line is only whitespaces */
            /* size_t line_begin = e->lines.items[row].begin; */
            /* size_t delete_length = (cursor_pos - line_begin >= indentation) ? indentation : cursor_pos - line_begin; */

            /* memmove(&e->data.items[cursor_pos - delete_length], &e->data.items[cursor_pos], e->data.count - cursor_pos); */
            /* e->cursor -= delete_length; */
            /* e->data.count -= delete_length; */

            // If the line is only whitespaces
            size_t line_begin = e->lines.items[row].begin;
            size_t line_end = e->lines.items[row].end;
            size_t whitespace_length = cursor_pos - line_begin;
            
            if (whitespace_length == indentation) {
                // If the number of whitespaces matches indentation exactly, remove the entire line
                if (row < e->lines.count - 1) {
                    memmove(&e->data.items[line_begin], &e->data.items[line_end + 1], e->data.count - line_end - 1);
                    e->data.count -= (line_end - line_begin + 1);
                    e->cursor = line_begin; // Update cursor position to the beginning of the next line
                } else if (row > 0 && e->data.items[line_begin - 1] == '\n') {
                    // If it's the last line, remove the preceding newline character
                    e->data.count -= 1;
                    memmove(&e->data.items[line_begin - 1], &e->data.items[line_end], e->data.count - line_end);
                    e->cursor = (line_begin > 1) ? line_begin - 1 : 0; // Move cursor to the end of the previous line, plus one character
                }
                // Update the cursor position if it's not the first line
                if (row > 0) {
                    e->cursor = e->lines.items[row - 1].end; // Move cursor to one character right of the end of the previous line
                    if (e->cursor > e->data.count) e->cursor = e->data.count; // Bound check
                }
            } else {
                // Original behavior for deleting whitespaces
                size_t delete_length = (whitespace_length >= indentation) ? indentation : whitespace_length;
                memmove(&e->data.items[cursor_pos - delete_length], &e->data.items[cursor_pos], e->data.count - cursor_pos);
                e->cursor -= delete_length;
                e->data.count -= delete_length;
            }
        } else {
            // Delete only the character before the cursor
            memmove(&e->data.items[cursor_pos - 1], &e->data.items[cursor_pos], e->data.count - cursor_pos);
            e->cursor -= 1;
            e->data.count -= 1;
        }
        editor_retokenize(e);
    }
}


// Unused ?
void editor_delete(Editor *e)
{
    if (e->searching) return;

    if (e->cursor >= e->data.count) return;
    memmove(
        &e->data.items[e->cursor],
        &e->data.items[e->cursor + 1],
        e->data.count - e->cursor - 1
    );
    e->data.count -= 1;
    editor_retokenize(e);
}

void editor_delete_selection(Editor *e)
{
    assert(e->selection);

    size_t begin = e->select_begin;
    size_t end = e->cursor;
    if (begin > end) {
        SWAP(size_t, begin, end);
    }

    if (end >= e->data.count) {
        end = e->data.count - 1;
    }
    if (begin == e->data.count) return;

    size_t nchars = end - begin + 1; // Correct calculation to include the end character

    memmove(
        &e->data.items[begin],
        &e->data.items[end + 1],
        e->data.count - end - 1
    );

    e->data.count -= nchars;
    e->cursor = begin; // Set cursor to the beginning of the deleted range

    editor_retokenize(e);
}



// TODO: make sure that you always have new line at the end of the file while saving
// https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_206

Errno editor_save_as(Editor *e, const char *file_path)
{
    printf("Saving as %s...\n", file_path);
    Errno err = write_entire_file(file_path, e->data.items, e->data.count);
    if (err != 0) return err;
    e->file_path.count = 0;
    sb_append_cstr(&e->file_path, file_path);
    sb_append_null(&e->file_path);
    return 0;
}

Errno editor_save(const Editor *e)
{
    assert(e->file_path.count > 0);
    printf("Saving as %s...\n", e->file_path.items);
    return write_entire_file(e->file_path.items, e->data.items, e->data.count);
}

Errno editor_load_from_file(Editor *e, const char *file_path)
{
    printf("Loading %s\n", file_path);

    e->data.count = 0;
    Errno err = read_entire_file(file_path, &e->data);
    if (err != 0) return err;

    e->cursor = 0;

    editor_retokenize(e);

    e->file_path.count = 0;
    sb_append_cstr(&e->file_path, file_path);
    sb_append_null(&e->file_path);

    // Add file path to buffer history
    if (e->buffer_history_count < MAX_BUFFER_HISTORY) {
        e->buffer_history[e->buffer_history_count++] = strdup(file_path);
    }

    return 0;
}

size_t editor_cursor_row(const Editor *e)
{
    assert(e->lines.count > 0);
    for (size_t row = 0; row < e->lines.count; ++row) {
        Line line = e->lines.items[row];
        if (line.begin <= e->cursor && e->cursor <= line.end) {
            return row;
        }
    }
    return e->lines.count - 1;
}

void editor_move_line_up(Editor *e)
{
    editor_stop_search(e);

    size_t cursor_row = editor_cursor_row(e);
    size_t cursor_col = e->cursor - e->lines.items[cursor_row].begin;
    if (cursor_row > 0) {
        Line next_line = e->lines.items[cursor_row - 1];
        size_t next_line_size = next_line.end - next_line.begin;
        if (cursor_col > next_line_size) cursor_col = next_line_size;
        e->cursor = next_line.begin + cursor_col;
    }
}

void editor_move_line_down(Editor *e)
{
    editor_stop_search(e);

    size_t cursor_row = editor_cursor_row(e);
    size_t cursor_col = e->cursor - e->lines.items[cursor_row].begin;
    if (cursor_row < e->lines.count - 1) {
        Line next_line = e->lines.items[cursor_row + 1];
        size_t next_line_size = next_line.end - next_line.begin;
        if (cursor_col > next_line_size) cursor_col = next_line_size;
        e->cursor = next_line.begin + cursor_col;
    }
}

void editor_move_char_left(Editor *e)
{
    editor_stop_search(e);
    if (e->cursor > 0) e->cursor -= 1;
}

void editor_move_char_right(Editor *e)
{
    editor_stop_search(e);
    if (e->cursor < e->data.count) e->cursor += 1;
}

void editor_move_word_left(Editor *e)
{
    editor_stop_search(e);
    while (e->cursor > 0 && !isalnum(e->data.items[e->cursor - 1])) {
        e->cursor -= 1;
    }
    while (e->cursor > 0 && isalnum(e->data.items[e->cursor - 1])) {
        e->cursor -= 1;
    }
}

void editor_move_word_right(Editor *e)
{
    editor_stop_search(e);
    while (e->cursor < e->data.count && !isalnum(e->data.items[e->cursor])) {
        e->cursor += 1;
    }
    while (e->cursor < e->data.count && isalnum(e->data.items[e->cursor])) {
        e->cursor += 1;
    }
}

void editor_insert_char(Editor *e, char x)
{
    editor_insert_buf(e, &x, 1);
}

void editor_insert_buf(Editor *e, char *buf, size_t buf_len)
{
    if (e->searching) {
        sb_append_buf(&e->search, buf, buf_len);
        bool matched = false;
        for (size_t pos = e->cursor; pos < e->data.count; ++pos) {
            if (editor_search_matches_at(e, pos)) {
                e->cursor = pos;
                matched = true;
                break;
            }
        }
        if (!matched) e->search.count -= buf_len;
    } else {
        if (e->cursor > e->data.count) {
            e->cursor = e->data.count;
        }

        for (size_t i = 0; i < buf_len; ++i) {
            da_append(&e->data, '\0');
        }
        memmove(
            &e->data.items[e->cursor + buf_len],
            &e->data.items[e->cursor],
            e->data.count - e->cursor - buf_len
        );
        memcpy(&e->data.items[e->cursor], buf, buf_len);
        e->cursor += buf_len;
        editor_retokenize(e);
    }
}


void editor_insert_buf_at(Editor *e, char *buf, size_t buf_len, size_t pos) {
    // Ensure the position is within bounds
    if (pos > e->data.count) {
        pos = e->data.count;
    }

    // Expand the buffer to accommodate the new text
    for (size_t i = 0; i < buf_len; ++i) {
        da_append(&e->data, '\0'); // Assuming da_append is a function to expand the buffer
    }

    // Shift existing text to make room for the new text
    memmove(&e->data.items[pos + buf_len], &e->data.items[pos], e->data.count - pos);

    // Copy the new text into the buffer at the specified position
    memcpy(&e->data.items[pos], buf, buf_len);

    // Update the cursor position and retokenize
    e->cursor = pos + buf_len;
    editor_retokenize(e);
}


void editor_retokenize(Editor *e)
{
    // Lines
    {
        e->lines.count = 0;

        Line line;
        line.begin = 0;

        for (size_t i = 0; i < e->data.count; ++i) {
            if (e->data.items[i] == '\n') {
                line.end = i;
                da_append(&e->lines, line);
                line.begin = i + 1;
            }
        }

        line.end = e->data.count;
        da_append(&e->lines, line);
    }

    // Syntax Highlighting
    {
        e->tokens.count = 0;
        Lexer l = lexer_new(e->atlas, e->data.items, e->data.count);
        /* Lexer l = lexer_new(e->atlas, e->data.items, e->data.count, e->file_path); */
        Token t = lexer_next(&l);
        while (t.kind != TOKEN_END) {
            da_append(&e->tokens, t);
            t = lexer_next(&l);
        }
    }
}

bool editor_line_starts_with(Editor *e, size_t row, size_t col, const char *prefix)
{
    size_t prefix_len = strlen(prefix);
    if (prefix_len == 0) {
        return true;
    }
    Line line = e->lines.items[row];
    if (col + prefix_len - 1 >= line.end) {
        return false;
    }
    for (size_t i = 0; i < prefix_len; ++i) {
        if (prefix[i] != e->data.items[line.begin + col + i]) {
            return false;
        }
    }
    return true;
}

const char *editor_line_starts_with_one_of(Editor *e, size_t row, size_t col, const char **prefixes, size_t prefixes_count)
{
    for (size_t i = 0; i < prefixes_count; ++i) {
        if (editor_line_starts_with(e, row, col, prefixes[i])) {
            return prefixes[i];
        }
    }
    return NULL;
}

void editor_render(SDL_Window *window, Free_Glyph_Atlas *atlas, Simple_Renderer *sr, Editor *editor)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    float max_line_len = 0.0f;

    sr->resolution = vec2f(w, h);
    sr->time = (float) SDL_GetTicks() / 1000.0f;

    float lineNumberWidth = FREE_GLYPH_FONT_SIZE * 5;
    /* Vec4f lineNumberColor = vec4f(0.5, 0.5, 0.5, 1);  // A lighter color for line numbers, adjust as needed */

    // Calculate the width of a whitespace character
    Vec2f whitespace_size = vec2fs(0.0f);
    free_glyph_atlas_measure_line_sized(atlas, " ", 1, &whitespace_size);
    float whitespace_width = whitespace_size.x;


    
    // Render hl_line
    {
        if (hl_line){
            simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
            
            size_t currentLine = editor_cursor_row(editor);
            Vec2f highlightPos = {0.0f, -((float)currentLine + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE};
            
            float highlightWidth = 8000;  // Default width for the highlight
            
            // If showing line numbers, adjust the position and width of the highlight
            if (showLineNumbers) {
                highlightPos.x -= lineNumberWidth - 260;  // Move highlight to the left to cover line numbers
                highlightWidth += lineNumberWidth;  // Increase width to include line numbers area
            }
            
            simple_renderer_solid_rect(sr, highlightPos, vec2f(highlightWidth, FREE_GLYPH_FONT_SIZE), themes[currentThemeIndex].hl_line);
            
            simple_renderer_flush(sr);
        }
    }

    // Render anchor
    if (editor->has_anchor) {
        simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);

        // Update the anchor position before rendering
        editor_update_anchor(editor);
        
        size_t anchor_row = editor_row_from_pos(editor, editor->anchor_pos);
        Line anchor_line = editor->lines.items[anchor_row];
        size_t anchor_col = editor->anchor_pos - anchor_line.begin;
        
        Vec2f anchor_pos_vec = vec2fs(0.0f);
        anchor_pos_vec.y = -((float)anchor_row + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE;
        anchor_pos_vec.x = free_glyph_atlas_cursor_pos(
                                                       atlas,
                                                       editor->data.items + anchor_line.begin, anchor_line.end - anchor_line.begin,
                                                       vec2f(0.0, anchor_pos_vec.y),
                                                       anchor_col
                                                       );
        
        // Adjust anchor position if line numbers are shown
        if (showLineNumbers) {
            anchor_pos_vec.x += lineNumberWidth;
        }
        
        Vec4f ANCHOR_COLOR = themes[currentThemeIndex].anchor;
        
        simple_renderer_solid_rect(
                                   sr, anchor_pos_vec, vec2f(whitespace_width, FREE_GLYPH_FONT_SIZE),
                                   ANCHOR_COLOR);
        
        
        simple_renderer_flush(sr);
    }

    
    // Render selection

    {
        if (isWave){
            simple_renderer_set_shader(sr, VERTEX_SHADER_WAVE, SHADER_FOR_COLOR);
        }else{
            simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
        }
        if (editor->selection) {
            for (size_t row = 0; row < editor->lines.count; ++row) {
                size_t select_begin_chr = editor->select_begin;
                size_t select_end_chr = editor->cursor;
                if (select_begin_chr > select_end_chr) {
                    SWAP(size_t, select_begin_chr, select_end_chr);
                }

                Line line_chr = editor->lines.items[row];

                if (select_begin_chr < line_chr.begin) {
                    select_begin_chr = line_chr.begin;
                }

                if (select_end_chr > line_chr.end) {
                    select_end_chr = line_chr.end;
                }

                if (select_begin_chr <= select_end_chr) {
                    Vec2f select_begin_scr = vec2f(0, -((float)row + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE);
                    free_glyph_atlas_measure_line_sized(
                                                        atlas, editor->data.items + line_chr.begin, select_begin_chr - line_chr.begin,
                                                        &select_begin_scr);

                    Vec2f select_end_scr = select_begin_scr;
                    free_glyph_atlas_measure_line_sized(
                                                        atlas, editor->data.items + select_begin_chr, select_end_chr - select_begin_chr,
                                                        &select_end_scr);

                    // Adjust selection for line numbers if displayed
                    if (showLineNumbers) {
                        select_begin_scr.x += lineNumberWidth;
                        select_end_scr.x += lineNumberWidth;
                    }

                    Vec4f selection_color = vec4f(.25, .25, .25, 1);

                    simple_renderer_solid_rect(sr, select_begin_scr, vec2f(select_end_scr.x - select_begin_scr.x, FREE_GLYPH_FONT_SIZE), selection_color);
                }
            }
        }
        simple_renderer_flush(sr);
    }

    Vec2f cursor_pos = vec2fs(0.0f);
    {
        size_t cursor_row = editor_cursor_row(editor);
        Line line = editor->lines.items[cursor_row];
        size_t cursor_col = editor->cursor - line.begin;
        cursor_pos.y = -((float)cursor_row + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE;
        cursor_pos.x = free_glyph_atlas_cursor_pos(
                           atlas,
                           editor->data.items + line.begin, line.end - line.begin,
                           vec2f(0.0, cursor_pos.y),
                           cursor_col
                       );
    }

    
    // Render search
    {
        if (editor->searching) {
            simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
            Vec4f selection_color = themes[currentThemeIndex].search; // or .selection_color if that's what you named it in the struct.

            Vec2f p1 = cursor_pos;
            Vec2f p2 = p1;

            free_glyph_atlas_measure_line_sized(editor->atlas, editor->search.items, editor->search.count, &p2);

            // Adjust for line numbers width if they are displayed
            if (showLineNumbers) {
                p1.x += lineNumberWidth;
                p2.x += lineNumberWidth;
            }

            simple_renderer_solid_rect(sr, p1, vec2f(p2.x - p1.x, FREE_GLYPH_FONT_SIZE), selection_color);
            simple_renderer_flush(sr);
        }
    }

    // Render marked search result
    {
        simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
        if (editor->has_mark) {
            for (size_t row = 0; row < editor->lines.count; ++row) {
                size_t mark_begin_chr = editor->mark_start;
                size_t mark_end_chr = editor->mark_end;

                Line line_chr = editor->lines.items[row];

                if (mark_begin_chr < line_chr.begin) {
                    mark_begin_chr = line_chr.begin;
                }

                if (mark_end_chr > line_chr.end) {
                    mark_end_chr = line_chr.end;
                }

                if (mark_begin_chr <= mark_end_chr) {
                    Vec2f mark_begin_scr = vec2f(0, -((float)row + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE);
                    free_glyph_atlas_measure_line_sized(
                                                        atlas, editor->data.items + line_chr.begin, mark_begin_chr - line_chr.begin,
                                                        &mark_begin_scr);

                    Vec2f mark_end_scr = mark_begin_scr;
                    free_glyph_atlas_measure_line_sized(
                                                        atlas, editor->data.items + mark_begin_chr, mark_end_chr - mark_begin_chr,
                                                        &mark_end_scr);

                    // Adjust for line numbers width if they are displayed
                    if (showLineNumbers) {
                        mark_begin_scr.x += lineNumberWidth;
                        mark_end_scr.x += lineNumberWidth;
                    }

                    Vec4f mark_color = themes[currentThemeIndex].marks;
                    simple_renderer_solid_rect(sr, mark_begin_scr, vec2f(mark_end_scr.x - mark_begin_scr.x, FREE_GLYPH_FONT_SIZE), mark_color);
                }
            }
        }
        simple_renderer_flush(sr);
    }



    // Render line numbers
    if (showLineNumbers) {
        if (isWave) {
            simple_renderer_set_shader(sr, VERTEX_SHADER_WAVE, SHADER_FOR_TEXT);
        } else {
            simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_TEXT);
        }

        // Determine the current line number using the provided function
        size_t currentLineNumber = editor_cursor_row(editor);

        // Different colors for line numbers
        Vec4f defaultColor = themes[currentThemeIndex].line_numbers;
        Vec4f currentLineColor = themes[currentThemeIndex].current_line_number;

        for (size_t i = 0; i < editor->lines.count; ++i) {
            char lineNumberStr[10];

            // Calculate display line number based on relative number setting
            size_t displayLineNumber;
            if (relativeLineNumbers) {
                if (i == currentLineNumber) {
                    // Display the actual line number for the current line
                    displayLineNumber = currentLineNumber + 1;
                } else {
                    // Show the distance from the current line for other lines
                    displayLineNumber = (i > currentLineNumber) ? i - currentLineNumber : currentLineNumber - i;
                }
            } else {
                displayLineNumber = i + 1;
            }

            snprintf(lineNumberStr, sizeof(lineNumberStr), "%zu", displayLineNumber);

            Vec2f pos = {0, -((float)i + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE};

            // Decide on the color to use
            Vec4f colorToUse = defaultColor;
            if (highlightCurrentLineNumber && i == currentLineNumber) {
                colorToUse = currentLineColor;
            }

            free_glyph_atlas_render_line_sized(atlas, sr, lineNumberStr, strlen(lineNumberStr), &pos, colorToUse);
        }

        simple_renderer_flush(sr);
    }

    // Render matching parenthesis
    {
        if (current_mode == NORMAL || current_mode == EMACS) {
            if (matchParenthesis) {
                if (isWave) {
                    simple_renderer_set_shader(sr, VERTEX_SHADER_WAVE, SHADER_FOR_COLOR);
                } else {
                    simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
                }
                
                ssize_t matching_pos = find_matching_parenthesis(editor, editor->cursor);
                if (matching_pos != -1) {
                    size_t matching_row = editor_row_from_pos(editor, matching_pos);
                    
                    Vec2f match_pos_screen = vec2fs(0.0f); // Initialize to zero
                    match_pos_screen.y = -((float)matching_row + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE;
                    
                    Line line = editor->lines.items[matching_row];
                    if (matching_pos >= line.begin && matching_pos < line.end) {
                        // Measure the position up to the matching character
                        free_glyph_atlas_measure_line_sized(atlas, editor->data.items + line.begin, matching_pos - line.begin, &match_pos_screen);
                        
                        // Measure the width of the actual character at the matching position
                        Vec2f char_end_pos = match_pos_screen;
                        free_glyph_atlas_measure_line_sized(atlas, editor->data.items + matching_pos, 1, &char_end_pos);
                        float char_width = char_end_pos.x - match_pos_screen.x;
                        
                        // Adjust for line numbers if displayed
                        if (showLineNumbers) {
                            match_pos_screen.x += lineNumberWidth;
                        }
                        
                        // Define the size of the highlight rectangle to match character size
                        Vec2f rect_size = vec2f(char_width, FREE_GLYPH_FONT_SIZE);
                        
                        simple_renderer_solid_rect(sr, match_pos_screen, rect_size, themes[currentThemeIndex].matching_parenthesis);
                    }
                }
            }
            simple_renderer_flush(sr);
        }
    }

    
    // Render text
    {
        if (isWave) {
            simple_renderer_set_shader(sr, VERTEX_SHADER_WAVE, SHADER_FOR_TEXT);
        } else {
            simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_TEXT);
        }
        for (size_t i = 0; i < editor->tokens.count; ++i) {
            Token token = editor->tokens.items[i];
            Vec2f pos = token.position;
            //Vec4f color = vec4fs(1);
            // TODO match color for open and close
            Vec4f color = themes[currentThemeIndex].text;

            // Adjust for line numbers width if they are displayed
            if (showLineNumbers) {
                pos.x += lineNumberWidth;
            }
            
            switch (token.kind) {
            case TOKEN_PREPROC:
                if (token.text_len >= 7 && token.text[0] == '#') { // Check if it's likely a hex color
                    bool valid_hex = true;
                    for (size_t j = 1; j < 7 && valid_hex; ++j) {
                        if (!is_hex_digit(token.text[j])) {
                            valid_hex = false;
                        }
                    }

                    if (valid_hex) {
                        unsigned int hex_value;
                        if(sscanf(token.text, "#%06x", &hex_value) == 1) {
                            color = hex_to_vec4f(hex_value);
                        } else {
                            color = themes[currentThemeIndex].hashtag; // Default to the hashtag color if not a valid hex
                        }
                    } else {
                        color = themes[currentThemeIndex].hashtag; // Not a valid hex color
                    }
                } else {
                    color = themes[currentThemeIndex].hashtag; // Default color for preprocessor directives
                }
                break;

            case TOKEN_KEYWORD:
                color = themes[currentThemeIndex].logic;
                break;
                
            case TOKEN_TYPE:
                color = themes[currentThemeIndex].type;
                break;
                
            case TOKEN_FUNCTION_DEFINITION:
                color = themes[currentThemeIndex].function_definition;
                break;

            case TOKEN_LINK:
                color = themes[currentThemeIndex].link;
                break;

            case TOKEN_OR:
                color = themes[currentThemeIndex].logic_or;
                break;

            case TOKEN_PIPE:
                color = themes[currentThemeIndex].pipe;
                break;

            case TOKEN_AND:
                color = themes[currentThemeIndex].logic_and;
                break;

            case TOKEN_AMPERSAND:
                color = themes[currentThemeIndex].ampersand;
                break;

            case TOKEN_POINTER:
                color = themes[currentThemeIndex].pointer;
                break;

            case TOKEN_MULTIPLICATION:
                color = themes[currentThemeIndex].multiplication;
                break;

            case TOKEN_COMMENT:
                {
                    color = themes[currentThemeIndex].comment;

                    // Checking for TODOOOO...
                    char* todoLoc = strstr(token.text, "TODO");
                    if (todoLoc && (size_t)(todoLoc - token.text + 3) < token.text_len) {

                        size_t numOs = 0;
                        char* ptr = todoLoc + 4; // Start right after "TODO"

                        // Count 'O's without crossing token boundary
                        while ((size_t)(ptr - token.text) < token.text_len && (*ptr == 'O' || *ptr == 'o')) {

                            numOs++;
                            ptr++;
                        }

                        Vec4f baseColor = themes[currentThemeIndex].todo;
                        float deltaRed = (1.0f - baseColor.x) / 5;  // Adjusting for maximum of TODOOOOO

                        color.x = baseColor.x + deltaRed * numOs;
                        color.y = baseColor.y * (1 - 0.2 * numOs);
                        color.z = baseColor.z * (1 - 0.2 * numOs);
                        color.w = baseColor.w;
                    }

                    // Checking for FIXMEEEE...
                    char* fixmeLoc = strstr(token.text, "FIXME");
                    if (fixmeLoc && (size_t)(fixmeLoc - token.text + 4) < token.text_len) {

                        size_t numEs = 0;
                        char* ptr = fixmeLoc + 5; // Start right after "FIXME"

                        // Count 'E's without crossing token boundary
                        while ((size_t)(ptr - token.text) < token.text_len && (*ptr == 'E' || *ptr == 'e')) {

                            numEs++;
                            ptr++;
                        }

                        Vec4f baseColor = themes[currentThemeIndex].fixme;
                        float deltaRed = (1.0f - baseColor.x) / 5;  // Adjusting for maximum of FIXMEEEE

                        color.x = baseColor.x + deltaRed * numEs;
                        color.y = baseColor.y * (1 - 0.2 * numEs);
                        color.z = baseColor.z * (1 - 0.2 * numEs);
                        color.w = baseColor.w;
                    }

                    // Checking for BUG...
                    char* bugLoc = strstr(token.text, "BUG");
                    if (bugLoc && (size_t)(bugLoc - token.text + 2) < token.text_len) {

                        color = themes[currentThemeIndex].bug;
                    }


                    // Checking for NOTE...
                    char* noteLoc = strstr(token.text, "NOTE");
                    if (noteLoc && (size_t)(noteLoc - token.text + 3) < token.text_len) {

                        color = themes[currentThemeIndex].note;
                    }

                    // Continue rendering with
                }
                break;


            case TOKEN_EQUALS:
                color = themes[currentThemeIndex].equals;
                break;

            case TOKEN_EXCLAMATION:
                color = themes[currentThemeIndex].exclamation;
                break;

            case TOKEN_NOT_EQUALS:
                color = themes[currentThemeIndex].not_equals;
                break;

            case TOKEN_EQUALS_EQUALS:
                color = themes[currentThemeIndex].equals_equals;
                break;


            case TOKEN_LESS_THAN:
                color = themes[currentThemeIndex].less_than;
                break;

            case TOKEN_GREATER_THAN:
                color = themes[currentThemeIndex].greater_than;
                break;
            case TOKEN_ARROW:
                color = themes[currentThemeIndex].arrow;
                break;

            case TOKEN_MINUS:
                color = themes[currentThemeIndex].minus;
                break;

            case TOKEN_PLUS:
                color = themes[currentThemeIndex].plus;
                break;

            case TOKEN_TRUE:
                color = themes[currentThemeIndex].truee;
                break;
            case TOKEN_FALSE:
                color = themes[currentThemeIndex].falsee;
                break;
            case TOKEN_OPEN_SQUARE:
                color = themes[currentThemeIndex].open_square;
                break;
            case TOKEN_CLOSE_SQUARE:
                color = themes[currentThemeIndex].close_square;
                break;
            case TOKEN_ARRAY_CONTENT:
                color = themes[currentThemeIndex].array_content;
                break;
            case TOKEN_BAD_SPELLCHECK:
                color = themes[currentThemeIndex].bug;
                break;
            case TOKEN_STRING:
                /* color = hex_to_vec4f(0x73c936ff); */
                color = themes[currentThemeIndex].string;
                break;
            case TOKEN_COLOR: // Added case for TOKEN_COLOR
                {
                    unsigned long long hex_value;
                    if(sscanf(token.text, "0x%llx", &hex_value) == 1) {
                        color = hex_to_vec4f((uint32_t)hex_value);
                    }
                }
                break;
            default:
                {}
            }


            free_glyph_atlas_render_line_sized(atlas, sr, token.text, token.text_len, &pos, color);
            // TODO: the max_line_len should be calculated based on what's visible on the screen right now
            if (max_line_len < pos.x) max_line_len = pos.x;
        }
        simple_renderer_flush(sr);
    }

    

    // WHITESPACES
    {
        if (showWhitespaces) {
            if (isWave) {
                simple_renderer_set_shader(sr, VERTEX_SHADER_WAVE, SHADER_FOR_COLOR);
            } else {
                simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
            }
            
            float squareSize = FREE_GLYPH_FONT_SIZE * 0.2;
            
            for (size_t i = 0; i < editor->lines.count; ++i) {
                Line line = editor->lines.items[i];
                Vec2f pos = { 0, -((float)i + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE };
                
                if (showLineNumbers) {
                    pos.x += lineNumberWidth;
                }
                
                for (size_t j = line.begin; j < line.end; ++j) {
                    if (editor->data.items[j] == ' ' || editor->data.items[j] == '\t') {
                        /* Vec4f whitespaceColor = vec4f(1, 0, 0, 1); // Red color for visibility */
                        
                        Vec4f backgroundColor = themes[currentThemeIndex].background;
                        Vec4f whitespaceColor;
                        
                        // Increase each RGB component by 70%, but not above 1
                        whitespaceColor.x = backgroundColor.x + 0.7 * (1 - backgroundColor.x);
                        whitespaceColor.y = backgroundColor.y + 0.7 * (1 - backgroundColor.y);
                        whitespaceColor.z = backgroundColor.z + 0.7 * (1 - backgroundColor.z);
                        
                        // Clamp values to max 1.0
                        whitespaceColor.x = whitespaceColor.x > 1 ? 1 : whitespaceColor.x;
                        whitespaceColor.y = whitespaceColor.y > 1 ? 1 : whitespaceColor.y;
                        whitespaceColor.z = whitespaceColor.z > 1 ? 1 : whitespaceColor.z;
                        
                        // Keep the alpha value the same
                        whitespaceColor.w = backgroundColor.w;
                        
                        // Measure the actual character width
                        Vec2f char_pos = pos;
                        char_pos.x += (j - line.begin) * squareSize; // Starting position for this character
                        free_glyph_atlas_measure_line_sized(atlas, editor->data.items + j, 1, &char_pos);
                        float char_width = char_pos.x - pos.x - (j - line.begin) * squareSize;
                        
                        Vec2f rectPos = {pos.x + (j - line.begin) * char_width + (char_width - squareSize) / 2, pos.y + (FREE_GLYPH_FONT_SIZE - squareSize) / 2};
                        simple_renderer_solid_rect(sr, rectPos, vec2f(squareSize, squareSize), whitespaceColor);
                    }
                }
            }
            simple_renderer_flush(sr);
        }
    }
    
    
    // Render minibuffer
    {
        if (showMinibuffer) {
            simple_renderer_set_shader(sr, VERTEX_SHADER_FIXED, SHADER_FOR_COLOR);
            simple_renderer_solid_rect(sr, (Vec2f){0.0f, 0.0f}, (Vec2f){1920, 21.0f}, CURRENT_THEME.minibuffer);
            simple_renderer_flush(sr);
        }
    }


    // Render cursor
    if(editor->searching){
        simple_renderer_set_shader(sr, VERTEX_SHADER_FIXED, SHADER_FOR_COLOR);
    }else if (isWave){
        simple_renderer_set_shader(sr, VERTEX_SHADER_WAVE, SHADER_FOR_COLOR);
    }else{
        simple_renderer_set_shader(sr, VERTEX_SHADER_SIMPLE, SHADER_FOR_COLOR);
    }

    {
        // Adjust cursor position if line numbers are shown
        if (showLineNumbers) {
            cursor_pos.x += lineNumberWidth;
        }

        // Constants and Default Settings
        float CURSOR_WIDTH;
        const Uint32 CURSOR_BLINK_THRESHOLD = 500;
        const Uint32 CURSOR_BLINK_PERIOD = 1000;
        const Uint32 t = SDL_GetTicks() - editor->last_stroke;
        Vec4f CURSOR_COLOR = themes[currentThemeIndex].cursor;
        float BORDER_THICKNESS = 3.0f;
        Vec4f INNER_COLOR = vec4f(CURSOR_COLOR.x, CURSOR_COLOR.y, CURSOR_COLOR.z, 0.3);

        sr->verticies_count = 0;

        // If editor has a mark, make the cursor transparent
        if (editor->has_mark) {
            CURSOR_COLOR.w = 0.0f; // Set alpha to 0 (fully transparent)
        }

        // Rendering based on mode
        switch (current_mode) {

        case NORMAL: {
            float cursor_width;
            // Check if the cursor is on an actual character or an empty line
            if (editor->cursor < editor->data.count &&
                editor->data.items[editor->cursor] != '\n') {
                    Vec2f next_char_pos = cursor_pos;
                    free_glyph_atlas_measure_line_sized(
                        atlas, editor->data.items + editor->cursor,
                        1, // Measure the actual character at the cursor
                        &next_char_pos);
                    cursor_width = next_char_pos.x - cursor_pos.x;
            } else {
                    cursor_width = whitespace_width;
            }

            simple_renderer_solid_rect(
                sr, cursor_pos, vec2f(cursor_width, FREE_GLYPH_FONT_SIZE),
                CURSOR_COLOR);
        } break;

        case EMACS: {
            float cursor_width;
            CURSOR_COLOR = themes[currentThemeIndex].emacs_cursor;
            // Check if the cursor is on an actual character or an empty line
            if (editor->cursor < editor->data.count &&
                editor->data.items[editor->cursor] != '\n') {
                Vec2f next_char_pos = cursor_pos;
                free_glyph_atlas_measure_line_sized(
                                                    atlas, editor->data.items + editor->cursor,
                                                    1, // Measure the actual character at the cursor
                                                    &next_char_pos);
                cursor_width = next_char_pos.x - cursor_pos.x;
            } else {
                cursor_width = whitespace_width;
            }
            
            // Implement blinking for EMACS mode
            if (t < CURSOR_BLINK_THRESHOLD ||
                (t / CURSOR_BLINK_PERIOD) % 2 != 0) {
                simple_renderer_solid_rect(sr, cursor_pos, vec2f(cursor_width, FREE_GLYPH_FONT_SIZE),
                                           CURSOR_COLOR);
            }
        } break;

            
        case INSERT:
            CURSOR_COLOR = themes[currentThemeIndex].insert_cursor;
            CURSOR_WIDTH = 5.0f; // Thin vertical line for INSERT mode
            // Implement blinking for INSERT mode
            if (t < CURSOR_BLINK_THRESHOLD ||
                (t / CURSOR_BLINK_PERIOD) % 2 != 0) {
                    simple_renderer_solid_rect(
                        sr, cursor_pos,
                        vec2f(CURSOR_WIDTH, FREE_GLYPH_FONT_SIZE),
                        CURSOR_COLOR);
            }
            break;

        case VISUAL: {
            float cursor_width;

            // Check if the cursor is on an actual character or an empty line
            if (editor->cursor < editor->data.count &&
                editor->data.items[editor->cursor] != '\n') {
                    Vec2f next_char_pos = cursor_pos;
                    free_glyph_atlas_measure_line_sized(
                        atlas, editor->data.items + editor->cursor, 1,
                        &next_char_pos);
                    cursor_width = next_char_pos.x - cursor_pos.x;
            } else {
                    Vec2f next_char_pos = cursor_pos;
                    free_glyph_atlas_measure_line_sized(atlas, "a", 1,
                                                        &next_char_pos);
                    cursor_width = next_char_pos.x - cursor_pos.x;
            }

            // Draw inner rectangle
            simple_renderer_solid_rect(
                sr,
                vec2f(cursor_pos.x + BORDER_THICKNESS,
                      cursor_pos.y + BORDER_THICKNESS),
                vec2f(cursor_width - 2 * BORDER_THICKNESS,
                      FREE_GLYPH_FONT_SIZE - 2 * BORDER_THICKNESS),
                INNER_COLOR);

            // Draw the outline (borders) using the theme's cursor color
            simple_renderer_solid_rect(sr, cursor_pos,
                                       vec2f(cursor_width, BORDER_THICKNESS),
                                       CURSOR_COLOR); // Top border
            simple_renderer_solid_rect(
                sr,
                vec2f(cursor_pos.x,
                      cursor_pos.y + FREE_GLYPH_FONT_SIZE - BORDER_THICKNESS),
                vec2f(cursor_width, BORDER_THICKNESS),
                CURSOR_COLOR); // Bottom border
            simple_renderer_solid_rect(
                sr, cursor_pos, vec2f(BORDER_THICKNESS, FREE_GLYPH_FONT_SIZE),
                CURSOR_COLOR); // Left border
            simple_renderer_solid_rect(
                sr,
                vec2f(cursor_pos.x + cursor_width - BORDER_THICKNESS,
                      cursor_pos.y),
                vec2f(BORDER_THICKNESS, FREE_GLYPH_FONT_SIZE),
                CURSOR_COLOR); // Right border

            break;
        }

        case VISUAL_LINE:
            // Set the cursor width to cover the entire height of the line
            CURSOR_WIDTH = FREE_GLYPH_FONT_SIZE;

            // Adjust cursor color for visual distinction. For instance, make it
            // slightly transparent
            Vec4f TRANSPARENT_CURSOR_COLOR =
                vec4f(CURSOR_COLOR.x, CURSOR_COLOR.y, CURSOR_COLOR.z,
                      0.5f); // 50% transparency

            // Render the cursor for the entire line
            simple_renderer_solid_rect(
                sr, cursor_pos, vec2f(CURSOR_WIDTH, FREE_GLYPH_FONT_SIZE),
                TRANSPARENT_CURSOR_COLOR);

            // If you'd like to add additional visual cues, consider adding a
            // border or some other distinguishing feature.
            break;
        }
        simple_renderer_flush(sr);
    }

    // Update camera
    {
        if (isAnimated) {

            if (max_line_len > 1000.0f) {
                max_line_len = 1000.0f;
            }

            float target_scale = w / zoom_factor / (max_line_len * 0.75); // TODO: division by 0

            Vec2f target = cursor_pos;
            float offset = 0.0f;

            if (target_scale > 3.0f) {
                target_scale = 3.0f;
            } else {
                offset = cursor_pos.x - w/3/sr->camera_scale;
                if (offset < 0.0f) offset = 0.0f;
                target = vec2f(w/3/sr->camera_scale + offset, cursor_pos.y);
            }

            sr->camera_vel = vec2f_mul(
                                       vec2f_sub(target, sr->camera_pos),
                                       vec2fs(2.0f));
            sr->camera_scale_vel = (target_scale - sr->camera_scale) * 2.0f;

            sr->camera_pos = vec2f_add(sr->camera_pos, vec2f_mul(sr->camera_vel, vec2fs(DELTA_TIME)));
            sr->camera_scale = sr->camera_scale + sr->camera_scale_vel * DELTA_TIME;

        } else {
            sr->camera_scale = 0.24f;  // Set the zoom level to 0.24

            // Static flag to ensure initial camera position is set only once
            static bool hasSetInitialPosition = false;

            // If the initial position hasn't been set, set it now
            if (!hasSetInitialPosition) {
                sr->camera_pos.x = 3850.0f;  // Set the x-position
                sr->camera_pos.y = -2000.0f;  // Set the initial y-position
                hasSetInitialPosition = true;
            } else {
                // Calculate the vertical position of the cursor in world coordinates.
                int currentLine = editor_cursor_row(editor);
                float cursorPosY = -((float)currentLine + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE;

                // Define the top and bottom edges of the current camera view.
                float cameraTopEdge = sr->camera_pos.y - (h/2.0f) / sr->camera_scale;
                float cameraBottomEdge = sr->camera_pos.y + (h/2.0f) / sr->camera_scale;

                // Adjust the camera's Y position if the cursor is outside the viewport.
                if (cursorPosY > cameraBottomEdge) {
                    sr->camera_pos.y += cursorPosY - cameraBottomEdge;  // Move camera down just enough
                } else if (cursorPosY < cameraTopEdge) {
                    sr->camera_pos.y -= cameraTopEdge - cursorPosY;  // Move camera up just enough
                }

                // Keeping the x-position fixed as per the previous logic
                sr->camera_pos.x = 3850.0f;
            }
        }
    }
}

void editor_clipboard_copy(Editor *e)
{
    if (e->searching) return;
    if (e->selection) {
        size_t begin = e->select_begin;
        size_t end = e->cursor;
        if (begin > end) SWAP(size_t, begin, end);

        e->clipboard.count = 0;
        sb_append_buf(&e->clipboard, &e->data.items[begin], end - begin + 1);
        sb_append_null(&e->clipboard);

        if (SDL_SetClipboardText(e->clipboard.items) < 0) {
            fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
        }
    }
    copiedLine = false;
}

void editor_clipboard_paste(Editor *e)
{
    char *text = SDL_GetClipboardText();
    size_t text_len = strlen(text);
    if (text_len > 0) {
        editor_insert_buf(e, text, text_len);
    } else {
        fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
    }
    SDL_free(text);
}

void editor_cut_char_under_cursor(Editor *e) {
    if (e->searching) return;

    if (e->cursor >= e->data.count) return;

    // 1. Copy the character to clipboard.
    e->clipboard.count = 0;
    sb_append_buf(&e->clipboard, &e->data.items[e->cursor], 1);
    sb_append_null(&e->clipboard);
    if (SDL_SetClipboardText(e->clipboard.items) < 0) {
        fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
    }

    // 2. Delete the character from the editor.
    memmove(
        &e->data.items[e->cursor],
        &e->data.items[e->cursor + 1],
        (e->data.count - e->cursor - 1) * sizeof(e->data.items[0])
    );
    e->data.count -= 1;
    editor_retokenize(e);
}

// VISUAL selection TODO

void editor_start_visual_selection(Editor *e) {
    e->selection = true;

    // Identify the current line the cursor is on
    size_t cursor_row = editor_cursor_row(e);
    Line current_line = e->lines.items[cursor_row];

    // If in VISUAL_LINE mode, adjust the selection to span the entire line
    if (current_mode == VISUAL_LINE) {
        e->select_begin = current_line.begin;

        // Set the cursor to the end of the current line to span the whole line
        e->cursor = current_line.end;
    } else {
        e->select_begin = e->cursor;
    }
}

void editor_start_visual_line_selection(Editor *e) {
    e->selection = true;

    // Identify the current line the cursor is on
    size_t cursor_row = editor_cursor_row(e);
    Line current_line = e->lines.items[cursor_row];

    // Set the beginning and end of the selection to span the entire line
    e->select_begin = current_line.begin;
    e->cursor = current_line.end;
}

void editor_update_selection(Editor *e, bool shift) {
    if (e->searching) return;

    if (current_mode == VISUAL) {
        if (!e->selection) {
            editor_start_visual_selection(e);
        }
        // If you want the selection to end when you leave VISUAL mode,
        // you will need to handle that logic elsewhere (perhaps where mode changes are managed).
    } else if (shift) {
        if (!e->selection) {
            e->selection = true;
            e->select_begin = e->cursor;
        }
    } else {
        e->selection = false;
    }
}

// search
void editor_start_search(Editor *e)
{
    if (e->searching) {
        for (size_t pos = e->cursor + 1; pos < e->data.count; ++pos) {
            if (editor_search_matches_at(e, pos)) {
                e->cursor = pos;
                break;
            }
        }
    } else {
        e->searching = true;
        if (e->selection) {
            e->selection = false;
            // TODO: put the selection into the search automatically
        } else {
            e->search.count = 0;
        }
    }
}

void editor_stop_search(Editor *e)
{
    e->searching = false;
}

void editor_stop_search_and_mark(Editor *e) {
    e->searching = false;

    e->has_mark = true;  // Mark the search result.
    e->mark_start = e->cursor;
    e->mark_end = e->cursor + e->search.count;
}

void editor_clear_mark(Editor *editor) {
    editor->has_mark = false;
    editor->mark_start = 0;  // or some other appropriate default value
    editor->mark_end = 0;    // or some other appropriate default value
}



bool editor_search_matches_at(Editor *e, size_t pos)
{
    if (e->data.count - pos < e->search.count) return false;
    for (size_t i = 0; i < e->search.count; ++i) {
        if (e->search.items[i] != e->data.items[pos + i]) {
            return false;
        }
    }
    return true;
}

void editor_search_next(Editor *e) {
    size_t startPos = e->cursor + 1;
    for (size_t pos = startPos; pos < e->data.count; ++pos) {
        if (editor_search_matches_at(e, pos)) {
            e->cursor = pos;
            editor_stop_search_and_mark(e);
            return; // Exit after finding a match
        }
    }

    // If not found in the remainder of the text, wrap around to the beginning
    for (size_t pos = 0; pos < startPos; ++pos) {
        if (editor_search_matches_at(e, pos)) {
            e->cursor = pos;
            editor_stop_search_and_mark(e);
            return; // Exit after finding a match
        }
    }
}

void editor_search_previous(Editor *e) {
    if (e->cursor == 0) {
        // If we are at the beginning of the file, wrap around immediately
        for (size_t pos = e->data.count - 1; pos != SIZE_MAX; --pos) { // Note the loop condition
            if (editor_search_matches_at(e, pos)) {
                e->cursor = pos;
                editor_stop_search_and_mark(e);
                return; // Exit after finding a match
            }
        }
    } else {
        for (size_t pos = e->cursor - 1; pos != SIZE_MAX; --pos) { // Note the loop condition
            if (editor_search_matches_at(e, pos)) {
                e->cursor = pos;
                editor_stop_search_and_mark(e);
                return; // Exit after finding a match
            }
        }

        // If not found in the preceding text, wrap around to the end
        for (size_t pos = e->data.count - 1; pos > e->cursor; --pos) {
            if (editor_search_matches_at(e, pos)) {
                e->cursor = pos;
                editor_stop_search_and_mark(e);
                return; // Exit after finding a match
            }
        }
    }
}






void editor_move_to_begin(Editor *e)
{
    editor_stop_search(e);
    e->cursor = 0;
}

void editor_move_to_end(Editor *e)
{
    editor_stop_search(e);
    e->cursor = e->data.count;
}

void editor_move_to_line_begin(Editor *e)
{
    editor_stop_search(e);
    size_t row = editor_cursor_row(e);
    e->cursor = e->lines.items[row].begin;
}

void editor_move_to_line_end(Editor *e)
{
    editor_stop_search(e);
    size_t row = editor_cursor_row(e);
    e->cursor = e->lines.items[row].end;
}

void editor_move_paragraph_up(Editor *e)
{
    editor_stop_search(e);
    size_t row = editor_cursor_row(e);
    while (row > 0 && e->lines.items[row].end - e->lines.items[row].begin <= 1) {
        row -= 1;
    }
    while (row > 0 && e->lines.items[row].end - e->lines.items[row].begin > 1) {
        row -= 1;
    }
    e->cursor = e->lines.items[row].begin;
}

void editor_move_paragraph_down(Editor *e)
{
    editor_stop_search(e);
    size_t row = editor_cursor_row(e);
    while (row + 1 < e->lines.count && e->lines.items[row].end - e->lines.items[row].begin <= 1) {
        row += 1;
    }
    while (row + 1 < e->lines.count && e->lines.items[row].end - e->lines.items[row].begin > 1) {
        row += 1;
    }
    e->cursor = e->lines.items[row].begin;
}


void editor_kill_line(Editor *e) {
    if (e->searching || e->cursor >= e->data.count) return;

    size_t row = editor_cursor_row(e);
    size_t line_begin = e->lines.items[row].begin;
    size_t line_end = e->lines.items[row].end;

    // Check if the line is empty or if the cursor is at the end of the line
    if (line_begin == line_end || e->cursor == line_end) {
        // If the line is empty or the cursor is at the end of the line
        // Remove the newline character if it's not the first line
        if (row < e->lines.count - 1) {
            memmove(&e->data.items[line_begin], &e->data.items[line_end + 1], e->data.count - line_end - 1);
            e->data.count -= (line_end - line_begin + 1);
        } else if (row > 0 && e->data.items[line_begin - 1] == '\n') {
            // If it's the last line, remove the preceding newline character
            e->data.count -= 1;
            memmove(&e->data.items[line_begin - 1], &e->data.items[line_end], e->data.count - line_end);
        }
    } else {
        // If the line is not empty, kill the text from the cursor to the end of the line
        size_t length = line_end - e->cursor;

        // Copy the text to be killed to the clipboard
        e->clipboard.count = 0;
        sb_append_buf(&e->clipboard, &e->data.items[e->cursor], length);
        sb_append_null(&e->clipboard);
        if (SDL_SetClipboardText(e->clipboard.items) < 0) {
            fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
        }

        // Delete the range from the editor
        memmove(&e->data.items[e->cursor], &e->data.items[line_end], e->data.count - line_end);
        e->data.count -= length;
    }

    editor_retokenize(e);
}


void editor_backward_kill_word(Editor *e) {
    editor_stop_search(e);

    // Remember the start position of the deletion
    size_t start_pos = e->cursor;

    // Move cursor left to the start of the previous word
    while (e->cursor > 0 && !isalnum(e->data.items[e->cursor - 1])) {
        e->cursor -= 1;
    }
    while (e->cursor > 0 && isalnum(e->data.items[e->cursor - 1])) {
        e->cursor -= 1;
    }

    // Remember the end position of the deletion
    size_t end_pos = e->cursor;

    // Check if there is anything to delete
    if (start_pos > end_pos) {
        // Copy the deleted text to clipboard
        size_t length = start_pos - end_pos;
        e->clipboard.count = 0;
        sb_append_buf(&e->clipboard, &e->data.items[end_pos], length);
        sb_append_null(&e->clipboard);
        if (SDL_SetClipboardText(e->clipboard.items) < 0) {
            fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
        }

        // Perform the deletion
        memmove(&e->data.items[end_pos], &e->data.items[start_pos], e->data.count - start_pos);
        e->data.count -= length;
    }

    editor_retokenize(e);
}




// TODO when there is a {} dont add the space
void editor_join_lines(Editor *e) {
    size_t row = editor_cursor_row(e);
    if (row >= e->lines.count - 1) return; // Exit if on the last line

    // Get the current line and the next line
    size_t current_line_end = e->lines.items[row].end;
    size_t next_line_begin = e->lines.items[row + 1].begin;
    size_t next_line_end = e->lines.items[row + 1].end;


    // Check if the current line is empty or only has whitespaces
    bool only_whitespaces = true;
    for (size_t i = e->lines.items[row].begin; i < current_line_end; ++i) {
        if (!isspace(e->data.items[i])) {
            only_whitespaces = false;
            break;
        }
    }

    if (only_whitespaces) {
        // Current line is empty or has only whitespaces, delete the line
        size_t length_to_move = e->data.count - current_line_end;
        memmove(&e->data.items[e->lines.items[row].begin],
                &e->data.items[next_line_begin],
                length_to_move);
        e->data.count -= (next_line_begin - e->lines.items[row].begin);
        editor_retokenize(e);
        return;
    }

    // Check if the current line ends in a newline character
    if (e->data.items[current_line_end] == '\n') {
        // Skip leading spaces on the next line
        while (next_line_begin < next_line_end &&
               isspace(e->data.items[next_line_begin])) {
            next_line_begin++;
        }

        // Calculate the length to move in memmove
        size_t length_to_move = e->data.count - next_line_begin;

        // Move the data from the next line start to the current line end
        memmove(&e->data.items[current_line_end + 1],
                &e->data.items[next_line_begin],
                length_to_move);

        // Adjust the total count of characters in the buffer
        e->data.count -= (next_line_begin - current_line_end - 1);

        // Insert a single space to separate the lines
        e->data.items[current_line_end] = ' ';
    }

    editor_retokenize(e);
}



bool editor_is_line_empty(Editor *e, size_t row) {
    if (row >= e->lines.count) return true; // Non-existent lines are considered empty

    return e->lines.items[row].begin == e->lines.items[row].end;
}

bool editor_is_line_whitespaced(Editor *e, size_t row) {
    if (row >= e->lines.count) return false;

    size_t line_begin = e->lines.items[row].begin;
    size_t line_end = e->lines.items[row].end;

    for (size_t i = line_begin; i < line_end; ++i) {
        if (!isspace(e->data.items[i])) {
            return false;
        }
    }
    return true;
}



void editor_yank_line(Editor* editor) {
    size_t start = editor->cursor;
    while (start > 0 && editor->data.items[start - 1] != '\n') {
        start--;
    }

    size_t end = start;
    while (end < editor->data.count && editor->data.items[end] != '\n') {
        end++;
    }

    if (start < end) {
        editor->clipboard.count = 0;
        sb_append_buf(&editor->clipboard, &editor->data.items[start], end - start);
        sb_append_null(&editor->clipboard);

        if (SDL_SetClipboardText(editor->clipboard.items) < 0) {
            fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
        }
    }
    copiedLine = true;
}

void editor_paste_line_after(Editor* editor) {
    if (!copiedLine) {
        return; // Do nothing if no line has been copied
    }

    char *text = SDL_GetClipboardText();
    if (!text) {
        fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
        return;
    }

    size_t text_len = strlen(text);
    if (text_len > 0) {
        // Find the end of the current line
        size_t end = editor->cursor;
        while (end < editor->data.count && editor->data.items[end] != '\n') {
            end++;
        }

        // If not at the end of the file, move to the start of the next line
        if (end < editor->data.count) {
            end++;
        }

        // Insert the text from the clipboard
        editor_insert_buf_at(editor, text, text_len, end);

        // If the pasted text does not end with a newline, add one
        if (text[text_len - 1] != '\n') {
            editor_insert_buf_at(editor, "\n", 1, end + text_len);
        }

        // Move cursor to the first non-space character of the pasted line
        editor->cursor = end;
        while (editor->cursor < editor->data.count && editor->data.items[editor->cursor] == ' ') {
            editor->cursor++;
        }
    }

    SDL_free(text);
}

void editor_paste_line_before(Editor* editor) {
    if (!copiedLine) {
        return; // Do nothing if no line has been copied
    }

    char *text = SDL_GetClipboardText();
    if (!text) {
        fprintf(stderr, "ERROR: SDL ERROR: %s\n", SDL_GetError());
        return;
    }

    size_t text_len = strlen(text);
    if (text_len > 0) {
        // Find the start of the current line
        size_t start = editor->cursor;
        while (start > 0 && editor->data.items[start - 1] != '\n') {
            start--;
        }

        // Insert the text from the clipboard at the start of the line
        editor_insert_buf_at(editor, text, text_len, start);

        // Optionally, insert a newline after pasting if the text doesn't end with one
        if (text[text_len - 1] != '\n') {
            editor_insert_buf_at(editor, "\n", 1, start + text_len);
        }

        // Move cursor to the first non-space character of the pasted line
        editor->cursor = start;
        while (editor->cursor < editor->data.count && editor->data.items[editor->cursor] == ' ') {
            editor->cursor++;
        }
    }

    SDL_free(text);
}


ssize_t find_matching_parenthesis(Editor *editor, size_t cursor_pos) {
    // Ensure the cursor position is within the valid range
    if (cursor_pos >= editor->data.count) return -1;
    if (matchParenthesis){
        char current_char = editor->data.items[cursor_pos];
        char matching_char;
        int direction;
        
        // Check if the character at cursor is a parenthesis
        switch (current_char) {
        case '(': matching_char = ')'; direction = 1; break;
        case ')': matching_char = '('; direction = -1; break;
        case '[': matching_char = ']'; direction = 1; break;
        case ']': matching_char = '['; direction = -1; break;
        case '{': matching_char = '}'; direction = 1; break;
        case '}': matching_char = '{'; direction = -1; break;
        default: return -1; // Not on a parenthesis character
        }
        
        int balance = 1;
        size_t pos = cursor_pos;
        
        while ((direction > 0 && pos < editor->data.count - 1) || (direction < 0 && pos > 0)) {
            pos += direction;
            
            if (editor->data.items[pos] == current_char) {
                balance++;
            } else if (editor->data.items[pos] == matching_char) {
                balance--;
                if (balance == 0) {
                    return pos; // Found the matching parenthesis
                }
            }
        }
        return -1; // No matching parenthesis found
    }
}

size_t editor_row_from_pos(const Editor *e, size_t pos) {
    assert(e->lines.count > 0);
    for (size_t row = 0; row < e->lines.count; ++row) {
        Line line = e->lines.items[row];
        if (line.begin <= pos && pos <= line.end) {
            return row;
        }
    }
    return e->lines.count - 1;
}

void editor_jump_to_matching_parenthesis(Editor *editor) {
    if (editor->cursor >= editor->data.count) return;

    ssize_t matching_pos = find_matching_parenthesis(editor, editor->cursor);
    if (matching_pos != -1) {
        // Move the cursor to the matching parenthesis
        editor->cursor = matching_pos;
    }
}

void evil_jump_item(Editor *editor) {
    if (editor->cursor >= editor->data.count) return;

    char current_char = editor->data.items[editor->cursor];
    ssize_t matching_pos = -1;

    // Check if the current cursor position is a parenthesis
    if (strchr("()[]{}", current_char)) {
        matching_pos = find_matching_parenthesis(editor, editor->cursor);
    } else {
        // If not, search for a parenthesis on the current line
        size_t row = editor_cursor_row(editor);
        size_t line_begin = editor->lines.items[row].begin;
        size_t line_end = editor->lines.items[row].end;

        for (size_t pos = line_begin; pos < line_end; ++pos) {
            current_char = editor->data.items[pos];
            if (strchr("()[]{}", current_char)) {
                matching_pos = find_matching_parenthesis(editor, pos);
                if (matching_pos != -1) {
                    break;
                }
            }
        }
    }

    // Move the cursor to the matching parenthesis
    if (matching_pos != -1) {
        editor->cursor = matching_pos;
    }
}


//TODO BUG
void editor_enter(Editor *e) {
    if (e->searching) {
        editor_stop_search_and_mark(e);
        current_mode = NORMAL;
        return;
    }

    size_t row = editor_cursor_row(e);
    size_t line_end = e->lines.items[row].end;

    editor_insert_char(e, '\n');
    size_t line_begin = e->lines.items[row].begin;
    bool inside_braces = false;

    // Check if the line contains an opening brace '{'
    for (size_t i = line_begin; i < line_end; ++i) {
        char c = e->data.items[i];
        if (c == '{') {
            inside_braces = true;
            break;
        }
    }

    // Insert the same whitespace character
    for (size_t i = line_begin; i < line_end; ++i) {
        char c = e->data.items[i];
        if (c == ' ' || c == '\t') {
            editor_insert_char(e, c);
        } else {
            break;
        }
    }

    // If inside braces, perform additional steps
    if (inside_braces) {
        editor_move_line_up(e);
        editor_move_to_line_end(e);
        editor_insert_char(e, '\n');

        // Add indentation
        for (size_t i = 0; i < indentation; ++i) {
            editor_insert_char(e, ' ');
        }
    }

    e->last_stroke = SDL_GetTicks();
}


// Anchor Implementation: Initially, the anchor used a single index from the
// start of the buffer, requiring updates on text changes. To simplify, we now
// track two indices (start and end of buffer). The anchor position self-adjusts
// based on cursor's relative position, ensuring correct placement without
// modifying all text-manipulating functions.

void editor_set_anchor(Editor *editor) {
    if (editor->cursor < editor->data.count) {
        editor->has_anchor = true;
        editor->anchor_pos_from_start = editor->cursor;
        editor->anchor_pos_from_end = editor->data.count - editor->cursor;
    }
}

void editor_goto_anchor_and_clear(Editor *editor) {
    if (editor->has_anchor) {
        if (editor->cursor > editor->anchor_pos_from_start) {
            editor->cursor = editor->anchor_pos_from_start;
        } else {
            editor->cursor = editor->data.count - editor->anchor_pos_from_end;
        }
        editor->has_anchor = false;
    }
}

void editor_update_anchor(Editor *editor) {
    if (!editor->has_anchor) return;

    if (editor->cursor > editor->anchor_pos_from_start) {
        // Cursor is after the anchor, use position from the start
        editor->anchor_pos = editor->anchor_pos_from_start;
    } else {
        // Cursor is before the anchor, use position from the end
        editor->anchor_pos = editor->data.count - editor->anchor_pos_from_end;
    }
}

void editor_drag_line_down(Editor *editor) {
    size_t row = editor_cursor_row(editor);
    if (row >= editor->lines.count - 1) return; // Can't move the last line down

    Line current_line = editor->lines.items[row];
    Line next_line = editor->lines.items[row + 1];

    // Calculate lengths including the newline character
    size_t current_line_length = current_line.end - current_line.begin + 1;
    size_t next_line_length = next_line.end - next_line.begin + 1;

    // Allocate temporary buffer to hold the lines
    char *temp = malloc(current_line_length + next_line_length);
    if (!temp) {
        // Handle memory allocation error
        fprintf(stderr, "ERROR: Unable to allocate memory for line swapping.\n");
        return;
    }

    // Copy current and next lines into temp
    memcpy(temp, &editor->data.items[current_line.begin], current_line_length);
    memcpy(temp + current_line_length, &editor->data.items[next_line.begin], next_line_length);

    // Swap lines in editor's data
    memcpy(&editor->data.items[current_line.begin], temp + current_line_length, next_line_length);
    memcpy(&editor->data.items[current_line.begin + next_line_length], temp, current_line_length);

    // Free the temporary buffer
    free(temp);

    // Update cursor position
    if (editor->cursor >= current_line.begin && editor->cursor < current_line.end) {
        // The cursor is on the current line, move it down with the line
        editor->cursor += next_line_length;
    } else if (editor->cursor >= next_line.begin && editor->cursor <= next_line.end) {
        // The cursor is on the next line, move it up to the start of the current line
        editor->cursor = current_line.begin + (editor->cursor - next_line.begin);
    }

    // Update line positions in the Lines struct
    editor->lines.items[row].begin = current_line.begin;
    editor->lines.items[row].end = current_line.begin + next_line_length - 1;
    editor->lines.items[row + 1].begin = current_line.begin + next_line_length;
    editor->lines.items[row + 1].end = editor->lines.items[row + 1].begin + current_line_length - 1;

    // Retokenize
    editor_retokenize(editor);
}

void editor_drag_line_up(Editor *editor) {
    size_t row = editor_cursor_row(editor);
    if (row == 0) return; // Can't move the first line up

    Line current_line = editor->lines.items[row];
    Line previous_line = editor->lines.items[row - 1];

    // Calculate lengths including the newline character
    size_t current_line_length = current_line.end - current_line.begin + 1;
    size_t previous_line_length = previous_line.end - previous_line.begin + 1;

    // Allocate temporary buffer to hold the lines
    char *temp = malloc(current_line_length + previous_line_length);
    if (!temp) {
        // Handle memory allocation error
        fprintf(stderr, "ERROR: Unable to allocate memory for line swapping.\n");
        return;
    }

    // Copy current and previous lines into temp
    memcpy(temp, &editor->data.items[previous_line.begin], previous_line_length);
    memcpy(temp + previous_line_length, &editor->data.items[current_line.begin], current_line_length);

    // Swap lines in editor's data
    memcpy(&editor->data.items[previous_line.begin], temp + previous_line_length, current_line_length);
    memcpy(&editor->data.items[previous_line.begin + current_line_length], temp, previous_line_length);

    // Free the temporary buffer
    free(temp);

    // Update cursor position
    editor->cursor = previous_line.begin + (editor->cursor - current_line.begin);

    // Update line positions in the Lines struct
    editor->lines.items[row - 1].begin = previous_line.begin;
    editor->lines.items[row - 1].end = previous_line.begin + current_line_length - 1;
    editor->lines.items[row].begin = previous_line.begin + current_line_length;
    editor->lines.items[row].end = editor->lines.items[row].begin + previous_line_length - 1;

    // Retokenize
    editor_retokenize(editor);
}



// BUFFERS
// TODO switching buffers delete unsaved changes
// TODO save cursor position on each buffer 

void editor_add_to_buffer_history(Editor *e, const char *file_path) {
    if (e->buffer_history_count < MAX_BUFFER_HISTORY) {
        free(e->buffer_history[e->buffer_history_count]); // Free existing string if any
        e->buffer_history[e->buffer_history_count] = strdup(file_path);
    }
    e->buffer_index = e->buffer_history_count; // Update buffer index
    e->buffer_history_count++;
}


void editor_remove_from_buffer_history(Editor *e) {
    if (e->buffer_history_count > 0) {
        free(e->buffer_history[--e->buffer_history_count]); // Free the last string
    }
}


Errno editor_open_buffer(Editor *e, const char *file_path) {
    printf("Opening buffer: %s\n", file_path);

    e->data.count = 0;
    Errno err = read_entire_file(file_path, &e->data);
    if (err != 0) return err;

    e->cursor = 0;
    editor_retokenize(e);

    e->file_path.count = 0;
    sb_append_cstr(&e->file_path, file_path);
    sb_append_null(&e->file_path);

    return 0;
}

void editor_kill_buffer(Editor *e) {
    if (e->buffer_history_count > 0) {
        // Free the current buffer path and remove it from the history
        free(e->buffer_history[e->buffer_index]);
        e->buffer_history[e->buffer_index] = NULL;

        // Shift all elements after the current index down
        for (int i = e->buffer_index; i < e->buffer_history_count - 1; i++) {
            e->buffer_history[i] = e->buffer_history[i + 1];
        }

        // Decrease the count of buffers in the history
        e->buffer_history_count--;

        // Update the buffer index to point to the previous buffer, if possible
        if (e->buffer_index > 0) {
            e->buffer_index--;
        }

        // If there are still buffers in the history, load the previous one
        if (e->buffer_history_count > 0) {
            const char *prev_file_path = e->buffer_history[e->buffer_index];
            editor_open_buffer(e, prev_file_path); // Open the previous buffer without adding to history
        } else {
            // Handle the case when there are no more buffers in the history
            // For example open a scratch buffer
        }
    }
}


void editor_previous_buffer(Editor *e) {
    if (e->buffer_index > 0) {
        e->buffer_index--; // Move to the previous buffer in history
        const char *prev_file_path = e->buffer_history[e->buffer_index];
        editor_open_buffer(e, prev_file_path); // Open the previous buffer
    } else {
        // Handle case when there's no previous buffer
        printf("No previous buffer available.\n");
    }
}

void editor_next_buffer(Editor *e) {
    if (e->buffer_index < e->buffer_history_count - 1) {
        e->buffer_index++; // Move to the next buffer in history
        const char *next_file_path = e->buffer_history[e->buffer_index];
        editor_open_buffer(e, next_file_path); // Open the next buffer
    } else {
        // Handle case when there's no next buffer
        printf("No next buffer available.\n");
    }
}

