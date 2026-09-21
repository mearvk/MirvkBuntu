# Writing and Loading Synchro Backends

A Synchro **backend** is any object that performs a network exchange. Backends
are resolved **dynamically** — imported and constructed on first use — so the
`@synchro` annotation adds no import-time cost and lets you swap transports
without touching call sites.

## The backend protocol

A backend satisfies the structural type in `backend.py`:

```python
@runtime_checkable
class Backend(Protocol):
    def send(self, dest, payload: bytes, **opts): ...
```

`send(...)` is the minimum. A backend may also expose `open()` / `close()`
(and act as a context manager) if it holds resources such as a socket or an
HTTP connection pool — `MeteredHttp2Client` does exactly this.

The two built-in backends go a little beyond the minimum:

| Short name | Class | Notes |
|------------|-------|-------|
| `udp` | `synchro.dispatcher:UdpDispatcher` | `send()`-style probing via `run(rounds=…)`. |
| `http2` / `http2-metered` | `synchro.http2:MeteredHttp2Client` | `send(url)` performs one metered request. |

## Referencing a backend

`load_backend()` accepts either:

- a **short name** registered in the internal registry (`"udp"`, `"http2"`), or
- an **entry-point-style spec** `"module.path:Attribute"`, e.g.
  `"mypkg.transports:QuicDispatcher"`.

```python
from synchro import load_backend

udp = load_backend("udp", [("127.0.0.1", 9999)])          # short name + args
quic = load_backend("mypkg.transports:QuicDispatcher")     # dotted spec
```

The import happens inside `load_backend()` (via `importlib.import_module`),
never at module import time. If the module or attribute cannot be found, a
`SynchroError` is raised with the failing spec.

### Instantiation rules

- If you pass constructor `*args`/`**kwargs`, `load_backend` calls the resolved
  attribute as a factory: `factory(*args, **kwargs)`.
- If you pass **no** args, the resolved attribute is returned **as-is** (so a
  spec can point at a module-level singleton or a callable you don't want
  auto-instantiated).

### Caching

By default instances are cached per `(name, args, kwargs)` key behind a lock, so
repeated resolutions share one instance. Pass `cache=False` for a fresh
instance, and call `clear_backend_cache()` to reset (useful in tests).

## Registering a short name

```python
from synchro.backend import register_backend

register_backend("quic", "mypkg.transports:QuicDispatcher")
load_backend("quic", host="10.0.0.1")   # now resolvable by short name
```

## The `@synchro` annotation

`@synchro(...)` binds a function to a lazily loaded backend and injects it as a
keyword argument (default name `backend`) unless the caller passes one
explicitly:

```python
from synchro import synchro

@synchro("udp", [("127.0.0.1", 9999)], window=4096)
def probe(*, backend):
    return backend.run(rounds=500)

report = probe()                 # backend imported + built on this first call
report = probe(backend=my_own)   # explicit backend wins; no loading happens
```

Introspection attributes are attached to the wrapper:

- `probe.synchro_backend` → the backend name/spec (`"udp"`).
- `probe.synchro_backend_args` → the `(args, kwargs)` used to build it.

Set `inject_as=None` if you want the annotation to resolve/warm the backend
policy but not inject it (e.g. when the function pulls the backend itself).

## A minimal custom backend

```python
# mypkg/transports.py
import time
from synchro.stats import LatencyStats, Sample

class LoopbackNull:
    """Trivial backend: 'sends' nothing, records a zero-RTT sample."""
    def __init__(self, dest="null"):
        self.stats = {dest: LatencyStats(dest)}
        self.dest = dest
        self._seq = 0

    def send(self, dest=None, payload: bytes = b"", **opts):
        self._seq += 1
        self.stats[self.dest].record(
            Sample(self.dest, 0.0, time.monotonic(), self._seq)
        )
        return self.stats[self.dest]
```

```python
from synchro import synchro

@synchro("mypkg.transports:LoopbackNull")
def ping(*, backend):
    return backend.send()
```

Because it produces `Sample`s into a `LatencyStats`, the custom backend works
with `SlaReporter` and the rest of the stack with no further wiring.
