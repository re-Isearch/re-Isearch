#include "fuzzy.hxx"
#include <vector>

extern "C" double sqrt(double x);

static int minimum(int a, int b, int c)
{
  return (a > b) ? (( b > c ? c : b ) ): (a >c ? c : a);
}

typedef unsigned char LTYPE;

static LTYPE *d = NULL;
static size_t d_size = 0;

// Max 65K memory
static size_t allocate(size_t m, size_t n)
{
#define HEADROOM(_x,_y) (((_x)/(_y) + 1)*(_y))
  size_t  need = m*n;
  if (need > d_size)
    {
      size_t       want = HEADROOM(need*sqrt(need),1024);

      if (want > 30000) want = 256*256;
      if (d) free(d);
      if ((d = (LTYPE *)malloc((sizeof(LTYPE))*want)) != NULL)
	d_size = want;
      else
	d_size = 0;
    }
  if (d_size)
    {
      // Need to Init the memory
      LTYPE k;
      for(k=0;k<n;k++) d[k]=k;
      for(k=0;k<m;k++) d[k*n]=k;
    }
  return d_size;
}

// We will support a max. of 256 characters per string

static int levenshtein_distance(const char *s, size_t n, const char *t, size_t m)
{
  //Step 1
  if(n!=0&&m!=0)
  {
    char s_i, t_j;
    // Step 2: Allocate
    if (allocate(++m, ++n) == 0)
       return -2; // Error
    //Step 3 and 4	
    for(size_t i=1;i<n;i++)
      for(size_t j=1;j<m;j++)
	{
        //Step 5
	const int cost = (s_i = s[i-1])==(t_j = t[j-1]) ? 0 : 1;
        //Step 6			 
        LTYPE cell =(LTYPE)minimum(d[(j-1)*n+i]+1,d[j*n+i-1]+1,d[(j-1)*n+i-1]+cost);
#if 1
        if (i>2 && j>2) {
	  LTYPE trans=d[(j-2)*n + (i-2)]+1;
	  if (s_i!=t[j-2]) trans++;
	  if (t_j!=t[i-2]) trans++;
	  if (cell>trans) cell=trans;
	}
#endif
	d[j*n+i]= cell;

      }
    return d[n*m-1]; // distance
  }
  else 
    return -1; //a negative return value means that one or both strings are empty.
}

int levenshtein_distance(STRING& source, const STRING& target)
{
  return  levenshtein_distance (source.c_str(), source.length(), target.c_str(), target.length()) ;
}

int LevenshteinDistance(const STRING& source, const STRING& target)
{
  const size_t n = source.length();
  const size_t m = target.length();

  // Step 1
  if (n == 0) return m;
  if (m == 0) return n;

  if (n < 256 && m < 256)
   levenshtein_distance(source.c_str(), n, target.c_str(), m);
    

  // Good form to declare a TYPEDEF
  typedef std::vector< std::vector<int> > Tmatrix; 
  Tmatrix matrix(n+1);

  // Size the vectors in the 2.nd dimension. Unfortunately C++ doesn't
  // allow for allocation on declaration of 2.nd dimension of vec of vec
  for (size_t i = 0; i <= n; i++) matrix[i].resize(m+1);

  // Step 2
  for (size_t i = 0; i <= n; i++) matrix[i][0]=i;
  for (size_t j = 0; j <= m; j++) matrix[0][j]=j;

  // Step 3

  for (size_t i = 1; i <= n; i++) {

    const unsigned char s_i = source.GetChr(i);

    // Step 4

    for (size_t j = 1; j <= m; j++) {

      const unsigned char t_j = target.GetChr(j);

      // Step 5
      int cost = (s_i == t_j) ? 0 : 1;

      // Step 6

      const int above = matrix[i-1][j];
      const int left = matrix[i][j-1];
      const int diag = matrix[i-1][j-1];

#define min(x,y) ((x)>(y)?(y):(x))

      int cell = min( above + 1, min(left + 1, diag + cost));

      // Step 6A: Cover transposition, in addition to deletion,
      // insertion and substitution. This step is taken from:
      // Berghel, Hal ; Roach, David : "An Extension of Ukkonen's 
      // Enhanced Dynamic Programming ASM Algorithm"
      // (http://www.acm.org/~hlb/publications/asm/asm.html)

      if (i>2 && j>2) {
        int trans=matrix[i-2][j-2]+1;
        if (source.GetChr(i-1)!=t_j) trans++;
        if (s_i!=target.GetChr(j-1)) trans++;
        if (cell>trans) cell=trans;
      }

      matrix[i][j]=cell;
    }
  }

  // Step 7
  return matrix[n][m];
}





