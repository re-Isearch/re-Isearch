//
// Object.h
//
#ifndef	_Object_h_
#define	_Object_h_

class Object
{
public:
  // Constructor/Destructor
  Object();
  virtual ~Object();

  // To ensure a consistent comparison interface and to allow comparison
  // of all kinds of different objects, we will define a comparison functions.
  virtual int		Compare(const Object& ) const;

  // To allow a deep copy of data structures we will define a standard interface...
  // This member will return a copy of itself, freshly allocated and deep copied.
  virtual Object	*Clone() const;

protected:
};


#endif
