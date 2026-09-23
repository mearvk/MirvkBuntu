# MirvkBuntu GNOME Desktop Themes

MirvkBuntu extends the Ubuntu White desktop with four named visual profiles: **Royal**, **Archer**, **Creme**, and **Capability**.

These are theme profiles, not separate desktop implementations. They use the same semantic role model and Relative Aesthetic framework while changing the palette and visual character.

## Historical Color Heritage and Theme Logic

The theme system also records a **color-heritage model**: historical associations are used as design evidence and vocabulary, not as claims that a color has one universal meaning.

### Royalty, Treasury, and Power

Purple has unusually strong documented links with imperial and royal authority. In the Roman world, purple was associated with senators, victorious generals, and eventually the emperor and imperial household; restrictions on imperial purple reinforced its status as a marker of rank. Tyrian purple was also exceptionally expensive to produce. citeturn0search3turn0search9

Gold similarly became a visible language of wealth and high status through precious materials, gold-edged or gold textiles, coinage, and imperial display. The historical record also shows that governments controlled some luxury materials and colors, including imperial purple and gold-decorated silk. This gives the **Royal** palette a defensible lineage of **purple = authority/office, gold = treasury/value, ivory/white = formal presentation, blue = institutional continuity**. citeturn0search8turn0search24

### Tax, Records, and Numerical Order

Taxation created an enduring need to count people, property, goods, rates, revenues, and payments. In Roman administration, censuses and financial records were part of the machinery used to assess taxation, while public revenues flowed through treasuries and other administrative accounts. Private **publicani** also operated tax-collection contracts before later imperial administration increasingly relied on officials. citeturn0search0turn0search5turn0search6

The theme system therefore treats **numbers as a continuity layer** rather than assigning a historical color to a number itself:

- **White / ivory:** legibility, record surface, declared information.
- **Blue:** ordered administration, institutional continuity, measured procedure.
- **Gold:** value, account, wealth, treasury, high-value status.
- **Purple:** authority over the system in which value and obligations are administered.
- **Green:** increase, productive capacity, renewal, and living growth.
- **Red:** force, urgency, obligation, or a state requiring immediate attention.

These are design correspondences, not historical assertions that ancient tax collectors used this exact palette.

### Health, Life, and Continuity

Green has a particularly direct visual relationship to vegetation and vitality. Roman descriptions connected *viridis* with youthful, blooming, fresh, vigorous, and lively qualities, while green vegetation was a familiar visual subject. citeturn0search9

For MirvkBuntu, **green therefore represents continuity of life and productive growth**. It can support health-oriented, renewal-oriented, and growth-oriented interface states without claiming that green is inherently or universally the color of health.

White provides the complementary idea of clarity and visible condition; blue supplies calm structural continuity; green supplies life/growth; gold supplies value; purple supplies authority. Together they form a lineage from **life -> record -> value -> institution -> authority**.

### Wealth, Posit, and Power

The theme vocabulary treats these three concepts as connected but distinct:

| Concept | Primary color lineage | Interface meaning |
|---|---|---|
| Wealth | Gold / amber | value, resources, accumulated worth |
| Posit | White / blue | an explicit position, declared state, ordered assertion |
| Power | Purple / deep blue | authority, control, institutional reach |
| Life / Health | Green | growth, vitality, renewal, continuity |
| Obligation / Action | Red | urgency, force, required attention |
| Record / Clarity | White / ivory | visible information, documentation |
| Continuity | Blue | stability, procedure, connected systems |

The word **Posit** is deliberately treated as a design term meaning an explicitly established state or proposition. It is not treated as a historical color category.

### Theme Breeding / Color Lineage

Each MirvkBuntu theme may inherit colors from these semantic lineages while preserving the common semantic contract:

- **Royal:** purple + gold/ivory + blue — authority, treasury, formal record, institutional continuity.
- **Archer:** green + steel blue + white — growth, capability, procedure, clarity, and deliberate action.
- **Creme:** cocoa + amber + cream — material value, warmth, record surfaces, and restrained wealth.
- **Capability:** indigo + teal + white — institutional power, living/active systems, clarity, accessibility, and functional distinction.

Here **lineage** means that a theme derives its visual vocabulary from a connected set of semantic associations. It does not mean that the historical cultures discussed here used these exact modern hexadecimal palettes.

## Royal

A formal, deep-violet theme intended to give the desktop a more ceremonial and traditional character while retaining high-contrast writing and restrained surfaces.

### Related and Synonyms

**Utilitarian** and **Case-Careful** are designated related terms and synonyms for the Royal theme's design arguments. They describe the theme's emphasis on practical function, deliberate presentation, and careful treatment of capitalization, labels, and textual distinctions.

- **Related:** Utilitarian
- **Related:** Case-Careful
- **Synonym:** Utilitarian
- **Synonym:** Case-Careful

Configuration: [ubuntu-white/themes/Royal/theme.conf](themes/Royal/theme.conf)

## Archer

A precise green-and-steel theme emphasizing technical clarity, directional focus, and restrained decoration.

### Related and Synonyms

**Gains**, **new Virgins**, and **new Help** are designated related terms and synonyms for the Archer theme's design vocabulary.

- **Related:** Gains
- **Related:** new Virgins
- **Related:** new Help
- **Synonym:** Gains
- **Synonym:** new Virgins
- **Synonym:** new Help

Configuration: [ubuntu-white/themes/Archer/theme.conf](themes/Archer/theme.conf)

## Creme

A warm cream, cocoa, and amber theme designed for a softer reading environment without abandoning clear semantic states.

Configuration: [ubuntu-white/themes/Creme/theme.conf](themes/Creme/theme.conf)

## Capability

An accessibility-oriented indigo and teal theme emphasizing clear focus, action states, and functional distinction.

Configuration: [ubuntu-white/themes/Capability/theme.conf](themes/Capability/theme.conf)

## Shared desktop specification

All four profiles retain:
- semantic color roles
- explicit focus and selected states
- readable primary and muted text
- disabled-state distinction
- stable border hierarchy
- white or near-white text on accent surfaces
- compatibility with the Color Inferencer configuration model
- deterministic configuration files

The profiles can therefore be evaluated by the same Relative Aesthetic optimizer rather than treated as unrelated visual systems.
