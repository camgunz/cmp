Why CMP uses a single-number versioning system
==============================================

There are lots of versioning systems, one example uses the three-number system
-- like Python -- to give users some information just by looking at the
version, X.X.X.  Changing the 3rd number means something was fixed, changing
the 2nd number means something was added, changing the 1st number means
something was broken.

I'm not confident in saying that version X.N.N is compatible in every way with
version X.M.M.  Every release of CMP is a major-version release and should be
treated as such.  On the bright side, at least you'll never see something like
CMP v2.576.30 (what?)

