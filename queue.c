#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list_sort.h"
#include "queue.h"

typedef void (*sort_f)(struct list_head *, bool descend);
typedef void (*queue_op)(struct list_head *head);
extern sort_f m_sort;
extern queue_op reverse_op;

#define list_for_each_safe_reverse(node, safe, head)         \
    for (node = head->prev, safe = node->prev; node != head; \
         node = safe, safe = node->prev)

static void list_swap(struct list_head *e1, struct list_head *e2)
{
    struct list_head *prev = e2->prev;
    list_move(e2, e1);
    if (prev != e1)
        list_move(e1, prev);
}

static int asc_cmp(void *priv,
                   const struct list_head *l1,
                   const struct list_head *l2)
{
    const char *s1 = list_entry(l1, element_t, list)->value;
    const char *s2 = list_entry(l2, element_t, list)->value;
    return strcmp(s1, s2);
}

static int dec_cmp(void *priv,
                   const struct list_head *l1,
                   const struct list_head *l2)
{
    return asc_cmp(priv, l2, l1);
}

static list_cmp_func_t sort_cmp[] = {
    [true] = dec_cmp,
    [false] = asc_cmp,
};

static inline element_t *q_remove(struct list_head *node,
                                  char *sp,
                                  size_t bufsize)
{
    if (list_empty(node))
        return NULL;

    element_t *el = list_entry(node, element_t, list);
    list_del(node);
    if (sp && el->value) {
        size_t len = strlen(el->value) + 1;
        len = len < bufsize ? len : bufsize;
        strncpy(sp, el->value, len);
        sp[len - 1] = '\0';
    }
    return el;
}

static inline bool q_insert(struct list_head *head, char *s)
{
    element_t *el = malloc(sizeof(element_t));
    if (!el)
        return false;

    el->value = strdup(s);
    if (!el->value) {
        free(el);
        return false;
    }
    list_add(&el->list, head);

    return true;
}

static void merge_two_lists(struct list_head *l1,
                            struct list_head *l2,
                            struct list_head *sorted,
                            bool descend)
{
    struct list_head *next, **node;
    list_cmp_func_t cmp = sort_cmp[descend];

    for (node = NULL; !list_empty(l1) && !list_empty(l2); *node = next) {
        node = cmp(NULL, l1->next, l2->next) <= 0 ? &l1->next : &l2->next;
        next = (*node)->next;
        list_move_tail(*node, sorted);
    }

    list_splice_tail_init(list_empty(l1) ? l2 : l1, sorted);
}

/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *que = malloc(sizeof(struct list_head));
    if (que)
        INIT_LIST_HEAD(que);

    return que;
}

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    if (!head)
        return;
    element_t *e, *s;
    list_for_each_entry_safe(e, s, head, list)
        q_release_element(e);
    free(head);
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    return q_insert(head, s);
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    return q_insert(head->prev, s);
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    return q_remove(head->next, sp, bufsize);
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    return q_remove(head->prev, sp, bufsize);
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    int sz = 0;
    struct list_head *node;
    list_for_each(node, head)
        sz++;
    return sz;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    struct list_head *fast = head->next, *node;
    list_for_each(node, head) {
        if (fast->next == head || fast->next->next == head)
            break;
        fast = fast->next->next;
    }
    list_del(node);
    q_release_element(list_entry(node, element_t, list));
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    if (!head || list_empty(head))
        return false;

    element_t *cur, *next;
    bool dup = false;
    list_for_each_entry_safe(cur, next, head, list) {
        if (cur->list.next != head && !strcmp(cur->value, next->value)) {
            dup = true;
            list_del(&cur->list);
            q_release_element(cur);
        } else {
            if (dup) {
                dup = false;
                list_del(&cur->list);
                q_release_element(cur);
            }
        }
    }
    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    struct list_head *node;
    list_for_each(node, head) {
        if (node->next == head)
            break;
        list_move_tail(node->next, node);
    }
}

void q_swap_forward_pair(struct list_head *head)
{
    struct list_head *node;
    list_for_each(node, head) {
        if (node->next == head)
            break;
        list_swap(node->next, node);
    }
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    struct list_head *node, *safe;
    list_for_each_safe(node, safe, head)
        list_move(node, head);
}

void q_reverse_bidir(struct list_head *head)
{
    struct list_head *cur, *safe;
    struct list_head *last = head->prev, *last_safe;

    list_for_each_safe(cur, safe, head) {
        last_safe = last->prev;
        if (cur->next == last) {
            list_swap(cur, last);
            break;
        } else if (cur == last)
            break;

        list_swap(cur, last);
        last = last_safe;
    }
}

