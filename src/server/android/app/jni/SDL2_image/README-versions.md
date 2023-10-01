# Versioning

## Since 2.5.0

`SDL_image` follows an "odd/even" versioning policy, similar to GLib, GTK, Flatpak
and older versions of the Linux kernel:

* The major version (first part) increases when backwards compatibility
    is broken, which will happen infrequently.

* If the minor version (second part) is divisible by 2
    (for example 2.6.x, 2.8.x), this indicates a version that
    is believed to be stable and suitable for production use.

    * In stable releases, the patchlevel or micro version (third part)
        indicates bugfix releases. Bugfix releases should not add or
        remove ABI, so the ".0" release (for example 2.6.0) should be
        forwards-compatible with all the bugfix releases from the
        same cycle (for example 2.6.1).

    * The minor version increases when new API or ABI is added, or when
        other significant changes are made. Newer minor versions are
        backwards-compatible, but not fully forwards-compatible.
        For example, programs built against `SDL_image` 2.6.x should work fine
        with 2.8.x, but programs built against 2.8.x will not necessarily
        work with 2.6.x.

* If the minor version (second part) is not divisible by 2
    (for example 2.5.x, 2.7.x), this indicates a development prerelease
    that is not suitable for stable software distributions.
    Use with caution.

    * The patchlevel or micro version (third part) increases with
        each prerelease.

    * Each prerelease might add new API and/or ABI.

    * Prereleases are backwards-compatible with older stable branches.
        For example, 2.7.x will be backwards-compatible with 2.6.x.

    * Prereleases are not guaranteed to be backwards-compatible with
        each other. For example, new API or ABI added in 2.5.1
        might be removed or changed in 2.5.2.
        If this would be a problem for you, please do not use prereleases.

    * Only upgrade to a prerelease if you can guarantee that you will
        promptly upgrade to the stable release that follows it.
        For example, do not upgrade to 2.5.x unless you will be able to
        upgrade to 2.6.0 when it becomes available.

    * Software distributions that have a freeze policy (in particular Linux
        distributions with a release cycle, such as Debian and Fedora)
        should usually only package stable releases, and not prereleases.

## Before 2.5.0

Older versions of `SDL_image` used the patchlevel (micro version,
third part) for feature releases, and did not distinguish between feature
and bugfix releases.
