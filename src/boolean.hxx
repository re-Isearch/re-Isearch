#ifndef _BOOLEAN_HXX
#define _BOOLEAN_HXX

class bool {
 protected:
  char data;
   
 public:
  bool():data(0)                  {}
  bool(int i):data(i!=0)          {}
  bool(const bool& b):data(b.data){}
  operator int() const                      {return (int)data!=0;}
  bool&    operator= (const bool& b)        {data=b.data; return *this;}
  bool&    operator! () const               {return data==0;}
  bool&    operator== (const bool& b) const {return data==b.data;}
};           

extern const bool true;
extern const bool false;