static void do_recur_reverse(struct list_head *node, struct list_head *head)
{
    if (node == head)
        return;

    struct list_head *safe = node->next;
    list_move(node, head);
    do_recur_reverse(safe, head);
}

void q_reverse_recur(struct list_head *head)
{
    do_recur_reverse(head->next, head);
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    if (!head || k <= 1)
        return;

    struct list_head *node, *safe, *anchor;
    LIST_HEAD(rev_head);
    anchor = head;
    int i = 0;

    list_for_each_safe(node, safe, head) {
        i++;
        if (i == k) {
            list_cut_position(&rev_head, anchor, node);
            reverse_op(&rev_head);
            list_splice_init(&rev_head, anchor);
            anchor = safe->prev;
            i = 0;
        }
    }
}

/* Sort elements of queue in ascending/descending order */
void q_sort(struct list_head *head, bool descend)
{
    if (list_empty(head) || list_is_singular(head))
        return;
    struct list_head *fast = head->next, *slow;
    list_for_each(slow, head) {
        if (fast->next == head || fast->next->next == head)
            break;
        fast = fast->next->next;
    }
    LIST_HEAD(sorted);
    LIST_HEAD(left);
    list_cut_position(&left, head, slow);
    q_sort(&left, descend);
    q_sort(head, descend);
    merge_two_lists(&left, head, &sorted, descend);
    list_splice_tail(&sorted, head);
}

/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    if (list_empty(head))
        return 0;

    const char *maxv = "\0";
    int sz = 0;
    struct list_head *cur, *safe;

    list_for_each_safe(cur, safe, head) {
        sz++;
        element_t *entry = list_entry(cur, element_t, list);
        if (strcmp(entry->value, maxv) > 0)
            maxv = entry->value;
        else {
            list_del(cur);
            q_release_element(entry);
            sz--;
        }
    }

    return sz;
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    if (list_empty(head))
        return 0;

    const char *maxv = "\0";
    int sz = 0;
    struct list_head *cur, *safe;

    list_for_each_safe_reverse(cur, safe, head)
    {
        sz++;
        element_t *entry = list_entry(cur, element_t, list);
        if (strcmp(entry->value, maxv) > 0)
            maxv = entry->value;
        else {
            list_del(cur);
            q_release_element(entry);
            sz--;
        }
    }

    return sz;
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
int q_merge(struct list_head *head, bool descend)
{
    if (!head)
        return 0;

    LIST_HEAD(sorted);
    queue_contex_t *cur = NULL;
    queue_contex_t *last = list_last_entry(head, queue_contex_t, chain);

    list_for_each_entry(cur, head, chain) {
        if (cur == last) {
            list_splice_init(cur->q, &sorted);
            break;
        }
        list_splice_init(cur->q, &sorted);
        list_splice_init(last->q, &sorted);
        last = list_entry(last->chain.prev, queue_contex_t, chain);
    }

    m_sort(&sorted, descend);
    int size = q_size(&sorted);
    list_splice_init(&sorted, list_first_entry(head, queue_contex_t, chain)->q);
    return size;
}

void q_ksort(struct list_head *head, bool descend)
{
    if (!head)
        return;

    list_cmp_func_t cmp = sort_cmp[descend];
    list_sort(NULL, head, cmp);
}

void q_shuffle(struct list_head *head)
{
    if (list_empty(head) || list_is_singular(head))
        return;

    struct list_head *node;
    LIST_HEAD(list);

    for (size_t len = q_size(head); len; len--) {
        int id = rand() % len;
        int i = 0;
        list_for_each(node, head) {
            if (i == id) {
                list_move_tail(node, &list);
                break;
            }
            i++;
        }
    }

    list_splice_tail(&list, head);
}

void q_shuffle_remain(struct list_head *head)
{
    if (list_empty(head) || list_is_singular(head))
        return;

    struct list_head *cur, *prev;
    struct list_head *chosen;
    LIST_HEAD(list);
    size_t len = q_size(head);


    for (cur = head->prev; len && cur->prev != head; len--) {
        prev = cur->prev;
        int id = rand() % len;
        int i = 0;

        list_for_each(chosen, head) {
            if (i == id)
                break;
            i++;
        }

        if (chosen == head || chosen == cur)
            continue;
        list_swap(cur, chosen);

        if (cur->next == chosen)
            continue;
        cur = prev;
    }
}
