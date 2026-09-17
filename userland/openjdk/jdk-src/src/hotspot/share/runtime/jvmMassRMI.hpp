/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * jvmMassRMI.hpp - Mass RMI Feature-Group Tree System
 *
 * Provides the native backing for System.mass.RMI — a hierarchical
 * feature-group tree that can be installed, templated, and composed
 * into larger syntactic structures. Each feature-group is a node in
 * a tree that supports:
 *
 *   - create        : Instantiate a new feature-group node
 *   - add           : Attach a child feature-group under a parent
 *   - remove        : Detach a feature-group from its parent (preserves node)
 *   - delete        : Remove a feature-group from the tree permanently
 *   - destroy       : Erase a feature-group and all its descendants
 *   - anterior      : Navigate to the parent/prior node in the tree
 *   - superior      : Navigate to the root or highest-authority node
 *   - antedelerior  : Encapsulate user guesses — holds the user's
 *                     speculative inputs before confirmation
 *   - positorer     : Installer of child guesses — installs child
 *                     node predictions into the tree structure
 *   - main_guess    : Installs a reference towards a child-symporetic
 *                     (a support system for used-mind — a cognitive
 *                     scaffold that supports reasoned thought)
 *   - post_guess    : Installs knowing conditions on mind or as on a
 *                     system for assuredness — confirmatory knowledge
 *                     that stabilizes the tree after speculation
 *
 * TREE STRUCTURE:
 *
 *   A feature-group tree is a rooted n-ary tree. Each node carries:
 *     - signature    (the group-signature string)
 *     - template     (the feature-group template name)
 *     - children     (ordered list of child feature-groups)
 *     - guesses      (antedelerior encapsulation buffer)
 *     - symporetic   (main-guess reference to support scaffold)
 *     - assuredness  (post-guess confirmation state)
 *     - status       (CREATED, INSTALLED, RUNNING, DETACHED, DESTROYED)
 *
 * MAPPING:
 *
 *   Feature-groups can be mapped onto other feature-groups that may be
 *   part of a main syntax. This allows compositional assembly of complex
 *   systems from simple, well-defined feature templates.
 *
 *     tree_A.map(tree_B)  — overlays tree_B's structure onto tree_A
 *     tree_A.map(tree_B, "subpath")  — maps onto a specific subtree
 *
 * CONFIG FILE:
 *
 *   The config file specifies RMI endpoints, group signatures, and
 *   template definitions. Format is XML:
 *
 *     <mass-rmi version="1">
 *       <group signature="my-feature">
 *         <template name="feature-group">
 *           <node name="child-1" type="symporetic"/>
 *           <node name="child-2" type="assuredness"/>
 *         </template>
 *       </group>
 *     </mass-rmi>
 *
 * INVOCATION (Java):
 *
 *   System.mass.RMI("group-signature")
 *       .install
 *       .template("feature-group")
 *       .run
 *       .install("");
 *
 * Author: Maximilian Eric Alexander Rupplin von Keffikon
 * Date: August 8, 2026
 */

#ifndef SHARE_RUNTIME_JVM_MASS_RMI_HPP
#define SHARE_RUNTIME_JVM_MASS_RMI_HPP

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════════════
   Constants
   ═══════════════════════════════════════════════════════════════════════ */

#define MASS_RMI_VERSION            1
#define MASS_RMI_MAX_SIGNATURE      256
#define MASS_RMI_MAX_TEMPLATE       256
#define MASS_RMI_MAX_CHILDREN       128
#define MASS_RMI_MAX_GUESSES        64
#define MASS_RMI_MAX_NODES          4096
#define MASS_RMI_MAX_DEPTH          64
#define MASS_RMI_MAX_CONFIG_SIZE    (1024 * 1024)  /* 1MB config file max */

/* ═══════════════════════════════════════════════════════════════════════
   Status and Type Enumerations
   ═══════════════════════════════════════════════════════════════════════ */

typedef enum {
    MASS_RMI_STATUS_CREATED     = 0,
    MASS_RMI_STATUS_INSTALLED   = 1,
    MASS_RMI_STATUS_RUNNING     = 2,
    MASS_RMI_STATUS_DETACHED    = 3,
    MASS_RMI_STATUS_DESTROYED   = 4
} MassRMIStatus;

typedef enum {
    MASS_RMI_NODE_STANDARD      = 0,   /* Normal feature-group node */
    MASS_RMI_NODE_SYMPORETIC    = 1,   /* Support system for used-mind */
    MASS_RMI_NODE_ASSUREDNESS   = 2,   /* Knowing conditions, confirmatory */
    MASS_RMI_NODE_GUESS         = 3,   /* Speculative / antedelerior */
    MASS_RMI_NODE_POSITORER     = 4,   /* Installer of child guesses */
    MASS_RMI_NODE_ROOT          = 5    /* Root of the feature-group tree */
} MassRMINodeType;

