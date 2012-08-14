// -*- c++ -*-
//*****************************************************************************
/** @file countedref.cc
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

#include "lists.h"
#include "attrib.h"


class CountedRef {
  typedef CountedRef self;

public:
  typedef int id_type;

  /// Set Singular type identitfier
  CountedRef(blackbox& bbx) {  
    CountedRefAccess_id() = setBlackboxStuff(&bbx, "reference");
  }
  CountedRef() { }

  /// Get Singular type identitfier
  static id_type id() { return CountedRefAccess_id(); }

  /// Check for being reference in Singular interpreter object
  static BOOLEAN is_ref(leftv arg) { return (arg->Typ() == id()); }
private:
  /// Access identifier (one per class)
  static id_type& CountedRefAccess_id() {
    static id_type g_id = 0;
    return g_id;
  }

};
/** @class CountedRefData
 * This class stores a reference counter as well as a Singular interpreter object.
 *
 * It also stores the Singular token number, once per type.
 **/
class CountedRefData: public CountedRef {
  typedef CountedRefData self;

  /// Forbit copy construction and normal assignment
  CountedRefData(const self&);
  self& operator=(const self&);

public:
  typedef unsigned long count_type;

  /// Construct reference for Singular object
  CountedRefData(leftv data): m_data(*data), m_count(0), m_ring(NULL) {

    if (RingDependend(data->Typ())  && (currRing != NULL) ) {
      m_ring = currRing;
      ++m_ring->ref;
    }
    m_data.e = copyall(data->e);
  }

  /// Destructor
  ~CountedRefData()  {
    assume(m_count == 0);
    killall(m_data.e);
    if (m_ring) --m_ring->ref;
  }


  static BOOLEAN set_to(leftv res, void* data, int op)
  {
    if (res->rtyp == IDHDL) {
      IDDATA((idhdl)res->data) = (char *)data;
      IDTYP((idhdl)res->data)  = op;
    }
    else {
      res->data = (void *)data;
      res->rtyp = op;
    }
    return (op == NONE? TRUE: FALSE);
  }

  static BOOLEAN construct(leftv result, leftv arg) {
    self* pRef(is_ref(arg)? static_cast<self*>(arg->Data()): new self(arg));
    pRef->reclaim();

    return set_to(result, pRef, id());
  }

  /// @name Reference counter management
  //@{
  count_type reclaim() { return ++m_count; }
  count_type release() { return --m_count; }
  count_type count() const { return m_count; }
  //@}

  leftv get () { 
    leftv result = (leftv)omAlloc0(sizeof(sleftv));
    get(*result);
    return result;
  }

  void get(sleftv& result) {
    if (m_ring && (m_ring != currRing)) {
      Werror("Can only use references from current ring.");
      return;
    }
    leftv next = result.next;
    memcpy(&result, &m_data, sizeof(sleftv));
    result.e = copyall(m_data.e);
    result.next = next;
  }
  BOOLEAN get(leftv result) {
    if (m_ring && (m_ring != currRing)) {
      Werror("Can only use references from current ring.");
      return TRUE;
    }
    leftv next = result->next;
    memcpy(result, &m_data, sizeof(sleftv));
    result->e = copyall(m_data.e);
    result->next = next;
    
    return FALSE;
  }

  static BOOLEAN dereference(leftv arg) {
    assume((arg != NULL) && is_ref(arg));
    self* pRef = static_cast<self*>(arg->Data());
    assume(pRef != NULL);
    return pRef->get(arg);
  }

  static BOOLEAN resolve(leftv arg) {
    assume(arg != NULL);
    if (is_ref(arg)) return dereference(arg);
    return FALSE;
  }

private:

  static Subexpr copyall(Subexpr rhs) {
    if (rhs == NULL)
      return NULL;

    Subexpr result = (Subexpr)omAlloc0Bin(sSubexpr_bin);
    Subexpr current = result;
    memcpy(current, rhs, sizeof(*rhs));

    while(rhs->next) {
      current->next = (Subexpr)omAlloc0Bin(sSubexpr_bin);
      memcpy(current->next, rhs->next, sizeof(*rhs));
      current = current->next;
      rhs = rhs->next;
    } 
    return result;
  }
  static void killall(Subexpr rhs) {
    while(rhs) {
      Subexpr next = rhs->next;
      omFree(rhs);
      rhs = next;
    }
  }
  /// Reference counter
  count_type m_count;

  /// Singular object
  sleftv m_data;

  /// Store ring for ring-dependent objects
  ring m_ring;
};

class CountedRefAccessBase {
  typedef CountedRefAccessBase self;

 public:
  CountedRefAccessBase(leftv data):
    m_data(data) {}

  CountedRefAccessBase(const self& rhs):
    m_data(rhs.m_data) {}

  ~CountedRefAccessBase() {}

  leftv operator->() { return *this; }
  operator leftv() {  return m_data;  }

