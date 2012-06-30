// -*- c++ -*-
//*****************************************************************************
/** @file pyobject.cc
 *
 * @author Alexander Dreyer
 * @date 2010-12-15
 *
 * This file defines the @c blackbox operations for the countedref type.
 *
 * @par Copyright:
 *   (c) 2010 by The Singular Team, see LICENSE file
**/
//*****************************************************************************





#include <Singular/mod2.h>

#include <Singular/ipid.h>
#include <Singular/blackbox.h>
#include <Singular/newstruct.h>

#include <omalloc/omalloc.h>
#include <kernel/febase.h>
#include <kernel/longrat.h>
#include <Singular/subexpr.h>
#include <Singular/ipshell.h>


/** @class CountedRefData
 * This class stores a reference counter as well as a Singular interpreter object.
 *
 * It also stores the Singular token number, once per type.
 **/
class CountedRefData {
public:
  typedef int id_type;
  typedef unsigned long count_type;

  /// Construct reference for Singular object
  CountedRefData(leftv data): m_count(0) {
    memset(&m_data, 0, sizeof(sleftv));
    m_data.Copy(data);
  }

  /// Deep Copy
  CountedRefData(const CountedRefData& to_copy): m_count(0) {
    memset(&m_data, 0, sizeof(sleftv));
    m_data.Copy(const_cast<leftv>(&to_copy.m_data));
  }

  ~CountedRefData()  { }

  const leftv operator*() {
    return &m_data;
  }
  operator char*() {
    sleftv result;
    memset(&result, 0, sizeof(sleftv));
    result.Copy(&m_data);
    return omStrDup(result.String());
  }

  /// Set Singular type identitfier 
  static void set_id(id_type new_id) {  access_id() = new_id;  }

  /// Get Singular type identitfier 
  static id_type id() { return access_id(); }

  /// @name Reference counter management
  //@{
  count_type operator++() { return ++m_count; }
  count_type operator--() { return --m_count; }
  count_type count() const { return m_count; }
  //@}

private:
  static id_type& access_id() {
    static id_type g_id = 0;
    return g_id;
  }

  count_type m_count;
  sleftv m_data;
};

/// blackbox support - initialization
void* countedref_Init(blackbox*)
{

    return NULL;
   // CountedRefData* result = new CountedRefData(NULL);
  //  result->operator++();
  //  return result;
}

/// blackbox support - convert to string representation
char* countedref_String(blackbox *b, void* ptr)
{
  CountedRefData tmp(*static_cast<CountedRefData*>(ptr));
  return (*tmp)->String();

  Print("string called");

 // if (ptr == NULL)  return omStrDup("oo");

//    sleftv result;
//    memset(&result, 0, sizeof(sleftv));
//    result.Copy((leftv)ptr);
    return omStrDup("undef");//result.String());
}

/// blackbox support - copy element
void* countedref_Copy(blackbox*b, void* ptr)
{ 
  static_cast<CountedRefData*>(ptr)->operator++();
  return ptr;
}

/// blackbox support - assign element
BOOLEAN countedref_Assign(leftv l, leftv r)
{
  CountedRefData* result = (r->Typ() == CountedRefData::id()? 
                            (CountedRefData*)r->Data(): new CountedRefData(r));

  if(result)
    result->operator++();

  if (l->rtyp == IDHDL)
    IDDATA((idhdl)l->data) = (char *)result;
  else
    l->data = (void *)result;
  

  //  Print( "ass %i, %i, %p  ,%i \n", l->Typ(), r->Typ(), result->operator*(), result->operator*()->Typ() );
  return !result;
}
                                                                     

/// blackbox support - unary operations
BOOLEAN countedref_Op1(int op, leftv res, leftv head)
{
  Print("op type %i %i\n", op, head->Typ());
  switch(op)
  {
  case TYPEOF_CMD:
    res->data = (void*) omStrDup("reference");
    res->rtyp = STRING_CMD;  
    return FALSE;
  }

  CountedRefData tmp(*static_cast<CountedRefData*>(head->Data()));
  return iiExprArith1(res, *tmp, op);
}


/// blackbox support - binary operations
BOOLEAN countedref_Op2(int op, leftv res, leftv arg1, leftv arg2)
{
  CountedRefData copy1(*static_cast<CountedRefData*>(arg1->Data()));

  if (arg2->Typ() != CountedRefData::id())  
     return iiExprArith2(res, *copy1, op, arg2);

  CountedRefData copy2(*static_cast<CountedRefData*>(arg2->Data()));
  return iiExprArith2(res, *copy1, op, *copy2, op);
}

/// blackbox support - ternary operations
BOOLEAN countedref_Op3(int op, leftv res, leftv arg1, leftv arg2, leftv arg3)
{
  CountedRefData copy1(*static_cast<CountedRefData*>(arg1->Data()));
  if (arg2->Typ() == CountedRefData::id()) {
    CountedRefData copy2(*static_cast<CountedRefData*>(arg2->Data()));
    if (arg3->Typ() == CountedRefData::id()) {
       CountedRefData copy3(*static_cast<CountedRefData*>(arg3->Data()));
       return iiExprArith3(res, op, *copy1, *copy2, *copy3);
    }
    return iiExprArith3(res, op, *copy1, *copy2, arg3);
  }
  if (arg3->Typ() == CountedRefData::id()) {
     CountedRefData copy3(*static_cast<CountedRefData*>(arg3->Data()));
     return iiExprArith3(res, op, *copy1, arg2, *copy3);
  }
  return iiExprArith3(res, op, *copy1, arg2, arg3);
}


/// blackbox support - n-ary operations
BOOLEAN countedref_OpM(int op, leftv res, leftv args)
{
  CountedRefData copy(*static_cast<CountedRefData*>(args->Data()));
  return iiExprArithM(res, *copy, op);
}

/// blackbox support - destruction
void countedref_destroy(blackbox *b, void* ptr)
{
  CountedRefData* pRef = static_cast<CountedRefData*>(ptr);

  Print("kill %i\n", pRef->count());
  if(!pRef->operator--())
    delete pRef;
}


// forward declaration
int iiAddCproc(const char *libname, const char *procname, BOOLEAN pstatic,
               BOOLEAN(*func)(leftv res, leftv v));

#define ADD_C_PROC(name) iiAddCproc("", #name, FALSE, name);


void countedref_init() 
{
  blackbox *bbx = (blackbox*)omAlloc0(sizeof(blackbox));
  bbx->blackbox_destroy = countedref_destroy;
  bbx->blackbox_String  = countedref_String;
  bbx->blackbox_Init    = countedref_Init;
  bbx->blackbox_Copy    = countedref_Copy;
  bbx->blackbox_Assign  = countedref_Assign;
  bbx->blackbox_Op1     = countedref_Op1;
  bbx->blackbox_Op2     = countedref_Op2;
  bbx->blackbox_Op3     = countedref_Op3;
  bbx->blackbox_OpM     = countedref_OpM;
  bbx->data             = omAlloc0(newstruct_desc_size());

  CountedRefData::set_id(setBlackboxStuff(bbx,"reference"));

}

extern "C" { void mod_init() { countedref_init(); } }