typedef enum {
    MASS_RMI_OP_CREATE          = 0,
    MASS_RMI_OP_ADD             = 1,
    MASS_RMI_OP_REMOVE          = 2,
    MASS_RMI_OP_DELETE          = 3,
    MASS_RMI_OP_DESTROY         = 4,
    MASS_RMI_OP_ANTERIOR        = 5,
    MASS_RMI_OP_SUPERIOR        = 6,
    MASS_RMI_OP_ANTEDELERIOR    = 7,
    MASS_RMI_OP_POSITORER       = 8,
    MASS_RMI_OP_MAIN_GUESS      = 9,
    MASS_RMI_OP_POST_GUESS      = 10
} MassRMIOperation;

/* ═══════════════════════════════════════════════════════════════════════
   Install Context Structures (must precede MassRMINode)
   ═══════════════════════════════════════════════════════════════════════ */

#define MASS_RMI_MAX_LOCAL_VARS     64
#define MASS_RMI_MAX_PRE_SETUP      16
#define MASS_RMI_MAX_REMOTES        8
#define MASS_RMI_MAX_VAR_KEY        128
#define MASS_RMI_MAX_VAR_VALUE      512
#define MASS_RMI_MAX_METHOD_REF     256
#define MASS_RMI_MAX_REMOTE_URI     512

/* A local variable bound to a node's install context */
typedef struct MassRMILocalVar {
    char key[MASS_RMI_MAX_VAR_KEY];
    char value[MASS_RMI_MAX_VAR_VALUE];
} MassRMILocalVar;

/* Install context attached to a node after .run.install("xxx") */
typedef struct MassRMIInstallContext {
    MassRMILocalVar     local_vars[MASS_RMI_MAX_LOCAL_VARS];
    int                 var_count;

    char                pre_setup[MASS_RMI_MAX_PRE_SETUP][MASS_RMI_MAX_METHOD_REF];
    int                 pre_setup_count;

    char                remote_endpoints[MASS_RMI_MAX_REMOTES][MASS_RMI_MAX_REMOTE_URI];
    int                 remote_count;

    bool                has_remote;
    bool                has_pre_setup;
} MassRMIInstallContext;

/* ═══════════════════════════════════════════════════════════════════════
   Core Data Structures
   ═══════════════════════════════════════════════════════════════════════ */

/* A single guess entry (antedelerior encapsulation) */
typedef struct MassRMIGuess {
    uint64_t            id;
    char                content[512];
    uint64_t            timestamp;
    bool                confirmed;      /* Promoted to post-guess if true */
    int                 confidence;     /* 0-100 */
} MassRMIGuess;

/* A node in the feature-group tree */
typedef struct MassRMINode {
    uint64_t            id;
    char                signature[MASS_RMI_MAX_SIGNATURE];
    char                template_name[MASS_RMI_MAX_TEMPLATE];
    MassRMINodeType     type;
    MassRMIStatus       status;

    /* Tree structure */
    struct MassRMINode *parent;
    struct MassRMINode *children[MASS_RMI_MAX_CHILDREN];
    int                 child_count;
    int                 depth;

    /* Antedelerior — user guesses buffer */
    MassRMIGuess        guesses[MASS_RMI_MAX_GUESSES];
    int                 guess_count;

    /* Symporetic reference (main-guess target) */
    struct MassRMINode *symporetic_ref;

    /* Post-guess assuredness state */
    bool                assured;
    char                assuredness_condition[512];
    uint64_t            assuredness_timestamp;

    /* Metadata */
    uint64_t            created_at;
    uint64_t            modified_at;
    int                 mapping_count;      /* How many trees mapped onto this */

    /* Install context (populated by .run.install("xxx")) */
    MassRMIInstallContext install_ctx;
} MassRMINode;

/* The top-level feature-group tree */
typedef struct MassRMITree {
    MassRMINode        *root;
    char                config_path[1024];
    int                 total_nodes;
    uint64_t            tree_id;
    bool                initialized;

    /* Node pool */
    MassRMINode         node_pool[MASS_RMI_MAX_NODES];
    int                 pool_used;
} MassRMITree;

/* ═══════════════════════════════════════════════════════════════════════
   API — Core Operations
   ═══════════════════════════════════════════════════════════════════════ */

/* Initialize the Mass RMI subsystem from a config file */
int mass_rmi_init(MassRMITree *tree, const char *config_path);

/* Shutdown and release all resources */
void mass_rmi_shutdown(MassRMITree *tree);

