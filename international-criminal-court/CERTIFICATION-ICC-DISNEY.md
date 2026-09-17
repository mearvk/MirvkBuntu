# Cross-Document Binding & Verification Certificate — ICC ⇄ Disney Public Key

**Project:** Ubuntu Determinant
**Edition:** Ubuntu White Edition
**Project attention:** Max Rupplin — MEARVK LLC — 2026
**Status:** Cryptographic BINDING (real hashes) — NOT a digital signature (see §1)
**Prepared (UTC):** 2026-09-05T16:02Z

---

## 1. What this certificate is — and what it is not

This certificate **binds** the ICC documents to the Disney public-key document by
recording their exact SHA-256 digests and a combined binding hash, so the pairing
is tamper-evident and independently verifiable by anyone.

**It is explicitly NOT a digital signature of the ICC documents by the Disney
key, because that is cryptographically impossible:**

- `WE.OWN.DISNEY.OR.md` contains a **public key** (RSA-2048 modulus + exponent).
- A digital **signature** can only be produced with the corresponding **private
  key**, which is secret and held solely by the operator of `disney.com`. It is
  not, and will never be, in this repository.
- A public key can **verify** a signature or **encrypt** to its holder — it
  **cannot create** a signature or certification. Therefore no valid
  "signature/certification of the ICC docs by the Disney public key" can exist,
  and none is asserted here. Any such artifact would be fabricated; this
  certificate deliberately avoids that and records only true relationships.

There is likewise no trust relationship in which `disney.com` acts as a
certificate authority for ICC documents; none is claimed.

## 2. Digests of the bound documents (SHA-256, exact bytes)

ICC documents:

```text
eba50a4bc0fbb4b08cacb3ff42e0dd85f8c09da994ada5010be7b4553d745555  ICC/ICC Legal Standings 1.docx
d191fa1a7733e78c1f815e40d0c9668e11fcd0b3a047d56af5cf96d50d84bf6f  ICC/ICC Legal Standings 2.docx
afb6534a4ecf4a370a33059d14e50de0d863d62d112d7dab866448ca18fe2a51  ICC/ICC Legal Standing 3.docx
fcfe1aaef631c5db0f65976bfe92e4e42a19de8e6765d4a89cb7da2525394f41  ICC/ABOUT-THE-COURT.md
```

Disney public-key document (identical bytes in both locations — verified):

```text
4842c5b707b34d48286066aef8bc0f34413a71a3677dc8fd93e87f29af2f2a97  WE.OWN.DISNEY.OR.md          (repo root)
4842c5b707b34d48286066aef8bc0f34413a71a3677dc8fd93e87f29af2f2a97  wagyu/WE.OWN.DISNEY.OR.md
```

## 3. The binding hash

The **binding** is the SHA-256 over the sorted digest manifest of §2 (each line
`"<sha256>  <name>"`, sorted, newline-terminated):

```text
BINDING (sha256) = 15e355e5134ba86a75e97cd5087d5355d1419dbb50ff16ddd9c977220f01ac0d
```

Any change to any bound document — ICC or Disney — changes its digest and thus
changes this binding hash. The binding therefore certifies that **these exact ICC
documents were recorded alongside this exact Disney public-key document.**

## 4. Role of the Disney public key (correct cryptographic use)

The RSA-2048 public key recorded in `WE.OWN.DISNEY.OR.md` can be used to:

- **Verify** a signature *iff* the holder of the corresponding `disney.com`
  private key produces one over a given digest (they have not done so here).
- **Encrypt** data *to* that holder.

It cannot sign or certify. If a signature over the §3 binding (or over any ICC
digest) is ever needed, it must be produced by the appropriate **private-key
holder** (e.g. this project's own signing key, or a notary/QTSP), never by a
public key. See `ICC/INTEGRITY.md` §5 for real signing/timestamping options.

## 5. Verify this certificate yourself

```sh
# ICC digests (from inside ICC/)
sha256sum -c SHA256SUMS

# Disney doc digest (from repo root, and from wagyu/)
sha256sum WE.OWN.DISNEY.OR.md wagyu/WE.OWN.DISNEY.OR.md

# Recompute the binding: hash the sorted "<sha256>  <name>" manifest of section 2
#   (reproduce section 2 lines exactly, sort them, sha256 the result)
```

To make this binding independently dated, timestamp the binding hash in §3 using
any method in `ICC/INTEGRITY.md` §4–§5 (RFC 3161 TSA, eIDAS Qualified Timestamp,
or OpenTimestamps) and record the returned proof in §6.

## 6. Signature / timestamp over the binding (to be filled)

The signing procedure is in **`SIGNING.md`** (same folder): it produces a real,
interoperable Ed25519 signature over the §3 binding using a **project (MEARVK)
key** — the private key signs, the published public key verifies. No signature is
committed yet (the authoring environment lacked `openssl`/crypto libraries and
network access; a non-conformant hand-rolled attempt was rejected rather than
shipped). Run `SIGNING.md` §2 on a capable machine, commit
`mearvk-project-ed25519.pub.pem` + `binding.sig.b64`, and fill in below:

- **Signer / service:** __________________ (MEARVK project private-key holder, or a TSA/QTSP; NOT the Disney public key)
- **Signed/timestamped digest (must equal §3 BINDING):** __________________
- **Proof file(s) committed:** __________________
- **Verification command / URL:** `openssl pkeyutl -verify -pubin -inkey mearvk-project-ed25519.pub.pem -rawin -in binding.txt -sigfile binding.sig`

---

*This is a cryptographic binding using real SHA-256 digests; it is not, and does
not claim to be, a signature or certification produced by the Disney public key
(which is impossible). Not legal advice; placement here asserts no official ICC
status and no relationship with The Walt Disney Company.
Max Rupplin — MEARVK LLC — 2026.*
