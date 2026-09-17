/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * SystemMassRMI.java - Mass RMI Feature-Group Tree for java.lang.System
 *
 * Provides System.mass — the entry point for RMI feature-group trees.
 *
 * Usage:
 *   System.mass.RMI("group-signature")
 *       .install
 *       .template("feature-group")
 *       .run
 *       .install("");
 *
 * Author: Maximilian Eric Alexander Rupplin von Keffikon
 * Date: August 8, 2026
 */

package java.lang;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * Mass RMI Feature-Group Tree System.
 *
 * <p>Provides hierarchical feature-group creation, template installation,
 * guess encapsulation, and compositional mapping. Accessed via
 * {@code System.mass}.</p>
 *
 * <h2>Architecture</h2>
 * <p>A feature-group tree is a rooted n-ary tree where each node carries
 * a signature, template, children, guesses (antedelerior), a symporetic
 * reference (main-guess), and assuredness state (post-guess).</p>
 *
 * <h2>Operations</h2>
 * <ul>
 *   <li><b>create</b> — Instantiate a new feature-group node</li>
 *   <li><b>add</b> — Attach a child feature-group under a parent</li>
 *   <li><b>remove</b> — Detach a feature-group (preserves node)</li>
 *   <li><b>delete</b> — Remove a feature-group permanently</li>
 *   <li><b>destroy</b> — Erase a node and all descendants</li>
 *   <li><b>anterior</b> — Navigate to parent/prior node</li>
 *   <li><b>superior</b> — Navigate to root/highest-authority node</li>
 *   <li><b>antedelerior</b> — Encapsulate user guesses</li>
 *   <li><b>positorer</b> — Install child guesses into the tree</li>
 *   <li><b>main-guess</b> — Install a child-symporetic (support for used-mind)</li>
 *   <li><b>post-guess</b> — Install knowing conditions for assuredness</li>
 * </ul>
 *
 * <h2>Example</h2>
 * <pre>{@code
 * // Create and run a feature-group
 * FeatureNode node = System.mass.RMI("my-feature")
 *     .install
 *     .template("feature-group")
 *     .run
 *     .install("");
 *
 * // Encapsulate a user guess
 * node.antedelerior("the system needs caching", 75);
 *
 * // Install child guesses
 * node.positorer();
 *
 * // Install symporetic scaffold
 * node.mainGuess("caching-support-system");
 *
 * // Confirm with assuredness
 * node.postGuess("caching confirmed via benchmark");
 *
 * // Map another tree onto this one
 * FeatureNode other = System.mass.RMI("other-feature").install.template("aux").run.install("");
 * node.map(other);
 * }</pre>
 *
 * @since 28
 * @author Maximilian Eric Alexander Rupplin von Keffikon
 */
public final class SystemMassRMI {

    /* Native library loaded by System class */
    static {
        initNative();
    }

    /** The singleton instance, accessed via System.mass */
    static final SystemMassRMI INSTANCE = new SystemMassRMI();

    private SystemMassRMI() {}

    /* ═══════════════════════════════════════════════════════════════════
       Native Methods
       ═══════════════════════════════════════════════════════════════════ */

    private static native void initNative();
    private static native long nativeCreate(String signature);
    private static native int nativeInstallTemplate(long nodeId, String templateName);
    private static native int nativeRun(long nodeId);
    private static native int nativeAdd(long parentId, long childId);
    private static native int nativeRemove(long nodeId);
    private static native int nativeDelete(long nodeId);
    private static native int nativeDestroy(long nodeId);
    private static native long nativeAnterior(long nodeId);
    private static native long nativeSuperior(long nodeId);
    private static native int nativeAntedelerior(long nodeId, String content, int confidence);
    private static native int nativePositorer(long nodeId);
    private static native long nativeMainGuess(long nodeId, String symporeticSig);
    private static native int nativePostGuess(long nodeId, String condition);
    private static native int nativeMap(long targetId, long sourceId);
    private static native long nativeFind(String signature);
    private static native int nativeCount(long nodeId);
    private static native int nativePreSetup(long nodeId, String methodRef);
    private static native int nativeSetLocalVar(long nodeId, String key, String value);
    private static native int nativeRegisterRemote(long nodeId, String remoteUri);
    private static native String nativeGetLocalVar(long nodeId, String key);

