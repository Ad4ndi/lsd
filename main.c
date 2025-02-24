#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

typedef unsigned long long u64;

size_t parse_size(const char *s) {
    return strtoull(s, NULL, 10) * (s[strlen(s)-1] == 'K' ? 1024 : s[strlen(s)-1] == 'M' ? 1024*1024 : s[strlen(s)-1] == 'G' ? 1024*1024*1024 : 1); }

void apply_conv(void *buf, size_t *size, const char *conv, size_t cbs) {
    for (const char *p = conv; *p; p++) {
        if (!strncmp(p, "lcase", 5)) for (size_t i = 0; i < *size; i++) ((char*)buf)[i] = tolower(((char*)buf)[i]); 
        if (!strncmp(p, "ucase", 5)) for (size_t i = 0; i < *size; i++) ((char*)buf)[i] = toupper(((char*)buf)[i]); 
        if (!strncmp(p, "swab", 4)) {
            for (size_t i = 0; i + 1 < *size; i += 2) { 
                char t = ((char*)buf)[i];
                ((char*)buf)[i] = ((char*)buf)[i + 1];
                ((char*)buf)[i + 1] = t; } }
        if (!strncmp(p, "sync", 4)) {
            if (*size < cbs) {
                memset((char*)buf + *size, '\0', cbs - *size);
                *size = cbs; } }
        if (!strncmp(p, "block", 5)) {
            if (*size < cbs) {
                memset((char*)buf + *size, ' ', cbs - *size);
                *size = cbs; } }
        if (!strncmp(p, "unblock", 7)) {
            size_t i;
            for (i = *size; i > 0 && ((char*)buf)[i-1] == ' '; i--) ((char*)buf)[i-1] = '\n';
            *size = i; } } }

void copy(FILE *in, FILE *out, size_t size, size_t count, size_t skip, size_t seek, size_t cbs, const char *conv) {
    fseek(in, skip * size, SEEK_SET);
    fseek(out, seek * size, SEEK_SET);
    
    void *buf = malloc(size);
    if (!buf) {
        perror("Memory allocation failed");
        fclose(in);
        fclose(out);
        exit(1); }

    static int copy_started = 0;

    for (size_t i = 0; buf && (count == 0 || i < count); i++) {
        size_t bytes_read = fread(buf, 1, size, in);
        
        if (bytes_read == 0) {
            if (feof(in)) break;
            perror("[ERROR] Error reading file");
            free(buf);
            fclose(in);
            fclose(out);
            exit(1); }

        if (!copy_started) {
            printf("[LOG] Start copy\n");
            copy_started = 1; }

        apply_conv(buf, &bytes_read, conv, cbs);

        size_t bytes_written = fwrite(buf, 1, bytes_read, out);
        if (bytes_written != bytes_read) {
            perror("[ERROR] Write error");
            free(buf);
            fclose(in);
            fclose(out);
            exit(1); }
        
        printf("[LOG] Block %zu: %zuB read, %zuB written\n", i, bytes_read, bytes_written); }
    free(buf);
    printf("[LOG] Copy completed\n"); }

void help() {
    printf("Usage: lsd [options]\n");
    printf("  in=FILE          input file\n");
    printf("  out=FILE         output file\n");
    printf("  size=SIZE        block size\n");
    printf("  count=COUNT      number of blocks to copy\n");
    printf("  skip=SKIP        skip blocks at start\n");
    printf("  seek=SEEK        skip blocks at start of output\n");
    printf("  cbs=SIZE         conversion block size\n");
    printf("  conv=CONV        conversion types (lcase, ucase, swab, sync, block, unblock)\n"); }

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
        else if (!strncmp(v[i], "conv=", 5)) conv = v[i] + 5; }

    if (!in || !out) { help(); return 1; }
    FILE *fin = fopen(in, "rb");
    if (!fin) { perror("Error opening input file"); return 1; }
    FILE *fout = fopen(out, "wb");
    if (!fout) { perror("Error opening output file"); fclose(fin); return 1; }

    copy(fin, fout, size, count, skip, seek, cbs, conv);
    fclose(fin);
    fclose(fout);
    return 0; }
