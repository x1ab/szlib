# My personal ground-levelling & general toolbox, C++ "edition"

Or, the half-minute summary:

Lightweight & self-containing generic, common foundational stuff shared
across my projects, incl. e.g. C++ blanket/shim/ground-levelling helpers,
but also some generic utilities (or even abstractions, patterns etc.)
independent of prg. language. (Preferably header-only in C++, but if
a lib form can improve compilation times, that's OK too.)


## The main design criteria is _my own development speed_.

This is explicitly not meant for other users: it's _my_ personal toolbox, to maximize _my_ own efficiency. (Everyone should grow their own personal "thinking platform" anyway.)

This directive alone should kinda automatically imply a bunch of derived priorities, decisions, best-practice principles & techniques to apply.
And also some important freedoms: e.g. the API doesn't need be  "library-grade" flawless, complete, even consistent, as professional sharing is not a requirement. If it can help me in any way, that's already success.

0. The library should organically grow by adding items "generalized out" from existing projects. (See also 3.)

1. As a base rule, anything, anywhere in my projects is free to use this, if at all can.

2. **NO EXTERNAL DEPENDENCIES** beyond the already available facilities of the language or platform being used (e.g. `std::` for C++)! (But see also 7.)

3. Reusing by importing, embedding, digesting, internalizing... a useful external resource -- i.e. turning it from a dependency into an internal component -- is encouraged. (Keep licensing clearly traceable!)

4. Even the overarching "foundational" features (like I/O, debug/diagnostics, run-time error-handling suport, language shims etc.) should be _kept modular_, as much as possible, to allow selectively depending on them. (It could be problematic to formally encode such dependencies (e.g. comments go out of sync, external tools are overkill and not available everywhere; etc.), but I should try.)

5. Internal components may depend on other internal components, but those interdependencies must still be made explicit. (Maybe just the include path prefixes are enough for C++?)
To simplify config. mgmt. (i.e. skip version tracking) these interdependencies are only allowed within the same repo snapshot.

6. Shouldn't be the home of any subprojects, so they can live in their own standalone packages, but ...
[Alas, I've forgotten what the hell I meant by this exactly!... :-/ ]

7. ... can _proxy-include_ anything, for convenience. _(Assuming it would indeed add value compared to separetely including dependencies. I mean, it should be pretty obvious (that integrating one aggregate package should be better than adding several components individually), but... I can make it cumbersome! ;) )_
[Alas, I've forgotten what the hell I meant by this exactly!... :-/ ]

    -> _Try Git submodules! Maybe it's just the optimal setup._

8. Supplementing the lib itself, useful additional (and optional) stuff like examples, prj/dev env setup tools etc. can be added.

    -> _**Shouldn't** grow into some unwieldy knowledge-base-like monolithic catch-all repo though!_

9. A pratical level of compatibility should be maintained across versions. Also, sem. versioning is used both at the modules individually, and for the lib as a whole.

10. Targets (minimum):
    - MSVC
    - GCC (both via w64devkit and native Linux)
    - CLANG (native Linux)

------------------------------------------------------------------------------

See the TODO for notes on how to build and run unit tests, but for now, it's
best to just build them by hand...
