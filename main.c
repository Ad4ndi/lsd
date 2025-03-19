#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

size_t parse_size(const char *s) {
    size_t factor = 1;
    size_t len = strlen(s);
    if (s[len - 1] == 'K') factor = 1024;
    else if (s[len - 1] == 'M') factor = 1024 * 1024;
    else if (s[len - 1] == 'G') factor = 1024 * 1024 * 1024;
    return strtoull(s, NULL, 10) * factor;
}

void apply_conv(void *buf, size_t *size, const char *conv, size_t cbs) {
    for (const char *p = conv; *p; p++) {
        if (!strncmp(p, "lcase", 5)) for (size_t i = 0; i < *size; i++) ((char*)buf)[i] = tolower(((char*)buf)[i]);
        if (!strncmp(p, "ucase", 5)) for (size_t i = 0; i < *size; i++) ((char*)buf)[i] = toupper(((char*)buf)[i]);
        if (!strncmp(p, "swab", 4)) for (size_t i = 0; i + 1 < *size; i += 2) {
            char t = ((char *)buf)[i]; ((char *)buf)[i] = ((char *)buf)[i + 1]; ((char *)buf)[i + 1] = t;
        }
        if (!strncmp(p, "sync", 4) && *size < cbs) {
            memset((char*)buf + *size, '\0', cbs - *size);
            *size = cbs;
        }
        if (!strncmp(p, "block", 5) && *size < cbs) {
            memset((char*)buf + *size, ' ', cbs - *size);
            *size = cbs;
        }
        if (!strncmp(p, "unblock", 7)) {
            size_t i;
            for (i = *size; i > 0 && ((char*)buf)[i - 1] == ' '; i--) ((char*)buf)[i - 1] = '\n';
            *size = i;
        }
    }
}

void copy(FILE *in, FILE *out, size_t size, size_t count, size_t skip, size_t seek, size_t cbs, const char *conv) {
    fseek(in, skip * size, SEEK_SET);
    fseek(out, seek * size, SEEK_SET);

    void *buf = malloc(size);
    if (!buf) { perror("Memory allocation failed"); exit(1); }

    for (size_t i = 0; buf && (count == 0 || i < count); i++) {
        size_t bytes_read = fread(buf, 1, size, in);
        if (bytes_read == 0) break;

        apply_conv(buf, &bytes_read, conv, cbs);
        size_t bytes_written = fwrite(buf, 1, bytes_read, out);
        if (bytes_written != bytes_read) { perror("Write error"); exit(1); }
    }
    free(buf);
}

void help() {
    printf("Usage: lsd [options]\n"
           "  in=FILE          input file\n"
           "  out=FILE         output file\n"
           "  size=SIZE        block size\n"
           "  count=COUNT      number of blocks to copy\n"
           "  skip=SKIP        skip blocks at start\n"
           "  seek=SEEK        skip blocks at start of output\n"
           "  cbs=SIZE         conversion block size\n"
           "  conv=CONV        conversion types (lcase, ucase, swab, sync, block, unblock)\n");
}

int main(int c, char *v[]) {
    if (c == 1) { help(); return 0; }
    char *in = NULL, *out = NULL, *conv = "";
    size_t size = 4096, count = 0, skip = 0, seek = 0, cbs = 0;

    for (int i = 1; i < c; i++) {
        if (!strncmp(v[i], "in=", 3)) in = v[i] + 3;
        else if (!strncmp(v[i], "out=", 4)) out = v[i] + 4;
        else if (!strncmp(v[i], "size=", 5)) size = parse_size(v[i] + 5);
        else if (!strncmp(v[i], "count=", 6)) count = strtoull(v[i] + 6, NULL, 10);
        else if (!strncmp(v[i], "skip=", 5)) skip = strtoull(v[i] + 5, NULL, 10);
        else if (!strncmp(v[i], "seek=", 5)) seek = strtoull(v[i] + 5, NULL, 10);
        else if (!strncmp(v[i], "cbs=", 4)) cbs = parse_size(v[i] + 4);
        else if (!strncmp(v[i], "conv=", 5)) conv = v[i] + 5;
    }

    if (!in || !out) { help(); return 1; }
    FILE *fin = fopen(in, "rb");
    if (!fin) { perror("Error opening input file"); return 1; }
    FILE *fout = fopen(out, "wb");
    if (!fout) { perror("Error opening output file"); fclose(fin); return 1; }

    copy(fin, fout, size, count, skip, seek, cbs, conv);
    fclose(fin);
    fclose(fout);
    return 0;
}
