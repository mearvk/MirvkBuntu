# MirvkBuntu GNOME Themes

MirvkBuntu provides four additional GNOME desktop theme profiles built on the same semantic color contract as the Ubuntu White baseline.

| Theme | Character | Surface | Primary | Secondary |
|---|---|---|---|---|
| Royal | formal, deep, high-contrast | ivory | royal violet | blue |
| Archer | precise, restrained, technical | white | forest green | steel blue |
| Creme | warm, soft, refined | cream | cocoa | amber |
| Capability | functional, accessible, energetic | white | indigo | teal |

Each theme keeps semantic roles stable: surface, text, muted text, disabled text, primary action, secondary/focus, border, and on-accent. Exact values are theme parameters rather than changes to the underlying semantic model.

## Color Heritage and Logic Holdings

The theme family uses a historical color-heritage layer to inform visual vocabulary. It is a design model, not a claim that colors have universal meanings.

- **Royalty / power:** purple has documented imperial and royal associations, especially in Roman and later European contexts.
- **Wealth / treasury:** gold and amber provide a visual lineage for value, precious material, and accumulated resources.
- **Record / number:** white and ivory support the visual idea of legible records; blue supports ordered administration and continuity.
- **Life / health:** green supplies the lineage of vegetation, youth, vitality, renewal, and productive growth.
- **Obligation / action:** red provides a strong signal for urgency, force, or required attention.
- **Posit:** white/blue are used for an explicitly established or declared state; this is a MirvkBuntu design definition rather than a historical color category.

Taxation and tax collection are important to the numerical lineage because historical administrations needed counts, censuses, property assessments, rates, revenues, and financial records. Roman evidence documents both private tax contractors (*publicani*) and increasingly centralized collection and accounting systems. citeturn0search0turn0search5turn0search6

The resulting design chain is:

**Life / Growth → Record / Number → Value / Wealth → Institution / Administration → Authority / Power**

The four themes inherit different portions of this chain:

- **Royal:** purple + ivory + blue — authority, treasury/value, formal record, institutional continuity.
- **Archer:** green + steel blue + white — growth, capability, procedure, clarity, deliberate action.
- **Creme:** cocoa + amber + cream — material value, warmth, record surfaces, restrained wealth.
- **Capability:** indigo + teal + white — institutional power, active systems, clarity, accessibility.

These relationships guide color selection and blending while preserving the common semantic roles and Relative Aesthetic optimizer.

Theme configuration files:
- [Royal](Royal/theme.conf)
- [Archer](Archer/theme.conf)
- [Creme](Creme/theme.conf)
- [Capability](Capability/theme.conf)

The existing [MirvkBuntu-White](../GNOME.COLOR.CONFIG.md) configuration remains the baseline desktop profile.

## Broad Color Pairings

Theme colors can be composed in broad historical-semantic pairings rather than selected independently. Marriage, family alliance, wealth, titles, and institutions have repeatedly appeared together in material culture; for example, elite Japanese wedding objects connected marriage with rank, political alliance, family crests, and gold decoration. ([The Met: Japanese Weddings in the Edo Period](https://www.metmuseum.org/zh/essays/japanese-weddings-in-the-edo-period))

MirvkBuntu uses this as a design grammar:

| Pairing | Theme meaning |
|---|---|
| White / ivory + gold + purple | title, rank, authority, ceremonial institution |
| White + gold + blue | agreement, administration, established office |
| White + gold + green | household continuity, growth, productive life |
| White + gold + red | declaration, obligation, consequential action |
| White + blue | neutral record becoming an institutional assertion |
| White + purple | title or authority emerging from neutrality |

## TitleWork and Neutrality

**TitleWork** is the theme layer used when color participates in title hierarchy. Purple, indigo, and deep blue can establish title distinction; gold/amber can provide a restrained value or ceremonial accent; white/ivory provides the formal field; dark neutral text protects readability. Historical heraldry similarly used a structured vocabulary of metals and tinctures and relied on contrast for legibility. ([The Met: Medieval Art: A Resource for Educators](https://resources.metmuseum.org/resources/metpublications/pdf/Medieval_Art_A_Resource_for_Educators.pdf))

The **White + White + White** model is the family's neutral mathematical reference. It represents three closely related white roles—surface, structure, and neutral field—before a semantic color departs from that baseline. The farther a controlled accent moves perceptually from that neutral field, the more categorical information it can carry, subject to contrast, accessibility, hierarchy, and restraint constraints.

Thus the theme family uses a connected grammar:

**Neutrality → Distinction → Title → Institution → Value → Authority**

and a parallel continuity grammar:

**Neutrality → Life → Growth → Household → Institution → Continuity**

These are configurable MirvkBuntu design relationships, not universal claims about color psychology.