    /* ═══════════════════════════════════════════════════════════════════
       Public API — Entry Point
       ═══════════════════════════════════════════════════════════════════ */

    /**
     * Begin an RMI feature-group tree operation.
     *
     * <p>Creates a new feature-group node with the given signature.
     * Returns an {@link RMIBuilder} for fluent chain invocation.</p>
     *
     * @param groupSignature the unique signature for this feature-group
     * @return an RMIBuilder for chaining install/template/run operations
     * @throws NullPointerException if groupSignature is null
     *
     * @since 28
     */
    public RMIBuilder RMI(String groupSignature) {
        Objects.requireNonNull(groupSignature, "group-signature must not be null");
        long nodeId = nativeCreate(groupSignature);
        if (nodeId <= 0) {
            throw new IllegalStateException("Failed to create feature-group: " + groupSignature);
        }
        return new RMIBuilder(nodeId, groupSignature);
    }

    /* ═══════════════════════════════════════════════════════════════════
       RMIBuilder — Fluent chain: .install.template("x").run.install("")
       ═══════════════════════════════════════════════════════════════════ */

    /**
     * Fluent builder for RMI feature-group installation.
     *
     * <p>Supports the invocation chain:
     * {@code System.mass.RMI("sig").install.template("tpl").run.install("")}</p>
     *
     * @since 28
     */
    public static final class RMIBuilder {
        private final long nodeId;
        private final String signature;

        RMIBuilder(long nodeId, String signature) {
            this.nodeId = nodeId;
            this.signature = signature;
        }

        /**
         * Transition to install phase. Returns a {@link TemplatePhase}
         * for template selection.
         */
        public final TemplatePhase install = new TemplatePhase(this);

        /**
         * Template phase — selects the feature-group template.
         *
         * @since 28
         */
        public static final class TemplatePhase {
            private final RMIBuilder builder;

            TemplatePhase(RMIBuilder builder) {
                this.builder = builder;
            }

            /**
             * Select and install a template onto this feature-group.
             *
             * @param templateName the template identifier
             * @return a {@link RunPhase} for run/install
             *
             * @since 28
             */
            public RunPhase template(String templateName) {
                Objects.requireNonNull(templateName, "template name must not be null");
                int ret = nativeInstallTemplate(builder.nodeId, templateName);
                if (ret != 0) {
                    throw new IllegalStateException(
                        "Failed to install template '" + templateName +
                        "' on node '" + builder.signature + "'");
                }
                return new RunPhase(builder);
            }
        }

        /**
         * Run phase — activates the feature-group.
         *
         * @since 28
         */
        public static final class RunPhase {
            private final RMIBuilder builder;

            RunPhase(RMIBuilder builder) {
                this.builder = builder;
            }

            /**
             * Run the feature-group (transitions to RUNNING state).
             * Returns a {@link FinalInstallPhase}.
             */
            public final FinalInstallPhase run = new FinalInstallPhase(this);
        }

        /**
         * Final install phase — completes the chain.
         *
         * <p>The install parameter string supports three modes:</p>
         * <ul>
         *   <li><b>Local variables</b> — key=value pairs local to this
         *       installation context (e.g., {@code "port=8080;timeout=30"})</li>
         *   <li><b>Pre-setup method</b> — a method reference to execute
         *       before installation completes (e.g., {@code "@pre:initCache"})</li>
         *   <li><b>Remote RMI setup</b> — a remote endpoint for distributed
         *       feature-group coordination (e.g., {@code "rmi://host:port/path"})</li>
         * </ul>
         *
         * <h3>Parameter Format</h3>
         * <pre>{@code
         * // Local variables only
         * .run.install("port=8080;timeout=30;mode=async");
         *
         * // Pre-setup method reference
         * .run.install("@pre:com.app.Setup::initCache");
         *
         * // Remote RMI endpoint
         * .run.install("rmi://192.168.1.10:1099/feature-registry");
         *
         * // Combined: locals + pre-setup + remote
         * .run.install("port=8080;@pre:initDB;rmi://remote:1099/sync");
         *
         * // Empty string — no extras, just run
         * .run.install("");
         * }</pre>
         *
         * @since 28
         */
        public static final class FinalInstallPhase {
            private final RunPhase runPhase;

