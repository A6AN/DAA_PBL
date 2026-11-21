#ifndef SIGNALDESK_MODELS_H
#define SIGNALDESK_MODELS_H

#define MAX_HEADLINE 256
#define MAX_BODY 8192
#define MAX_TAGS 10
#define MAX_TAG_LENGTH 64
#define MAX_NAME 128
#define MAX_DESCRIPTION 512

typedef enum {
    VERIFY_PENDING = 0,
    VERIFY_IN_PROGRESS = 1,
    VERIFY_VERIFIED = 2,
    VERIFY_REJECTED = 3
} VerificationStatus;

typedef enum {
    URGENCY_LOW = 0,
    URGENCY_MEDIUM = 1,
    URGENCY_HIGH = 2,
    URGENCY_CRITICAL = 3
} UrgencyBand;

/* The Article layout matches SPEC section 4, Module B. */
typedef struct {
    int id;
    char headline[MAX_HEADLINE];
    char body[MAX_BODY];
    int source_id;
    long long publication_time;
    char tags[MAX_TAGS][64];
    int tag_count;
    int event_id;
    double urgency_score;
    int verification_status;
} Article;

typedef struct {
    int id;
    char name[MAX_NAME];
    char description[MAX_DESCRIPTION];
} Source;

typedef struct {
    int id;
    char name[MAX_NAME];
    char description[MAX_DESCRIPTION];
    long long first_seen_time;
    UrgencyBand urgency_band;
} Event;

typedef struct {
    int id;
    char name[MAX_NAME];
    char expertise_tags[MAX_TAGS][MAX_TAG_LENGTH];
    int expertise_count;
    int available_capacity;
    int assigned_workload;
} FactChecker;

typedef struct {
    int id;
    int article_id;
    char required_expertise[MAX_TAG_LENGTH];
    double verification_priority;
    int estimated_work;
    VerificationStatus status;
} FactCheckTask;

#endif