/* Create a new feature-group with given signature */
MassRMINode *mass_rmi_create(MassRMITree *tree, const char *signature);

/* Install a template onto a node (transitions to INSTALLED) */
int mass_rmi_install_template(MassRMINode *node, const char *template_name);

/* Run the feature-group (transitions to RUNNING) */
int mass_rmi_run(MassRMINode *node);

/* Add a child node under a parent */
int mass_rmi_add(MassRMINode *parent, MassRMINode *child);

/* Remove a child from its parent (node remains in pool, status DETACHED) */
int mass_rmi_remove(MassRMINode *node);

/* Delete a node permanently (status DESTROYED, freed from pool) */
int mass_rmi_delete(MassRMITree *tree, MassRMINode *node);

/* Destroy a node and ALL its descendants recursively */
int mass_rmi_destroy(MassRMITree *tree, MassRMINode *node);

/* ═══════════════════════════════════════════════════════════════════════
   API — Navigation
   ═══════════════════════════════════════════════════════════════════════ */

/* anterior — navigate to parent/prior node */
MassRMINode *mass_rmi_anterior(MassRMINode *node);

/* superior — navigate to root/highest-authority node */
MassRMINode *mass_rmi_superior(MassRMINode *node);

/* ═══════════════════════════════════════════════════════════════════════
   API — Guess System (Antedelerior / Positorer / Main-Guess / Post-Guess)
   ═══════════════════════════════════════════════════════════════════════ */

/*
 * antedelerior — Encapsulate a user guess.
 * Stores the speculative input into the node's guess buffer.
 * The guess is held (not committed) until confirmed via post_guess.
 */
int mass_rmi_antedelerior(MassRMINode *node, const char *guess_content,
                          int confidence);

/*
 * positorer — Install child guesses.
 * Takes guesses from the antedelerior buffer of a parent node and
 * installs them as child nodes (type POSITORER). This is the mechanism
 * by which speculative thought becomes structural.
 */
int mass_rmi_positorer(MassRMITree *tree, MassRMINode *parent);

/*
 * main_guess — Install a reference towards a child-symporetic.
 * A symporetic is a support system for used-mind — a cognitive
 * scaffold that holds reasoned thought patterns. main_guess creates
 * this scaffold and links it to the target node.
 */
MassRMINode *mass_rmi_main_guess(MassRMITree *tree, MassRMINode *target,
                                  const char *symporetic_signature);

/*
 * post_guess — Install knowing conditions on mind/system.
 * Marks the node's guesses as confirmed and installs assuredness —
 * the state of knowing that stabilizes the tree after speculation.
 */
int mass_rmi_post_guess(MassRMINode *node, const char *condition);

/* ═══════════════════════════════════════════════════════════════════════
   API — Mapping (Feature-group composition)
   ═══════════════════════════════════════════════════════════════════════ */

/*
 * Map one feature-group tree onto another.
 * This overlays source's structure onto target, enabling compositional
 * assembly of complex systems from simple feature templates.
 */
int mass_rmi_map(MassRMINode *target, MassRMINode *source);

/*
 * Map onto a specific subpath within the target tree.
 */
int mass_rmi_map_subpath(MassRMITree *tree, MassRMINode *target,
                         MassRMINode *source, const char *subpath);

/* ═══════════════════════════════════════════════════════════════════════
   API — Query
   ═══════════════════════════════════════════════════════════════════════ */

/* Find a node by signature (DFS) */
MassRMINode *mass_rmi_find(MassRMITree *tree, const char *signature);

/* Get node count in subtree */
int mass_rmi_count(MassRMINode *node);

/* Print tree structure (debug) */
void mass_rmi_print_tree(MassRMINode *node, int indent);

/* ═══════════════════════════════════════════════════════════════════════
   API — Install Context (Local Variables, Pre-Setup, Remote RMI)
   ═══════════════════════════════════════════════════════════════════════ */

/* Set a local variable on a node */
int mass_rmi_set_local_var(MassRMINode *node, const char *key, const char *value);

/* Get a local variable from a node (inherits from parent if not found) */
const char *mass_rmi_get_local_var(MassRMINode *node, const char *key);

/* Execute a pre-setup method reference (native callback) */
int mass_rmi_pre_setup(MassRMINode *node, const char *method_ref);

/* Register this node with a remote RMI endpoint */
int mass_rmi_register_remote(MassRMINode *node, const char *remote_uri);

/* Get the install context for a node */
MassRMIInstallContext *mass_rmi_get_install_context(MassRMINode *node);

#ifdef __cplusplus
}
#endif

#endif /* SHARE_RUNTIME_JVM_MASS_RMI_HPP */
