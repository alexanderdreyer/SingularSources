proc indexOf(list l, int k)
"returns the index of k inside l if k is present; -1 otherwise;
l is assumed to be a flat list of integers"
{
  for (int i = 1; i <= size(l); i = i + 1)
  {
    if (l[i] == k) { return (i); }
  }
  return (-1);
}

proc haveSameGenerators(ideal ii, ideal jj)
"checks whether the LISTS of generators of the two ideals are identical;
so this method does not just check whether the SETS of
generators are identical but takes duplicates into account;
returns 1 in case of equality, 0 otherwise"
{
  int n = size(ii);
  if (n != size(jj)) { return (0); }

  list l; // Herein, we will save indices of polynomials in jj
          // that have been used for matching with polynomials of ii.
          // So far, there are no matches. ;-)
  poly f;
  poly g1;
  poly g2;
  int foundF;
  int i;
  int j;
  for (i = 1; i <= n; i = i + 1)
  {
    f = ii[i];
    foundF = 0;
    // try to find f in jj:
    for (j = 1; j <= n; j = j + 1)
    {
      g1 = f - jj[j];
      if (g1 == poly(0))
      {
        // we found ii[i] in jj; but maybe we have used this
        // polynomial before for matching with one of ii's polynomials
        if (indexOf(l, j) == -1)
        {
          // now we know that we did not use jj[j] before
          foundF = 1;
          l = insert(l, jj);
          break;
        }
      }
    }
    if (foundF == 0) { return (0); }
  }
  return (1);
}

proc haveSameLeadingTerms(ideal ii, ideal jj)
"checks whether the LISTS of leading terms given by the lists
of generators of the two ideals are identical;
so this method does not just check whether the SETS of
leading terms are identical but takes duplicates into account;
returns 1 in case of equality, 0 otherwise"
{
  int n = size(ii);
  if (n != size(jj)) { return (0); }

  list l; // Herein, we will save indices of polynomials in jj
          // that have been used for matching with polynomials of ii.
          // So far, there are no matches. ;-)
  poly f;
  poly g1;
  poly g2;
  int foundF;
  int i;
  int j;
  for (i = 1; i <= n; i = i + 1)
  {
    f = lead(ii[i]);
    foundF = 0;
    // try to find f in jj:
    for (j = 1; j <= n; j = j + 1)
    {
      g1 = f - lead(jj[j]);
      if (g1 == poly(0))
      {
        // we found lead(ii[i]) in lead(jj); but maybe we have used this
        // polynomial before for matching with one of ii's polynomials
        if (indexOf(l, j) == -1)
        {
          // now we know that we did not use jj[j] before
          foundF = 1;
          l = insert(l, jj);
          break;
        }
      }
    }
    if (foundF == 0) { return (0); }
  }
  return (1);
}

ring r = (integer, 2, 6), (x, y, z), dp;
ideal I = system("vanishingIdeal", "direct");
ideal J = system("vanishingIdeal", "recursive");
"next output must be 1:";
haveSameGenerators(I, J);
"next output must be 1:";
haveSameLeadingTerms(I, groebner(I));
poly g = 17x-yz+z;
poly f = J[33] + g;
poly h = system("vanishingIdeal", "normalForm", f);
"next output must be", g;
h;
"next output must be", g;
reduce(f, I);
"next output must be", g;
reduce(f, J);
int i = system("vanishingIdeal", "smarandache", 11*13*337);
"next output must be 337";
i;
f = xz*J[33] + 17*I[14] -z3*J[31];
g = f + x*(x-1)*(x-2)*y*(y-1)*z*(z-1)*(z-2)*(z-3);
i = system("vanishingIdeal", "isZeroFunction", f);
"next output must be 1";
i;
i = system("vanishingIdeal", "isZeroFunction", g);
"next output must be 0";
i;
intvec ii = system("vanishingIdeal", "nonZeroTuple", f);
"next output must be -1, -1, -1";
ii;
ii = system("vanishingIdeal", "nonZeroTuple", g);
"next output must be 3, 2, 4";
ii;
"next output must be <>0";
subst(g, x, 3, y, 2, z, 4);
"try this:";
"system(\"vanishingIdeal\", \"usage\");";