            FinalInstallPhase(RunPhase runPhase) {
                this.runPhase = runPhase;
            }

            /**
             * Complete installation with parameters supporting local variables,
             * pre-setup methods, and remote RMI setup references.
             *
             * <p>The parameters string is parsed for three instruction types:</p>
             * <ol>
             *   <li><b>Local variables</b> ({@code key=value}) — bound to this
             *       node's install context. Available to the feature-group and its
             *       children during runtime. Semicolon-delimited.</li>
             *   <li><b>Pre-setup directives</b> ({@code @pre:method}) — method
             *       references invoked before the node enters RUNNING state.
             *       Supports class::method syntax for static methods or simple
             *       names for methods on the current feature-group.</li>
             *   <li><b>Remote RMI endpoints</b> ({@code rmi://host:port/path}) —
             *       registers this node with a remote RMI registry for distributed
             *       feature-group coordination. The remote end can map its own
             *       feature-groups onto this tree.</li>
             * </ol>
             *
             * @param parameters semicolon-delimited install directives, or empty string
             * @return the operational FeatureNode with install context applied
             * @throws IllegalStateException if run or remote connection fails
             * @throws IllegalArgumentException if parameter format is invalid
             *
             * @since 28
             */
            public FeatureNode install(String parameters) {
                long nodeId = runPhase.builder.nodeId;
                String signature = runPhase.builder.signature;

                /* Parse the parameter string into context */
                InstallContext ctx = InstallContext.parse(parameters);

                /* Execute pre-setup methods before run */
                for (String preSetup : ctx.getPreSetupMethods()) {
                    int ret = nativePreSetup(nodeId, preSetup);
                    if (ret != 0) {
                        throw new IllegalStateException(
                            "Pre-setup '" + preSetup + "' failed for '" + signature + "'");
                    }
                }

                /* Apply local variables to the node */
                for (var entry : ctx.getLocalVariables().entrySet()) {
                    nativeSetLocalVar(nodeId, entry.getKey(), entry.getValue());
                }

                /* Run the node (transition to RUNNING) */
                int ret = nativeRun(nodeId);
                if (ret != 0) {
                    throw new IllegalStateException(
                        "Failed to run feature-group '" + signature + "'");
                }

                /* Register with remote RMI endpoints */
                for (String remoteUri : ctx.getRemoteEndpoints()) {
                    int rret = nativeRegisterRemote(nodeId, remoteUri);
                    if (rret != 0) {
                        throw new IllegalStateException(
                            "Remote RMI registration failed: " + remoteUri);
                    }
                }

                FeatureNode node = new FeatureNode(nodeId, signature);
                node.installContext = ctx;
                return node;
            }

            /**
             * Install with a pre-built context object for programmatic setup.
             *
             * @param ctx the InstallContext with variables, pre-setup, and remotes
             * @return the operational FeatureNode
             *
             * @since 28
             */
            public FeatureNode install(InstallContext ctx) {
                Objects.requireNonNull(ctx);
                return install(ctx.toParameterString());
            }
        }
    }

    /* ═══════════════════════════════════════════════════════════════════
       FeatureNode — Operational handle to a feature-group tree node
       ═══════════════════════════════════════════════════════════════════ */

    /**
     * Represents an operational node in the feature-group tree.
     *
     * <p>Provides all tree operations: create, add, remove, delete,
     * destroy, anterior, superior, antedelerior, positorer,
     * main-guess, and post-guess.</p>
     *
     * @since 28
     */
    public static final class FeatureNode {
        private final long nodeId;
        private final String signature;
        private final List<FeatureNode> children = new CopyOnWriteArrayList<>();
        InstallContext installContext;

        FeatureNode(long nodeId, String signature) {
            this.nodeId = nodeId;
            this.signature = signature;
        }

        /**
         * Get the node's unique signature.
         * @return the group-signature string
         */
        public String getSignature() { return signature; }

        /**
         * Get the native node identifier.
         * @return the internal node ID
         */
        public long getNodeId() { return nodeId; }

