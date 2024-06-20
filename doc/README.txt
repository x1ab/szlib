  
  (Amazingly, after ~30 years in SW engineering, I find myself writing
  such a memo to myself, as a reminder, or focusing aid... Like a senile
  craftsman suddenly recognizing, again, the importance of a flat tabletop.
  I have no words. ;) )
  
  This should grow into my own personal "stdlib" that can be used by
  default (i.e.: without thinking twice) in any new (C++) project of mine.
  (An example I've recently seen is Jan Haller's Aurora, among infinite
  others.)
  
  Also: the "language" (concepts, IDs, features) defined here should be
  independent of C++ by nature, even tnough in this particular version
  (i.e. "unilang for C++") they obviously aren't. But it should still be
  considered in the context of the bigger picture, as a unified abstract
  language, independent of any particular *approximation* of it.
  
  OTOH, for any particular impl. language, specific extensions (that make
  sense only there) should also be added, if they are generic enough (in
  the context of that lang.) to make life easier across any projects.
  (But, obviously, separate languages (platforms, domains etc.) should
  have their own distinct repos to avoid the bloat & burden of dealing with every unused flavor for every language.)
  
  Also: there's no sharp distinction between lang. shims and a utility lib
  (IOW, this is what others often call the "foundation" or even (slightly
  confusingly) "platform" layer.)
  
  See also: https://github.com/x1ab/cpp/blob/main/WISHLIST.txt
