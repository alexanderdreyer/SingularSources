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
 typedef CountedRefData self;
public:
  typedef int id_type;
  typedef unsigned long count_type;

  /// Construct reference for Singular object
  CountedRefData(leftv data): m_count(0) { init(data); }

  /// Deep Copy
  CountedRefData(const CountedRefData& to_copy): m_count(0) {
    init(to_copy);
  }

  /// Deep copy of n-ary argument sequence
  CountedRefData(const CountedRefData& to_copy, leftv next): m_count(0) {
    init(to_copy);
    leftv tmp = &m_data;
    while (next) {
      (tmp->next)=(leftv)omAlloc0(sizeof(sleftv));
      (tmp->next)->Copy(is_ref(next)? self(next).get(): next);
      tmp = tmp->next;
      next = next->next;
    }
  }

  /// Destructor
  ~CountedRefData()  { m_data.CleanUp(); }

  /// Assignment
  self& operator=(const self& rhs) {
    m_data.CleanUp();
    init(rhs);
    return *this;
  }

  /// Access ti actual object
  leftv get() {
    return &m_data;
  }

  /// @name Forward operations
  //@{
  BOOLEAN operator()(int op, leftv result) {
    return iiExprArith1(result, get(), op);
  }
  BOOLEAN operator()(int op, leftv result, leftv arg) {
    return iiExprArith2(result, get(), op,
                       (is_ref(arg)? cast(arg).get(): arg));
  }
  BOOLEAN operator()(int op, leftv result, leftv arg1, leftv arg2) {
    return iiExprArith3(result, op, get(),
                        (is_ref(arg1)? cast(arg1).get(): arg1),
                        (is_ref(arg2)? cast(arg2).get(): arg2));
  }
  //@}

  /// Set Singular type identitfier 
  static void set_id(id_type new_id) {  access_id() = new_id;  }

  /// Get Singular type identitfier 
  static id_type id() { return access_id(); }

  static self cast(leftv value) {
    assume((value != NULL) && is_ref(value));
    return self(*static_cast<CountedRefData*>(value->Data()),value->next);
  }

  /// Extract and deeply copy reference from Singular interpreter object data
  static self cast(void* data) {
    assume(data != NULL);
    return self(*static_cast<CountedRefData*>(data));
  }

  /// Check for being reference in Singular interpreter object
  static BOOLEAN is_ref(leftv arg) { return (arg->Typ() == id()); }

  /// @name Reference counter management
  //@{
  count_type operator++() { return ++m_count; }
  count_type operator--() { return --m_count; }
  count_type count() const { return m_count; }
  //@}

private:
  /// Gain our own copy of argument
  void init(leftv arg) {
    memset(&m_data, 0, sizeof(sleftv));
    m_data.Copy(arg);
  }

  /// Gain deep copy
  void init(const self& rhs) {
    init(const_cast<leftv>(&rhs.m_data));
  }

  /// Access identifier (one per class)
  static id_type& access_id() {
    static id_type g_id = 0;
    return g_id;
  }
  /// Reference counter
  count_type m_count;

  /// Singular object
  sleftv m_data;
};

/// blackbox support - initialization
void* countedref_Init(blackbox*)
{
    return NULL;
}

/// blackbox support - convert to string representation
char* countedref_String(blackbox *b, void* ptr)
{
  CountedRefData tmp(*static_cast<CountedRefData*>(ptr));
  return (tmp.get())->String();
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

  return !result;
}
                                                                     

/// blackbox support - unary operations
BOOLEAN countedref_Op1(int op, leftv res, leftv head)
{
  switch(op)
  {
  case TYPEOF_CMD:
    res->data = (void*) omStrDup("reference");
    res->rtyp = STRING_CMD;  
    return FALSE;
  }

  return CountedRefData::cast(head)(op, res);
}


/// blackbox support - binary operations
BOOLEAN countedref_Op2(int op, leftv res, leftv head, leftv arg)
{
  return CountedRefData::cast(head)(op, res, arg);
}

/// blackbox support - ternary operations
BOOLEAN countedref_Op3(int op, leftv res, leftv head, leftv arg1, leftv arg2)
{
   return CountedRefData::cast(head)(op, res, arg1, arg2);
}


/// blackbox support - n-ary operations
BOOLEAN countedref_OpM(int op, leftv res, leftv args)
{
  return iiExprArithM(res, CountedRefData::cast(args).get(), op);
}

/// blackbox support - destruction
void countedref_destroy(blackbox *b, void* ptr)
{
  CountedRefData* pRef = static_cast<CountedRefData*>(ptr);
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

