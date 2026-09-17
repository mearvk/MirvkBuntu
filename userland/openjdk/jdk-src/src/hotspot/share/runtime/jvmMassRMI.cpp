/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * jvmMassRMI.cpp - Mass RMI Feature-Group Tree Implementation
 *
 * Native C implementation of the Mass RMI feature-group tree system.
 * Provides hierarchical feature-group creation, template installation,
 * guess encapsulation (antedelerior), child-guess installation (positorer),
 * symporetic scaffolding (main-guess), and assuredness confirmation
 * (post-guess).
 *
 * Invoked from Java via:
 *   System.mass.RMI("group-signature").install.template("feature-group").run.install("");
 *
 * Author: Maximilian Eric Alexander Rupplin von Keffikon
 * Date: August 8, 2026
 */

#include "jvmMassRMI.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ═══════════════════════════════════════════════════════════════════════
   Internal Helpers
   ═══════════════════════════════════════════════════════════════════════ */

static uint64_t rmi_next_id = 1;

static uint64_t rmi_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

static uint64_t rmi_alloc_id(void) {
    return rmi_next_id++;
}

static MassRMINode *rmi_pool_alloc(MassRMITree *tree) {
    if (tree->pool_used >= MASS_RMI_MAX_NODES) {
        fprintf(stderr, "[MassRMI] ERROR: Node pool exhausted (%d max)\n",
                MASS_RMI_MAX_NODES);
        return NULL;
    }
    MassRMINode *node = &tree->node_pool[tree->pool_used++];
    memset(node, 0, sizeof(MassRMINode));
    node->id = rmi_alloc_id();
    node->created_at = rmi_timestamp();
    node->modified_at = node->created_at;
    node->status = MASS_RMI_STATUS_CREATED;
    node->type = MASS_RMI_NODE_STANDARD;
    tree->total_nodes++;
    return node;
}

/* ═══════════════════════════════════════════════════════════════════════
   Config File Parsing (minimal XML — matches jvm-config.xml style)
   ═══════════════════════════════════════════════════════════════════════ */

/*
 * Simple config parser. Reads <mass-rmi> XML and populates the tree
 * root with initial structure. Full DTD is rejected for security
 * (consistent with xmlConfigReader policy).
 */
