# `mf` — Ubuntu White File Metadata Modifier

`mf` is the write-side adjunct to `lf`. It addresses file metadata without editing the file payload.

## Flags

```text
mf [options] FILE
  --name NAME          metadata filename/display name
  --date DATE          metadata date
  --author AUTHOR      metadata author
  --database NAME      metadata database
  --rating NAME VALUE  one of the 18 generic ratings
  --health VALUE       Sense overall health
  --sense 1|2|3        Sense layer to modify
  --hold TYPE          Hold type
  --schema             display the schema
```

A command may select one Sense layer and modify one or more fields in that layer. `--rating` accepts exactly the defined generic rating names.

## Metadata boundary

The prototype validates operations but does not yet write a native EXT4 record. A production implementation must use an atomic, versioned metadata backend and preserve file contents. Failed writes must leave the prior metadata intact.

## Authorization

Changing metadata is an administrative operation. The implementation must use the normal OS permission model and must not elevate privileges merely because `mf` is installed with the base system.

## Safety

- `mf` never treats ratings as executable instructions.
- It must not infer sensitive personal characteristics from metadata.
- Invalid Sense numbers and unknown rating names are rejected.
- Unknown future metadata fields must be preserved by the backend.
- File payload, inode ownership, and filesystem timestamps must not be changed merely by changing these metadata fields unless an explicit, separately defined operation requests it.
