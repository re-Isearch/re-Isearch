//
// HTMLEntities.h
//
//
#ifndef _HTMLEntities_h_
#define _HTMLEntities_h_

#include "dictionary.hxx"

class HTMLEntities : public Object
{
public:
  HTMLEntities();
  ~HTMLEntities();

  void           add(const char *entity, long value);
			
  STRING         entity(unsigned short Ch) const;

  int            translate(const char *) const;
  int            translate(size_t *pos, char **) const;
  void           normalize(char *buffer) const;
  void           normalize(char *buffer, size_t len) const;
  void           normalize2(char *buffer, size_t len) const;

private:
  void           init();
  Dictionary    *trans;
};

#endif