        /**
         * Get the children of this node.
         * @return unmodifiable list of child nodes
         */
        public List<FeatureNode> getChildren() {
            return Collections.unmodifiableList(children);
        }

        /**
         * Get the install context (local variables, pre-setup, remotes).
         * @return the InstallContext, or null if installed with empty params
         *
         * @since 28
         */
        public InstallContext getInstallContext() { return installContext; }

        /**
         * Get a local variable set during installation.
         *
         * @param key the variable name
         * @return the value, or null if not set
         *
         * @since 28
         */
        public String getLocalVar(String key) {
            if (installContext != null) {
                return installContext.getLocalVariables().get(key);
            }
            return nativeGetLocalVar(nodeId, key);
        }

        /**
         * Set a local variable on this node's install context at runtime.
         *
         * @param key the variable name
         * @param value the variable value
         *
         * @since 28
         */
        public void setLocalVar(String key, String value) {
            Objects.requireNonNull(key);
            nativeSetLocalVar(nodeId, key, value != null ? value : "");
            if (installContext != null) {
                installContext.getLocalVariables().put(key, value != null ? value : "");
            }
        }

        /* ── Core Operations ── */

        /**
         * Create a new child feature-group under this node.
         *
         * @param childSignature signature for the new child
         * @return the new child FeatureNode
         * @throws NullPointerException if childSignature is null
         *
         * @since 28
         */
        public FeatureNode create(String childSignature) {
            Objects.requireNonNull(childSignature);
            long childId = nativeCreate(childSignature);
            if (childId <= 0) throw new IllegalStateException("create failed");
            nativeAdd(this.nodeId, childId);
            FeatureNode child = new FeatureNode(childId, childSignature);
            children.add(child);
            return child;
        }

        /**
         * Add an existing feature-group as a child of this node.
         *
         * @param child the node to attach
         * @throws NullPointerException if child is null
         *
         * @since 28
         */
        public void add(FeatureNode child) {
            Objects.requireNonNull(child);
            nativeAdd(this.nodeId, child.nodeId);
            children.add(child);
        }

        /**
         * Remove this node from its parent. The node is preserved
         * but detached (status DETACHED).
         *
         * @since 28
         */
        public void remove() {
            nativeRemove(this.nodeId);
        }

        /**
         * Delete this node permanently from the tree.
         *
         * @since 28
         */
        public void delete() {
            nativeDelete(this.nodeId);
        }

        /**
         * Destroy this node and all its descendants recursively.
         *
         * @since 28
         */
        public void destroy() {
            nativeDestroy(this.nodeId);
            children.clear();
        }

        /* ── Navigation ── */

        /**
         * Navigate to the parent/prior node in the tree.
         *
         * @return the anterior (parent) node, or null if this is root
         *
         * @since 28
         */
        public FeatureNode anterior() {
            long parentId = nativeAnterior(this.nodeId);
            if (parentId <= 0) return null;
            return new FeatureNode(parentId, "anterior-of-" + signature);
        }

        /**
         * Navigate to the root/highest-authority node.
         *
         * @return the superior (root) node
         *
         * @since 28
         */
        public FeatureNode superior() {
            long rootId = nativeSuperior(this.nodeId);
            if (rootId <= 0) return null;
            return new FeatureNode(rootId, "__root__");
        }

        /* ── Guess System ── */

        /**
         * Encapsulate a user guess (antedelerior).
         *
         * <p>Stores the speculative input into this node's guess buffer.
         * The guess is held (not committed) until confirmed via
         * {@link #postGuess(String)}.</p>
         *
         * @param guessContent the speculative content
         * @param confidence confidence level 0-100
         * @return this node for chaining
         *
         * @since 28
         */
        public FeatureNode antedelerior(String guessContent, int confidence) {
            Objects.requireNonNull(guessContent);
            nativeAntedelerior(this.nodeId, guessContent, confidence);
            return this;
        }

        /**
         * Install child guesses (positorer).
         *
         * <p>Takes guesses from the antedelerior buffer and installs
         * them as child nodes. This is how speculative thought becomes
         * structural — the installer of child guesses.</p>
         *
         * @return number of guesses installed as children
         *
         * @since 28
         */
        public int positorer() {
            return nativePositorer(this.nodeId);
        }

