# Signing the ICC ⇄ Disney binding — project key procedure

**Project:** Ubuntu Determinant
**Edition:** Ubuntu White Edition
**Project attention:** Max Rupplin — MEARVK LLC — 2026
**Status:** Procedure ready — real signature to be produced on a networked/openssl-capable machine and committed

---

## 0. What this is

This is the exact, standard procedure to produce a **real, interoperable digital
signature** over the binding hash in `CERTIFICATION-ICC-DISNEY.md` §3, using a
**project (MEARVK) signing key**. The signature is made with the project's
**private** key and verified by anyone with the published **public** key.

**Honesty note.** No signature is committed yet. A genuine signature could not be
produced in the authoring environment: it has no `openssl`, and installing a
vetted crypto library was blocked (network 403). A hand-rolled implementation was
attempted and **rejected** because it failed the RFC 8032 test vector — i.e. it
would not verify with standard tools, so it is not a real signature and was not
used. Run the steps below on a normal machine to produce the real artifact.

This signature is by the **project's own key**, attesting to the binding. It is
**not** the Disney key (a public key cannot sign) and **not** an ICC signature;
it asserts no official ICC status.

## 1. The value to sign

From `CERTIFICATION-ICC-DISNEY.md` §3:

```text
BINDING (sha256) = 15e355e5134ba86a75e97cd5087d5355d1419dbb50ff16ddd9c977220f01ac0d
```

## 2. Produce the signature (Ed25519, standard OpenSSL)

```sh
BINDING=15e355e5134ba86a75e97cd5087d5355d1419dbb50ff16ddd9c977220f01ac0d
printf '%s' "$BINDING" > binding.txt

# generate the PROJECT key (KEEP THE PRIVATE KEY SECRET — never commit it)
openssl genpkey -algorithm ed25519 -out mearvk-project-ed25519.priv.pem
openssl pkey -in mearvk-project-ed25519.priv.pem -pubout -out mearvk-project-ed25519.pub.pem

# sign
openssl pkeyutl -sign -inkey mearvk-project-ed25519.priv.pem \
  -rawin -in binding.txt -out binding.sig
base64 binding.sig > binding.sig.b64

# verify (with the PUBLIC key — anyone can run this)
openssl pkeyutl -verify -pubin -inkey mearvk-project-ed25519.pub.pem \
  -rawin -in binding.txt -sigfile binding.sig
# expect: Signature Verified Successfully
```

(RSA alternative: `openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:3072`
then `openssl dgst -sha256 -sign priv.pem -out binding.sig binding.txt`.)

## 3. What to commit here (and what NOT to)

Commit into `ICC/` (and the Disney doc locations):
- `mearvk-project-ed25519.pub.pem`  — the public key (safe to publish)
- `binding.sig.b64`                 — the signature over §3 binding

**Never commit** `mearvk-project-ed25519.priv.pem` (the private key). Add it to
`.gitignore`. Anyone with the private key can forge project signatures.

## 4. Record it

Fill in `CERTIFICATION-ICC-DISNEY.md` §6 with: signer = MEARVK project key,
signed digest = the §3 binding, proof files committed = the two files above, and
the verify command from §2. Optionally timestamp `binding.sig` per
`ICC/INTEGRITY.md` §4–§5.

---

*Procedure only; no signature bytes are fabricated here. The real signature is
produced with a private key on a capable machine and verified by standard tools.
Max Rupplin — MEARVK LLC — 2026.*
