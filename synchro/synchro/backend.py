"""Dynamic backend loading and the ``@synchro`` annotation.

A Synchro *backend* is any object exposing a ``send(dest, payload, **opts)``
method (and optionally ``open()`` / ``close()``). Backends are resolved lazily
by name, so a function annotated with ``@synchro("...")`` does not import or
construct its transport until the first call — "dynamically loads the Synchro
backend for the network exchange".

Backend references use ``"module.path:AttributeName"`` (an entry-point-style
spec). A few built-in short names are registered for convenience.
"""

from __future__ import annotations

import functools
import importlib
import threading
from typing import Any, Callable, Dict, Optional, Protocol, runtime_checkable


class SynchroError(RuntimeError):
    pass


@runtime_checkable
class Backend(Protocol):
    """Structural type every Synchro backend satisfies."""

    def send(self, dest: Any, payload: bytes, **opts: Any) -> Any: ...


# Short-name registry -> "module:attr" specs. Kept intentionally small; anything
# importable can be referenced by its full dotted spec without registration.
_REGISTRY: Dict[str, str] = {
    "udp": "synchro.dispatcher:UdpDispatcher",
    "http2": "synchro.http2:MeteredHttp2Client",
    "http2-metered": "synchro.http2:MeteredHttp2Client",
}

_cache: Dict[str, Any] = {}
_lock = threading.Lock()


def register_backend(name: str, spec: str) -> None:
    """Register a short name -> ``'module:attr'`` spec."""
    _REGISTRY[name] = spec


def _resolve_spec(name: str) -> str:
    if name in _REGISTRY:
        return _REGISTRY[name]
    if ":" in name:
        return name
    raise SynchroError(
        f"unknown backend {name!r}; register it or use a 'module:attr' spec"
    )


def load_backend(name: str, *args: Any, cache: bool = True, **kwargs: Any) -> Any:
    """Dynamically import and instantiate a backend by name or spec.

    The import happens here, on demand — not at module import time. If ``cache``
    is true, a single instance per (name, args, kwargs) key is reused.
    """
    key = f"{name}|{args!r}|{sorted(kwargs.items())!r}"
    if cache:
        with _lock:
            if key in _cache:
                return _cache[key]

    spec = _resolve_spec(name)
    module_path, _, attr = spec.partition(":")
    if not module_path or not attr:
        raise SynchroError(f"invalid backend spec {spec!r} (want 'module:attr')")
    try:
        module = importlib.import_module(module_path)
        factory = getattr(module, attr)
    except (ImportError, AttributeError) as exc:
        raise SynchroError(f"could not load backend {spec!r}: {exc}") from exc

    instance = factory(*args, **kwargs) if (args or kwargs) else factory
    if cache:
        with _lock:
            _cache.setdefault(key, instance)
            return _cache[key]
    return instance


def clear_backend_cache() -> None:
    with _lock:
        _cache.clear()


def synchro(
    backend: str,
    *b_args: Any,
    inject_as: Optional[str] = "backend",
    cache: bool = True,
    **b_kwargs: Any,
) -> Callable[[Callable], Callable]:
    """Annotation that binds a function to a dynamically loaded Synchro backend.

    The decorated function receives the resolved backend instance as a keyword
    argument (named by ``inject_as``, default ``"backend"``) unless the caller
    supplied it explicitly. The backend is loaded on first call, so importing
    the decorated function costs nothing at import time.

    Example
    -------
    >>> @synchro("udp", [("127.0.0.1", 9999)])
    ... def probe(*, backend):
    ...     return backend.run(rounds=5)

    The attribute ``.synchro_backend`` on the wrapper names the bound backend,
    for introspection/tooling.
    """

    def decorator(func: Callable) -> Callable:
        @functools.wraps(func)
        def wrapper(*args: Any, **kwargs: Any) -> Any:
            if inject_as is not None and inject_as not in kwargs:
                kwargs[inject_as] = load_backend(
                    backend, *b_args, cache=cache, **b_kwargs
                )
            return func(*args, **kwargs)

        wrapper.synchro_backend = backend  # type: ignore[attr-defined]
        wrapper.synchro_backend_args = (b_args, b_kwargs)  # type: ignore[attr-defined]
        return wrapper

    return decorator