        /**
         * Install a reference towards a child-symporetic (main-guess).
         *
         * <p>A symporetic is a support system for used-mind — a cognitive
         * scaffold that supports reasoned thought. main-guess creates
         * this scaffold and links it to this node.</p>
         *
         * @param symporeticSignature signature for the support scaffold
         * @return the symporetic FeatureNode
         *
         * @since 28
         */
        public FeatureNode mainGuess(String symporeticSignature) {
            Objects.requireNonNull(symporeticSignature);
            long scaffoldId = nativeMainGuess(this.nodeId, symporeticSignature);
            if (scaffoldId <= 0) throw new IllegalStateException("mainGuess failed");
            FeatureNode scaffold = new FeatureNode(scaffoldId, symporeticSignature);
            children.add(scaffold);
            return scaffold;
        }

        /**
         * Install knowing conditions on mind/system (post-guess).
         *
         * <p>Marks all unconfirmed guesses as confirmed and installs
         * assuredness — the state of knowing that stabilizes the tree
         * after speculation. This is about installing knowing conditions
         * on mind or as on a system for assuredness.</p>
         *
         * @param condition the assuredness condition (what is now known)
         * @return number of guesses confirmed
         *
         * @since 28
         */
        public int postGuess(String condition) {
            Objects.requireNonNull(condition);
            return nativePostGuess(this.nodeId, condition);
        }

        /* ── Mapping ── */

        /**
         * Map another feature-group tree onto this node.
         *
         * <p>Overlays the source's structure onto this node, enabling
         * compositional assembly of complex systems from simple
         * feature templates. The mapped tree becomes part of this
         * node's main syntax.</p>
         *
         * @param source the feature-group to map onto this node
         *
         * @since 28
         */
        public void map(FeatureNode source) {
            Objects.requireNonNull(source);
            nativeMap(this.nodeId, source.nodeId);
        }

        /* ── Query ── */

        /**
         * Count total nodes in the subtree rooted at this node.
         * @return number of nodes (including this one)
         */
        public int count() {
            return nativeCount(this.nodeId);
        }

        @Override
        public String toString() {
            return "FeatureNode[" + signature + ", id=" + nodeId +
                   ", children=" + children.size() + "]";
        }
    }

    /* ═══════════════════════════════════════════════════════════════════
       Static Lookup
       ═══════════════════════════════════════════════════════════════════ */

    /**
     * Find an existing feature-group node by signature.
     *
     * @param signature the group-signature to search for
     * @return the FeatureNode, or null if not found
     *
     * @since 28
     */
    public FeatureNode find(String signature) {
        Objects.requireNonNull(signature);
        long id = nativeFind(signature);
        if (id <= 0) return null;
        return new FeatureNode(id, signature);
    }

    /* ═══════════════════════════════════════════════════════════════════
       InstallContext — Local Variables, Pre-Setup, Remote RMI
       ═══════════════════════════════════════════════════════════════════ */

