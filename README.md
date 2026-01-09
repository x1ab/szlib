## My personal "random toolbox" lib _(C++ "edition")_

### Half-minute summary:

Lightweight, self-containing, generic, common "foundations stuff" shared
across my projects; incl. e.g. C++ shim/ground-levelling helpers, or various
generic utilities (or even abstractions, patterns, algorithms, etc.),
independent of prg. language. (In case of C++, it's mostly header-only,
plus some precompilable TUs (e.g. for #including in unity builds) to improve
compilation times, and/or for simplicity.)


### Main design criteria: improving _my own_ dev. efficiency

Everyone should have their own personal workshop; or grow their own personal
"thinking platform". NIH is an overrated meme; not every paraphrase of some
existing expression is "reinventing the wheel". Painters keep painting trees,
and nobody questions *that*. 
Sometimes your unique personal needs, taste, or just *mere circumstances* may
warrant putting something together _just the way **you** like it_, often also
faster than doing the extensive research + trial/eval + learning required to
find (and then keep using) the best tool out there somewhere.

So, this lib is explicitly not meant to be used by others: this is *my own*
personal toolbox, for *my own* quirks.

_(Why is it not a private repo then? Well, why should it be? We can still be
interested in each other's way of working/thinking, even if we don't intend
to copy it as-is. Blog posts are even more personal, and they are definitely
not kept private either.)_


This direction alone should kinda automatically imply a bunch of derived priorities,
decisions, best-practice principles & techniques to apply. And also some *important
freedoms*: e.g. the API doesn't need be flawless, "library-grade", complete,
even consistent, as professional sharing is not a priority, beyond sharing it
with my future self. And if it can help *me, in any way*, that's already success.


### A few reminders for myself:

0. The lib should organically grow by adding items "generalized out" from
   existing projects. (See also 3.)

1. As a base rule, anything, anywhere in my projects is free to use this,
   if at all can.

2. **NO EXTERNAL DEPENDENCIES** beyond the already available facilities of the
   language/platform already used (e.g. `std::` for C++)! (But see also 7.)

3. OTOH, reusing by importing, embedding, digesting, internalizing, "vendoring"
   a useful external resource — i.e. turning it from a *dependency* into an
   internal *component* — is encouraged. (Keep licensing clearly traceable!)

4. Even the overarching "foundational" features (like I/O, debug/diagnostics,
   run-time error-handling, language shims etc.) should be kept *modular*,
   as much as possible, to allow selectively depending on them. (It could be
   problematic to formally encode such dependencies (e.g. comments go out of
   sync, external tools are overkill, and also not available everywhere; etc.),
   but I should try.)

5. Internal components may depend on other internal components, but those
   interdependencies must still be kept explicit. (Maybe just the include
   path prefixes are enough for C++?)
   To simplify config. mgmt. (i.e. skip version tracking) these interdeps.
   are only allowed within the same repo snapshot. _[What did I even mean by this though?!...]_

6. Shouldn't be the home of any *subprojects*, so they can live in their own
   standalone packages, but ... _[Alas, I've forgotten what the hell I meant to say here, too... :-/ ]_

7. It can still *proxy-include* things, for convenience, if I can come up
   with a clean way to offer that as an optional service via some packaging
   conventions/APIs. But this might be better done by a completely different
   project (and repo).
   _(I mean, it should be pretty obvious that integrating one "virtual"
   aggregate package should be better than adding several components
   individually), but... I can make it cumbersome! ;) So, shouldn't...
   please, dear me.)_

    -> _Try Git submodules! Maybe it's just the optimal setup for that,
       despite the horror stories?_

8. Supplementing the lib itself, useful additional (and optional) stuff like
   examples, prj/dev env setup tools etc. can also be added.

    -> _**Shouldn't grow** into some unwieldy knowledge-base-like monolithic catch-all repo though!_

9. A practical level of compatibility should be maintained across versions.
   Also, sem. versioning is used (not very strictly!) both at the modules
   individually, and also at the lib level (separately).

10. Targets (minimum):

  - MSVC 17
  - GCC 15 via w64devkit
  - GCC 15 (Linux)
  - CLANG 19 (Linux)

11. Tooling:

  - GNU Make
  - Busybox (at least on Windows)
  - For the C++ version: C++20 (or probably even 23)


------------------------------------------------------------------------------

_(See also the old [TODO](https://github.com/x1ab/szlib/issues/4) for obsolete notes about
building and unit-testing... No, sorry, changed my mind. Don't see it.)_
