#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char topic[51];
    unsigned char tip_date;
    char continut[1501];
} udp_message;

char** split_topic(const char* topic, int* count) {
    if (topic == NULL || count == NULL) {
        return NULL;
    }

    *count = 0;
    char **result = malloc(sizeof(char*));
    const char *start = topic;
    const char *end;

    while ((end = strchr(start, '/')) != NULL) {
        result[*count] = strndup(start, end - start);
        start = end + 1;
        (*count)++;
        result = realloc(result, (*count + 1) * sizeof(char*));
    }

    if (*start != '\0') {
        result[*count] = strdup(start);
        (*count)++;
    }

    return result;
}

int finite_state(char *str1, char *str2) {
    int count1 = 0, count2 = 0;
    char **segments1 = split_topic(str1, &count1);
    char **segments2 = split_topic(str2, &count2);

    if (!segments1 || !segments2) {  // Handle memory allocation failure
        fprintf(stderr, "Failed to allocate memory for topic segmentation.\n");
        return 0;
    }

    int i = 0, j = 0;
    while (i < count1 && j < count2) {
        
        if (strcmp(segments1[i], segments2[j]) == 0 || strcmp(segments1[i], "+") == 0) {
            i++;
            j++;
            continue;
        } else if (strcmp(segments1[i], "*") == 0) {
            if (i == count1 - 1) {  // If '*' is the last segment
                j = count2;  // Match all remaining segments
                i = count1;
                break;
            } else {
                //print segments to be compared and j
                printf("i: %s, j: %s\n", segments1[i + 1], segments2[j]);
                //values of i and j
                printf("i: %d, j: %d\n", i, j);
                while (j < count2 - 1 && strcmp(segments1[i + 1], segments2[j]) != 0) {
                    printf("muie");
                    j++;
                }
                i++;
            }
        } else {
            printf("muie");
            break;
        }
    }

    int match = (i == count1 && j == count2);

    // Free allocated memory
    for (int k = 0; k < count1; k++) free(segments1[k]);
    for (int k = 0; k < count2; k++) free(segments2[k]);
    free(segments1);
    free(segments2);

    return match;
}

void test_finite_state() {
    struct {
        char *pattern;
        char *topic;
        int expected;
    } cases[] = {
        {"upb/precis/*", "upb/precis/elevator/1/floor", 1},
        {"upb/precis/*", "upb/precis/100/200/temperature", 1},
        {"upb/*/temperature", "upb/precis/100/temperature", 0},
        {"upb/+/100/temperature", "upb/precis/100/temperature", 1},
        {"upb/*", "upb/precis/100", 1},
        {NULL, NULL, 0}
    };

    for (int i = 0; cases[i].pattern != NULL; i++) {
        int result = finite_state(cases[i].pattern, cases[i].topic);
        printf("Test %d: Pattern '%s' with Topic '%s' => %s (Expected %s)\n",
               i+1, cases[i].pattern, cases[i].topic,
               result ? "Match" : "No match",
               cases[i].expected ? "Match" : "No match");
    }
}

int main() {
    test_finite_state();
    return 0;
}