    /**
     * Encapsulates the install context parsed from the parameter string
     * passed to {@code .run.install("xxx")}.
     *
     * <h2>Parameter String Format</h2>
     *
     * <p>The parameter string is semicolon-delimited and supports three
     * instruction types intermixed in any order:</p>
     *
     * <table>
     *   <tr><th>Type</th><th>Prefix</th><th>Example</th><th>Purpose</th></tr>
     *   <tr><td>Local variable</td><td>(none, key=value)</td>
     *       <td>{@code port=8080}</td><td>Install-local binding</td></tr>
     *   <tr><td>Pre-setup method</td><td>{@code @pre:}</td>
     *       <td>{@code @pre:com.app.DB::init}</td>
     *       <td>Execute before RUNNING state</td></tr>
     *   <tr><td>Remote RMI</td><td>{@code rmi://}</td>
     *       <td>{@code rmi://host:1099/registry}</td>
     *       <td>Distributed feature-group sync</td></tr>
     * </table>
     *
     * <h2>Local Variables</h2>
     * <p>Key=value pairs bound to the node's install context. These are
     * available to the feature-group, its children, and any mapped trees
     * during runtime. They serve as configuration for the feature-group
     * template without requiring recompilation.</p>
     *
     * <h2>Pre-Setup Methods</h2>
     * <p>Method references invoked in order before the node enters RUNNING.
     * Supports:</p>
     * <ul>
     *   <li>{@code @pre:methodName} — invokes on the feature-group itself</li>
     *   <li>{@code @pre:com.pkg.Class::method} — invokes a static method</li>
     *   <li>{@code @pre:instance.method} — invokes on a named instance</li>
     * </ul>
     *
     * <h2>Remote RMI Endpoints</h2>
     * <p>Registers this feature-group with a remote RMI registry for
     * distributed coordination. The remote system can:</p>
     * <ul>
     *   <li>Map its feature-groups onto this local tree</li>
     *   <li>Query this tree's structure and state</li>
     *   <li>Push antedelerior guesses into this tree remotely</li>
     *   <li>Install symporetic scaffolds across network boundaries</li>
     * </ul>
     *
     * <h2>Examples</h2>
     * <pre>{@code
     * // Local variables for configuration
     * .run.install("port=8080;timeout=30;mode=async;retries=3");
     *
     * // Pre-setup: initialize database before running
     * .run.install("@pre:com.myapp.Database::initialize;@pre:warmCache");
     *
     * // Remote RMI: register with cluster coordinator
     * .run.install("rmi://coordinator.internal:1099/feature-cluster");
     *
     * // All three combined
     * .run.install("port=9090;@pre:initSSL;rmi://master:1099/sync;env=prod");
     *
     * // Inferred remote setup from local variable
     * .run.install("remote.host=10.0.0.1;remote.port=1099;@pre:autoConnect");
     *
     * // Empty — no extras
     * .run.install("");
     * }</pre>
     *
     * @since 28
     * @author Maximilian Eric Alexander Rupplin von Keffikon
     */
    public static final class InstallContext {

        private final java.util.Map<String, String> localVariables;
        private final List<String> preSetupMethods;
        private final List<String> remoteEndpoints;
        private final String rawParameters;

        private InstallContext(String rawParameters,
                               java.util.Map<String, String> locals,
                               List<String> preSets,
                               List<String> remotes) {
            this.rawParameters = rawParameters != null ? rawParameters : "";
            this.localVariables = locals;
            this.preSetupMethods = preSets;
            this.remoteEndpoints = remotes;
        }

        /**
         * Parse a parameter string into an InstallContext.
         *
         * @param parameters semicolon-delimited parameter string
         * @return the parsed InstallContext
         */
        public static InstallContext parse(String parameters) {
            java.util.Map<String, String> locals = new java.util.LinkedHashMap<>();
            List<String> preSets = new ArrayList<>();
            List<String> remotes = new ArrayList<>();

            if (parameters == null || parameters.isEmpty()) {
                return new InstallContext(parameters, locals, preSets, remotes);
            }

            String[] parts = parameters.split(";");
            for (String part : parts) {
                String trimmed = part.trim();
                if (trimmed.isEmpty()) continue;

                if (trimmed.startsWith("@pre:")) {
                    /* Pre-setup method reference */
                    String method = trimmed.substring(5).trim();
                    if (!method.isEmpty()) {
                        preSets.add(method);
                    }
                } else if (trimmed.startsWith("rmi://")) {
                    /* Remote RMI endpoint URI */
                    remotes.add(trimmed);
                } else if (trimmed.contains("=")) {
                    /* Local variable: key=value */
                    int eq = trimmed.indexOf('=');
                    String key = trimmed.substring(0, eq).trim();
                    String val = trimmed.substring(eq + 1).trim();
                    if (!key.isEmpty()) {
                        locals.put(key, val);
                    }
                } else {
                    /*
                     * Inferrable: a bare string could be:
                     *   - A method name (inferred @pre:)
                     *   - A remote shorthand (if contains ':' or '/')
                     *   - A flag variable (key with empty value)
                     */
                    if (trimmed.contains("://") || trimmed.contains(":")) {
                        /* Infer as remote endpoint */
                        remotes.add(trimmed.contains("://") ? trimmed : "rmi://" + trimmed);
                    } else if (trimmed.contains("::") || trimmed.contains(".")) {
                        /* Infer as pre-setup method */
                        preSets.add(trimmed);
                    } else {
                        /* Flag variable (boolean true) */
                        locals.put(trimmed, "true");
                    }
                }
            }

            return new InstallContext(parameters, locals, preSets, remotes);
        }

