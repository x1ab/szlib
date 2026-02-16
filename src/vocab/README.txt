- Keep stuff here as lean and as stable as possible: this is where the API solidifies!

- No implementations in this layer!

- In agreement with Myco/vocab/README: sz/vocab might be better extracted
  into its own separate lib, as the dedicated common sz:: language, shared
  by all things sz::.

- To avoid "deep boostification" though (i.e. some of it is inevitable, and
  accepted), but keep it shallow, the constraints of "ADHD-Driven Development"
  still apply: vocab stuff can only include atomic type defs from within sz::.
  (And atomics can include nothing internal (to prevent cycles, etc.).)


/*!! Originally in lang/vocab, for fixints:

	If "vocab", then use it as a protocol; it should be defined better:
	e.g. what about lib-internals using it? If so, then this is both
	a leaf-level import to them, while root-level export to the user
	(via the API) — so... Should the top-level `vocab` include the
	sub-vocabs, or should the hierarchy inversion be explicitly decoupled?!
	(A structureless "ambient type soupe" is e.g. what happens in O2N
	currently, and, while seems viable, I'm not fond of that, especially
	not in a place like a foundations lib, like this. But it could be
	just prejudice/orthodoxy.)
!!*/