// Performs a 'fuzzy' comparison between two strings.  Returns how
// 'alike' they are expressed as a percentage match.
//
// Written originally by John W. Ratcliff for Dr. Dobbs Journal
// of Software Tools Volume 13, 7, July 1988
//
// Pages 46, 47, 59-51, 68-72
// http://www.ddj.com/184407970?pgno=5
//

static int GCsubstr(const char *st1,const char *end1,const char *st2,const char *end2)
{
  const char *s1 = NULL;
  const char *s2 = NULL;
  int max = 0;
  const char *b1 = end1;
  const char *b2 = end2;
  int i;
	const char *a1;
	const char *a2;


  if (end1 <= st1) return 0;
  if (end2 <= st2) return 0;
  if (end1 == (st1+1) && end2 == (st2+1) ) return 0;


  for (a1 = st1; a1 < b1; a1++)
  {
    for (a2 = st2; a2 < b2; a2++)
    {
      if (*a1 == *a2)
			{

				for (i=1; a1[i] && (a1[i] == a2[i]); i++);

        if (i > max)
				{
					max = i; s1 = a1; s2 = a2;
					b1 = end1 - max; b2 = end2 - max;
				}

			}
    }
  }

  if (!max) return 0;

  max += GCsubstr(s1+max, end1, s2+max, end2);
  max += GCsubstr(st1, s1, st2, s2);

  return max;
}


int RatcliffCompare(const STRING &s1, const STRING& s2, int scale)
{
  size_t  l1 = s1.length();
  size_t  l2 = s2.length();
  const char *p1 = s1.c_str();
  const char *p2 = s2.c_str();

  int distance = LevenshteinDistance (s1, s2);
  if (distance == 0) return scale;

  int metric =  GCsubstr(s1, p1+l1, s2, p2+l2);

  return(2 * scale * metric/ (l1+l2));
}

#ifdef TEST
int main(int argc, char **argv)
{
  STRING s1, s2;
  if (argv[1])
    {
      s1 = argv[1];
      if (argv[2]) s2 = argv[2];
    }
  int ratcliff =  RatcliffCompare(s1,s2, 100);
  double r_factor   = ((double)ratcliff)/100.0;
  int  r_dist       = (int) ((s1.length() * (1 - r_factor)) + 0.5);
  
  printf("Distance(\"%s\",\"%s\")-> %d versus %d (%d%%,%d)\n", 
	s1.c_str(), s2.c_str(), LevenshteinDistance (s1, s2),
	levenshtein_distance(s1, s2),
	ratcliff, r_dist);
  if (s2.GetLength()) return 0;

  char tmp[256];
  int terms = 0;
  FILE *fp = fopen("/usr/share/dict/words", "r");
  clock_t  start = clock();
  while (fgets(tmp, sizeof(tmp), fp) != NULL)
    {
      s2 = s1;
      s1 = tmp;
      s1.Pack();
      terms++;
#if 0
      LevenshteinDistance (s1, s2) ;
#else
      printf("<%s,%s> --> %d (%d%%)\n", s1.c_str(), s2.c_str(),
	LevenshteinDistance (s1, s2),
	RatcliffCompare(s1,s2, 100));
#endif
    }
  clock_t end = clock();
  clock_t spent = end - start;
  printf("Timer Ticks = %ld for %d comps (%.1f comps/tick)\n", spent, terms, ((double)terms)/spent);
}
#endif