static int rmi_parse_config(MassRMITree *tree, const char *config_path) {
    FILE *f;
    char buf[4096];
    size_t total = 0;

    if (!config_path || config_path[0] == '\0') {
        /* No config — empty tree, that's valid */
        return 0;
    }

    f = fopen(config_path, "r");
    if (!f) {
        fprintf(stderr, "[MassRMI] WARNING: Config not found: %s (proceeding empty)\n",
                config_path);
        return 0;
    }

    /* Security: reject oversized configs */
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    if (fsize > MASS_RMI_MAX_CONFIG_SIZE) {
        fprintf(stderr, "[MassRMI] ERROR: Config too large (%ld > %d)\n",
                fsize, MASS_RMI_MAX_CONFIG_SIZE);
        fclose(f);
        return -1;
    }
    fseek(f, 0, SEEK_SET);

    /* Security: reject DTD/ENTITY (XXE prevention) */
    while (fgets(buf, sizeof(buf), f)) {
        total += strlen(buf);
        if (strstr(buf, "<!DOCTYPE") || strstr(buf, "<!ENTITY") ||
            strstr(buf, "SYSTEM")) {
            fprintf(stderr, "[MassRMI] ERROR: DTD/ENTITY/SYSTEM rejected (XXE prevention)\n");
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    strncpy(tree->config_path, config_path, sizeof(tree->config_path) - 1);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
   Core Operations
   ═══════════════════════════════════════════════════════════════════════ */

int mass_rmi_init(MassRMITree *tree, const char *config_path) {
    if (!tree) return -1;

    memset(tree, 0, sizeof(MassRMITree));
    tree->tree_id = rmi_alloc_id();
    tree->initialized = false;

    /* Parse config if provided */
    if (rmi_parse_config(tree, config_path) != 0) {
        return -1;
    }

    /* Create root node */
    tree->root = rmi_pool_alloc(tree);
    if (!tree->root) return -1;

    snprintf(tree->root->signature, MASS_RMI_MAX_SIGNATURE, "__root__");
    snprintf(tree->root->template_name, MASS_RMI_MAX_TEMPLATE, "root-template");
    tree->root->type = MASS_RMI_NODE_ROOT;
    tree->root->status = MASS_RMI_STATUS_RUNNING;
    tree->root->depth = 0;

    tree->initialized = true;
    return 0;
}

void mass_rmi_shutdown(MassRMITree *tree) {
    if (!tree) return;
    /* Pool is stack-allocated within tree struct — just mark as shutdown */
    tree->initialized = false;
    tree->total_nodes = 0;
    tree->pool_used = 0;
    tree->root = NULL;
}

MassRMINode *mass_rmi_create(MassRMITree *tree, const char *signature) {
    if (!tree || !tree->initialized || !signature) return NULL;

    MassRMINode *node = rmi_pool_alloc(tree);
    if (!node) return NULL;

    strncpy(node->signature, signature, MASS_RMI_MAX_SIGNATURE - 1);
    node->status = MASS_RMI_STATUS_CREATED;

    return node;
}

int mass_rmi_install_template(MassRMINode *node, const char *template_name) {
    if (!node || !template_name) return -1;
    if (node->status == MASS_RMI_STATUS_DESTROYED) return -1;

    strncpy(node->template_name, template_name, MASS_RMI_MAX_TEMPLATE - 1);
    node->status = MASS_RMI_STATUS_INSTALLED;
    node->modified_at = rmi_timestamp();

    return 0;
}

int mass_rmi_run(MassRMINode *node) {
    if (!node) return -1;
    if (node->status == MASS_RMI_STATUS_DESTROYED) return -1;

    /* Must be at least INSTALLED to run */
    if (node->status < MASS_RMI_STATUS_INSTALLED) {
        fprintf(stderr, "[MassRMI] ERROR: Cannot run node '%s' — not installed\n",
                node->signature);
        return -1;
    }

    node->status = MASS_RMI_STATUS_RUNNING;
    node->modified_at = rmi_timestamp();
    return 0;
}

int mass_rmi_add(MassRMINode *parent, MassRMINode *child) {
    if (!parent || !child) return -1;
    if (parent->status == MASS_RMI_STATUS_DESTROYED) return -1;
    if (parent->child_count >= MASS_RMI_MAX_CHILDREN) {
        fprintf(stderr, "[MassRMI] ERROR: Max children reached for '%s'\n",
                parent->signature);
        return -1;
    }

    child->parent = parent;
    child->depth = parent->depth + 1;
    parent->children[parent->child_count++] = child;
    parent->modified_at = rmi_timestamp();

    return 0;
}

int mass_rmi_remove(MassRMINode *node) {
    if (!node || !node->parent) return -1;

    MassRMINode *parent = node->parent;

    /* Find and remove from parent's children array */
    for (int i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == node) {
            /* Shift remaining children left */
            for (int j = i; j < parent->child_count - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->children[parent->child_count - 1] = NULL;
            parent->child_count--;
            break;
        }
    }

    node->parent = NULL;
    node->status = MASS_RMI_STATUS_DETACHED;
    node->modified_at = rmi_timestamp();

    return 0;
}

int mass_rmi_delete(MassRMITree *tree, MassRMINode *node) {
    if (!tree || !node) return -1;

    /* Remove from parent first */
    if (node->parent) {
        mass_rmi_remove(node);
    }

    node->status = MASS_RMI_STATUS_DESTROYED;
    node->modified_at = rmi_timestamp();
    tree->total_nodes--;

    return 0;
}

int mass_rmi_destroy(MassRMITree *tree, MassRMINode *node) {
    if (!tree || !node) return -1;

    /* Recursively destroy all children first */
    while (node->child_count > 0) {
        mass_rmi_destroy(tree, node->children[0]);
    }

    /* Now destroy self */
    return mass_rmi_delete(tree, node);
}

/* ═══════════════════════════════════════════════════════════════════════
   Navigation
   ═══════════════════════════════════════════════════════════════════════ */

MassRMINode *mass_rmi_anterior(MassRMINode *node) {
    if (!node) return NULL;
    return node->parent;  /* Parent is the anterior (prior) node */
}

MassRMINode *mass_rmi_superior(MassRMINode *node) {
    if (!node) return NULL;
    /* Walk up to root — the highest-authority node */
    MassRMINode *current = node;
    while (current->parent != NULL) {
        current = current->parent;
    }
    return current;
}

/* ═══════════════════════════════════════════════════════════════════════
   Guess System
   ═══════════════════════════════════════════════════════════════════════ */

int mass_rmi_antedelerior(MassRMINode *node, const char *guess_content,
                          int confidence) {
    if (!node || !guess_content) return -1;
    if (node->guess_count >= MASS_RMI_MAX_GUESSES) {
        fprintf(stderr, "[MassRMI] ERROR: Guess buffer full for '%s'\n",
                node->signature);
        return -1;
    }

    MassRMIGuess *g = &node->guesses[node->guess_count++];
    g->id = rmi_alloc_id();
    strncpy(g->content, guess_content, sizeof(g->content) - 1);
    g->timestamp = rmi_timestamp();
    g->confirmed = false;
    g->confidence = (confidence < 0) ? 0 : (confidence > 100) ? 100 : confidence;

    node->modified_at = rmi_timestamp();
    return 0;
}

int mass_rmi_positorer(MassRMITree *tree, MassRMINode *parent) {
    if (!tree || !parent) return -1;

    int installed = 0;

    /* Take each unconfirmed guess and install it as a child node */
    for (int i = 0; i < parent->guess_count; i++) {
        MassRMIGuess *g = &parent->guesses[i];
        if (g->confirmed) continue;  /* Already promoted */

        /* Create a child node from the guess */
        MassRMINode *child = rmi_pool_alloc(tree);
        if (!child) break;

        snprintf(child->signature, MASS_RMI_MAX_SIGNATURE,
                 "%s.guess.%lu", parent->signature, (unsigned long)g->id);
        snprintf(child->template_name, MASS_RMI_MAX_TEMPLATE,
                 "positorer-installed");
        child->type = MASS_RMI_NODE_POSITORER;
        child->status = MASS_RMI_STATUS_INSTALLED;

        /* Copy guess content into child's first guess slot as record */
        child->guesses[0] = *g;
        child->guess_count = 1;

        /* Attach to parent */
        mass_rmi_add(parent, child);
        installed++;
    }

    parent->modified_at = rmi_timestamp();
    return installed;
}

MassRMINode *mass_rmi_main_guess(MassRMITree *tree, MassRMINode *target,
                                  const char *symporetic_signature) {
    if (!tree || !target || !symporetic_signature) return NULL;

    /* Create the symporetic scaffold node */
    MassRMINode *scaffold = rmi_pool_alloc(tree);
    if (!scaffold) return NULL;

    strncpy(scaffold->signature, symporetic_signature,
            MASS_RMI_MAX_SIGNATURE - 1);
    snprintf(scaffold->template_name, MASS_RMI_MAX_TEMPLATE,
             "symporetic-scaffold");
    scaffold->type = MASS_RMI_NODE_SYMPORETIC;
    scaffold->status = MASS_RMI_STATUS_INSTALLED;

    /* Link the symporetic reference on the target */
    target->symporetic_ref = scaffold;

    /* Add as child of target for tree integrity */
    mass_rmi_add(target, scaffold);

    target->modified_at = rmi_timestamp();
    return scaffold;
}

int mass_rmi_post_guess(MassRMINode *node, const char *condition) {
    if (!node || !condition) return -1;

    /* Mark all unconfirmed guesses as confirmed */
    int confirmed = 0;
    for (int i = 0; i < node->guess_count; i++) {
        if (!node->guesses[i].confirmed) {
            node->guesses[i].confirmed = true;
            confirmed++;
        }
    }

    /* Install assuredness condition */
    node->assured = true;
    strncpy(node->assuredness_condition, condition,
            sizeof(node->assuredness_condition) - 1);
    node->assuredness_timestamp = rmi_timestamp();
    node->modified_at = rmi_timestamp();

    return confirmed;
}

/* ═══════════════════════════════════════════════════════════════════════
   Mapping — Feature-group composition
   ═══════════════════════════════════════════════════════════════════════ */

int mass_rmi_map(MassRMINode *target, MassRMINode *source) {
    if (!target || !source) return -1;
    if (source->status == MASS_RMI_STATUS_DESTROYED) return -1;

    /*
     * Mapping overlays source's children onto target.
     * Each child of source is added as a child of target.
     * The source node itself becomes a reference, not a copy.
     */
    for (int i = 0; i < source->child_count; i++) {
        MassRMINode *src_child = source->children[i];
        if (src_child->status == MASS_RMI_STATUS_DESTROYED) continue;

        /* Link (not copy) — the source child now also belongs to target */
        if (target->child_count < MASS_RMI_MAX_CHILDREN) {
            target->children[target->child_count++] = src_child;
        }
    }

    target->mapping_count++;
    target->modified_at = rmi_timestamp();
    return 0;
}

int mass_rmi_map_subpath(MassRMITree *tree, MassRMINode *target,
                         MassRMINode *source, const char *subpath) {
    if (!tree || !target || !source || !subpath) return -1;

    /* Find the subpath node within target */
    MassRMINode *sub = mass_rmi_find(tree, subpath);
    if (!sub) {
        fprintf(stderr, "[MassRMI] ERROR: Subpath '%s' not found\n", subpath);
        return -1;
    }

    return mass_rmi_map(sub, source);
}

/* ═══════════════════════════════════════════════════════════════════════
   Query
   ═══════════════════════════════════════════════════════════════════════ */

static MassRMINode *find_recursive(MassRMINode *node, const char *signature) {
    if (!node) return NULL;
    if (strcmp(node->signature, signature) == 0) return node;

    for (int i = 0; i < node->child_count; i++) {
        MassRMINode *found = find_recursive(node->children[i], signature);
        if (found) return found;
    }
    return NULL;
}

MassRMINode *mass_rmi_find(MassRMITree *tree, const char *signature) {
    if (!tree || !tree->root || !signature) return NULL;
    return find_recursive(tree->root, signature);
}

int mass_rmi_count(MassRMINode *node) {
    if (!node) return 0;
    int count = 1;
    for (int i = 0; i < node->child_count; i++) {
        count += mass_rmi_count(node->children[i]);
    }
    return count;
}

void mass_rmi_print_tree(MassRMINode *node, int indent) {
    if (!node) return;

    for (int i = 0; i < indent; i++) printf("  ");

    const char *type_str;
    switch (node->type) {
    case MASS_RMI_NODE_ROOT:        type_str = "ROOT"; break;
    case MASS_RMI_NODE_SYMPORETIC:  type_str = "SYMPORETIC"; break;
    case MASS_RMI_NODE_ASSUREDNESS: type_str = "ASSUREDNESS"; break;
    case MASS_RMI_NODE_GUESS:       type_str = "GUESS"; break;
    case MASS_RMI_NODE_POSITORER:   type_str = "POSITORER"; break;
    default:                        type_str = "STANDARD"; break;
    }

    const char *status_str;
    switch (node->status) {
    case MASS_RMI_STATUS_CREATED:   status_str = "CREATED"; break;
    case MASS_RMI_STATUS_INSTALLED: status_str = "INSTALLED"; break;
    case MASS_RMI_STATUS_RUNNING:   status_str = "RUNNING"; break;
    case MASS_RMI_STATUS_DETACHED:  status_str = "DETACHED"; break;
    case MASS_RMI_STATUS_DESTROYED: status_str = "DESTROYED"; break;
    default:                        status_str = "UNKNOWN"; break;
    }

    printf("[%s] %s (template=%s, status=%s, children=%d, guesses=%d%s)\n",
           type_str, node->signature, node->template_name,
           status_str, node->child_count, node->guess_count,
           node->assured ? ", ASSURED" : "");

    for (int i = 0; i < node->child_count; i++) {
        mass_rmi_print_tree(node->children[i], indent + 1);
    }
}

/* ═══════════════════════════════════════════════════════════════════════
   Install Context — Local Variables, Pre-Setup, Remote RMI
   ═══════════════════════════════════════════════════════════════════════ */

int mass_rmi_set_local_var(MassRMINode *node, const char *key, const char *value) {
    if (!node || !key) return -1;

    MassRMIInstallContext *ctx = &node->install_ctx;

    /* Check if key already exists — update in place */
    for (int i = 0; i < ctx->var_count; i++) {
        if (strcmp(ctx->local_vars[i].key, key) == 0) {
            strncpy(ctx->local_vars[i].value, value ? value : "",
                    MASS_RMI_MAX_VAR_VALUE - 1);
            node->modified_at = rmi_timestamp();
            return 0;
        }
    }

    /* Add new variable */
    if (ctx->var_count >= MASS_RMI_MAX_LOCAL_VARS) {
        fprintf(stderr, "[MassRMI] ERROR: Local var limit reached for '%s'\n",
                node->signature);
        return -1;
    }

    MassRMILocalVar *var = &ctx->local_vars[ctx->var_count++];
    strncpy(var->key, key, MASS_RMI_MAX_VAR_KEY - 1);
    strncpy(var->value, value ? value : "", MASS_RMI_MAX_VAR_VALUE - 1);
    node->modified_at = rmi_timestamp();
    return 0;
}

const char *mass_rmi_get_local_var(MassRMINode *node, const char *key) {
    if (!node || !key) return NULL;

    MassRMIInstallContext *ctx = &node->install_ctx;
    for (int i = 0; i < ctx->var_count; i++) {
        if (strcmp(ctx->local_vars[i].key, key) == 0) {
            return ctx->local_vars[i].value;
        }
    }

    /* Walk up to parent — variables inherit upward */
    if (node->parent) {
        return mass_rmi_get_local_var(node->parent, key);
    }

    return NULL;
}

int mass_rmi_pre_setup(MassRMINode *node, const char *method_ref) {
    if (!node || !method_ref) return -1;

    MassRMIInstallContext *ctx = &node->install_ctx;
    if (ctx->pre_setup_count >= MASS_RMI_MAX_PRE_SETUP) {
        fprintf(stderr, "[MassRMI] ERROR: Pre-setup limit reached for '%s'\n",
                node->signature);
        return -1;
    }

    strncpy(ctx->pre_setup[ctx->pre_setup_count], method_ref,
            MASS_RMI_MAX_METHOD_REF - 1);
    ctx->pre_setup_count++;
    ctx->has_pre_setup = true;

    /*
     * Execute the pre-setup callback.
     * In a full JNI implementation, this would invoke the Java method
     * via JNI CallStaticVoidMethod or CallVoidMethod depending on
     * whether the reference is Class::method or instance.method.
     *
     * For now, we record the reference and log the invocation.
     * The JNI bridge layer handles actual Java method dispatch.
     */
    fprintf(stdout, "[MassRMI] PRE-SETUP: '%s' on node '%s'\n",
            method_ref, node->signature);

    node->modified_at = rmi_timestamp();
    return 0;
}

int mass_rmi_register_remote(MassRMINode *node, const char *remote_uri) {
    if (!node || !remote_uri) return -1;

    MassRMIInstallContext *ctx = &node->install_ctx;
    if (ctx->remote_count >= MASS_RMI_MAX_REMOTES) {
        fprintf(stderr, "[MassRMI] ERROR: Remote endpoint limit reached for '%s'\n",
                node->signature);
        return -1;
    }

    /* Validate URI format: must start with rmi:// or contain host:port */
    if (strncmp(remote_uri, "rmi://", 6) != 0) {
        fprintf(stderr, "[MassRMI] ERROR: Invalid remote URI (must be rmi://...): '%s'\n",
                remote_uri);
        return -1;
    }

    strncpy(ctx->remote_endpoints[ctx->remote_count], remote_uri,
            MASS_RMI_MAX_REMOTE_URI - 1);
    ctx->remote_count++;
    ctx->has_remote = true;

    /*
     * In a full implementation, this would:
     * 1. Parse host:port from the URI
     * 2. Establish a TCP connection to the remote RMI registry
     * 3. Register this node's signature and tree structure
     * 4. Set up a listener for incoming map/guess operations
     *
     * The remote end can then:
     *   - mass_rmi_map() its trees onto this node
     *   - mass_rmi_antedelerior() guesses remotely
     *   - mass_rmi_main_guess() scaffolds across the network
     *   - Query this node's structure and state
     */
    fprintf(stdout, "[MassRMI] REMOTE-REGISTER: '%s' → '%s'\n",
            node->signature, remote_uri);

    node->modified_at = rmi_timestamp();
    return 0;
}

MassRMIInstallContext *mass_rmi_get_install_context(MassRMINode *node) {
    if (!node) return NULL;
    return &node->install_ctx;
}

/* ═══════════════════════════════════════════════════════════════════════
   JNI Bridge Entry Points
   These are called from the Java native methods in SystemMassRMI.java
   ═══════════════════════════════════════════════════════════════════════ */

/* Global tree instance (one per JVM) */
static MassRMITree g_mass_rmi_tree;
static bool g_mass_rmi_initialized = false;

int jvm_mass_rmi_global_init(const char *config_path) {
    if (g_mass_rmi_initialized) return 0;
    int ret = mass_rmi_init(&g_mass_rmi_tree, config_path);
    if (ret == 0) g_mass_rmi_initialized = true;
    return ret;
}

void jvm_mass_rmi_global_shutdown(void) {
    if (!g_mass_rmi_initialized) return;
    mass_rmi_shutdown(&g_mass_rmi_tree);
    g_mass_rmi_initialized = false;
}

MassRMITree *jvm_mass_rmi_get_tree(void) {
    if (!g_mass_rmi_initialized) {
        jvm_mass_rmi_global_init(NULL);
    }
    return &g_mass_rmi_tree;
}
