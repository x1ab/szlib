# My personal C++ ground-levelling & general toolbox lib

## The main design criteria is _my own development speed_. 
I shouldn't care what others would prefer; this is _my_ personal toolbox, to maximize _my_ own efficiency. _(Every serious dev. grows their own personal platform anyway.)_

This alone should imply a bunch of (derived) priorities, (best-practice) principle, techniques etc., so they don't need to be enumerated individually; they would come up.

1. **NO EXTERNAL DEPENDENCIES beyond `std::`!**

2. Internal components _may_ depend on other _internal_ components, but the interdependencies must be explicit.

3. Even the overarching "foundational" features (like I/O, debug/diagnostics, run-time error-handling suport, language shims etc.) should be kept modular, as much as possible, to allow selectively depending on them. (It could be problematic to _encode_ such dependencies (comments go out of sync!), but I should try.)

4. Shouldn't be the _home_ of other projects, so they can live in their own standalone packages, but ...

5. ... can _proxy-include_ anything, for convenience. _(Assuming it would indeed add value compared to separetely including dependencies. I mean, it should be pretty obvious (that integrating one aggregate package should be better than adding several components individually), but... I can make it cumbersome! ;) )_

    -> _Try Git submodules! Maybe it's just the optimal setup._

6. Apart from the lib itself, might add useful tools, example prj./dev env setup components etc.

    -> _**Shouldn't** grow into some unwieldy knowledge-base-like monolithic catch-all repo though!_

7. Targets (minimum):
    - MSVC
    - GCC (both via w64devkit and native Linux)
    - CLANG (native Linux)