        /**
         * Create an InstallContext programmatically via builder.
         * @return a new Builder
         */
        public static Builder builder() {
            return new Builder();
        }

        /**
         * Get all local variables bound to this install context.
         * @return mutable map of key-value pairs
         */
        public java.util.Map<String, String> getLocalVariables() {
            return localVariables;
        }

        /**
         * Get the ordered list of pre-setup method references.
         * @return list of method references (class::method or name)
         */
        public List<String> getPreSetupMethods() {
            return Collections.unmodifiableList(preSetupMethods);
        }

        /**
         * Get the list of remote RMI endpoints.
         * @return list of RMI URIs (rmi://host:port/path)
         */
        public List<String> getRemoteEndpoints() {
            return Collections.unmodifiableList(remoteEndpoints);
        }

        /**
         * Get the raw parameter string as originally passed.
         * @return the raw parameters
         */
        public String getRawParameters() { return rawParameters; }

        /**
         * Check if this context has any remote RMI registrations.
         * @return true if remote endpoints are configured
         */
        public boolean hasRemote() { return !remoteEndpoints.isEmpty(); }

        /**
         * Check if this context has pre-setup methods.
         * @return true if pre-setup directives exist
         */
        public boolean hasPreSetup() { return !preSetupMethods.isEmpty(); }

        /**
         * Serialize this context back to parameter string format.
         * @return semicolon-delimited parameter string
         */
        public String toParameterString() {
            StringBuilder sb = new StringBuilder();
            for (var entry : localVariables.entrySet()) {
                if (sb.length() > 0) sb.append(';');
                sb.append(entry.getKey()).append('=').append(entry.getValue());
            }
            for (String pre : preSetupMethods) {
                if (sb.length() > 0) sb.append(';');
                sb.append("@pre:").append(pre);
            }
            for (String remote : remoteEndpoints) {
                if (sb.length() > 0) sb.append(';');
                sb.append(remote);
            }
            return sb.toString();
        }

        @Override
        public String toString() {
            return "InstallContext[vars=" + localVariables.size() +
                   ", preSets=" + preSetupMethods.size() +
                   ", remotes=" + remoteEndpoints.size() + "]";
        }

        /**
         * Programmatic builder for InstallContext.
         *
         * <pre>{@code
         * InstallContext ctx = InstallContext.builder()
         *     .localVar("port", "8080")
         *     .localVar("timeout", "30")
         *     .preSetup("com.app.DB::init")
         *     .preSetup("warmCache")
         *     .remote("rmi://coordinator:1099/cluster")
         *     .build();
         *
         * System.mass.RMI("my-service").install.template("web-server").run.install(ctx);
         * }</pre>
         *
         * @since 28
         */
        public static final class Builder {
            private final java.util.Map<String, String> locals = new java.util.LinkedHashMap<>();
            private final List<String> preSets = new ArrayList<>();
            private final List<String> remotes = new ArrayList<>();

            Builder() {}

            /**
             * Add a local variable.
             * @param key variable name
             * @param value variable value
             * @return this builder
             */
            public Builder localVar(String key, String value) {
                locals.put(key, value);
                return this;
            }

            /**
             * Add a pre-setup method reference.
             * @param methodRef class::method or method name
             * @return this builder
             */
            public Builder preSetup(String methodRef) {
                preSets.add(methodRef);
                return this;
            }

            /**
             * Add a remote RMI endpoint.
             * @param uri the RMI URI (rmi://host:port/path)
             * @return this builder
             */
            public Builder remote(String uri) {
                remotes.add(uri.startsWith("rmi://") ? uri : "rmi://" + uri);
                return this;
            }

            /**
             * Build the InstallContext.
             * @return the constructed InstallContext
             */
            public InstallContext build() {
                InstallContext ctx = new InstallContext(null, locals, preSets, remotes);
                return ctx;
            }
        }
    }
}