 protected:
   leftv m_data;
 };


 class CountedRefAccess:
   public CountedRefAccessBase {
    typedef CountedRefAccess self;
    typedef CountedRefAccessBase base;

  public:
    CountedRefAccess(CountedRefData* data):
      m_owns(true), base(data->get())  { }


    CountedRefAccess(leftv data):
      m_owns(CountedRef::is_ref(data)), base(data) {
      if (m_owns) m_data = static_cast<CountedRefData*>(data->Data())->get();
    }

    CountedRefAccess(const self& rhs):
      m_owns(rhs.m_owns), base(rhs) {

      if (m_owns){
        m_data = (leftv)omAlloc0(sizeof(sleftv));
        if(rhs.m_data != NULL) memcpy(m_data, rhs.m_data, sizeof(sleftv));
      }
    }

    ~CountedRefAccess() {  if (m_owns) omFree(m_data);  }

  private:
    bool m_owns;
};

class CountedRefCast:
  public CountedRefAccessBase {
    typedef CountedRefCast self;
    typedef CountedRefAccessBase base;
  public:
    CountedRefCast(void* data):
      base(static_cast<CountedRefData*>(data)->get()) { }

    CountedRefCast(leftv data):
      base(static_cast<CountedRefData*>(data->Data())->get()) { }

    CountedRefCast(const self& rhs):
      base((leftv)omAlloc0(sizeof(sleftv))) {
      memcpy(m_data, rhs.m_data, sizeof(sleftv));
    }

    ~CountedRefCast() {  omFree(m_data);  }
};

/// blackbox support - initialization
void* countedref_Init(blackbox*)
{
  return NULL;
}

/// blackbox support - convert to string representation
void countedref_Print(blackbox *b, void* ptr)
{
  if (ptr != NULL) CountedRefCast(ptr)->Print();
}

/// blackbox support - convert to string representation
char* countedref_String(blackbox *b, void* ptr)
{
  if (ptr != NULL) return CountedRefCast(ptr)->String();
}

/// blackbox support - copy element
void* countedref_Copy(blackbox*b, void* ptr)
{ 
  if (ptr) static_cast<CountedRefData*>(ptr)->reclaim();
  return ptr;
}

/// blackbox support - assign element
BOOLEAN countedref_Assign(leftv result, leftv arg)
{
  // Case: replace assignment behind reference
  if (result->Data() != NULL) 
    return CountedRefData::dereference(result) || CountedRefData::resolve(arg) ||
      iiAssign(result, arg);
  
  // Case: new reference
  if(arg->rtyp == IDHDL) 
    return CountedRefData::construct(result, arg);
  
  Werror("Can only take reference from identifier");
  return CountedRefData::set_to(result, NULL, NONE);
}
                                                                     

/// blackbox support - unary operations
BOOLEAN countedref_Op1(int op, leftv res, leftv head)
{
  if(op == TYPEOF_CMD)
    return CountedRefData::set_to(res, omStrDup("reference"), STRING_CMD);

  if (CountedRefData::dereference(head))
    return TRUE;

  if (op == DEF_CMD){
    res->rtyp = head->Typ();
    return iiAssign(res, head);
  }
  return iiExprArith1(res, head, op);
}

/// blackbox support - binary operations
BOOLEAN countedref_Op2(int op, leftv res, leftv head, leftv arg)
{
  return CountedRefData::dereference(head) || CountedRefData::resolve(arg) ||
    iiExprArith2(res, head, op, arg);
}

/// blackbox support - ternary operations
BOOLEAN countedref_Op3(int op, leftv res, leftv head, leftv arg1, leftv arg2)
{
  return CountedRefData::dereference(head) || 
    CountedRefData::resolve(arg1) || CountedRefData::resolve(arg2) ||
    iiExprArith3(res, op, head, arg1, arg2);
}


/// blackbox support - n-ary operations
BOOLEAN countedref_OpM(int op, leftv res, leftv args)
{
  if (CountedRefData::dereference(args))
    return TRUE;

  for(leftv current = args->next; current != NULL; current = current->next) {
    if(CountedRef::is_ref(current)) {
      CountedRefData* pRef = static_cast<CountedRefData*>(current->Data());
      pRef->get(*current);
      pRef->release();
      assume(pRef->count() > 0);
    }  
  }

  return iiExprArithM(res, args, op);
}

/// blackbox support - destruction
void countedref_destroy(blackbox *b, void* ptr)
{
  CountedRefData* pRef = static_cast<CountedRefData*>(ptr);
  if(ptr && !pRef->release())
    delete pRef;
}

void countedref_init() 
{
  blackbox *bbx = (blackbox*)omAlloc0(sizeof(blackbox));
  bbx->blackbox_destroy = countedref_destroy;
  bbx->blackbox_String  = countedref_String;
  bbx->blackbox_Print  = countedref_Print;
  bbx->blackbox_Init    = countedref_Init;
  bbx->blackbox_Copy    = countedref_Copy;
  bbx->blackbox_Assign  = countedref_Assign;
  bbx->blackbox_Op1     = countedref_Op1;
  bbx->blackbox_Op2     = countedref_Op2;
  bbx->blackbox_Op3     = countedref_Op3;
  bbx->blackbox_OpM     = countedref_OpM;
  bbx->data             = omAlloc0(newstruct_desc_size());
  CountedRef init(*bbx);
}

extern "C" { void mod_init() { countedref_init(); } }

