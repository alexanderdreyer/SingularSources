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


class CountedRefEnv {
  typedef CountedRefEnv self;



};

class RefCounter {
  typedef RefCounter self;

  /// Name numerical type for enumbering
  typedef unsigned long count_type;

public:
  /// Default Constructor
  RefCounter(): m_count(0) {}

  /// Copying resets the counter
  RefCounter(const self& rhs): m_count(0) {}

  /// Destructor
  ~RefCounter() { assume(m_count == 0); }

  /// @name Reference counter management
  //@{
  count_type reclaim() { return ++m_count; }
  count_type release() { return --m_count; }
  count_type count() const { return m_count; }
  //@}

private:
  /// Number of references
  count_type m_count;
};

class LeftvShallow {
  typedef LeftvShallow self;

public:
  LeftvShallow(leftv data): m_data(*data) {  copy(&m_data.e, data->e); }
  LeftvShallow(const self& rhs): m_data(rhs.m_data) { copy(&m_data.e, rhs.m_data.e); }

  ~LeftvShallow() {  kill(m_data.e); }

  BOOLEAN get(leftv result) {
    leftv next = result->next;
    result->next = NULL;
    result->CleanUp();
    memcpy(result, &m_data, sizeof(m_data));
    copy(&result->e, m_data.e);
    result->next = next;
    return FALSE;
  }

  /// Read-only access to object
  const leftv operator->() { return &m_data; }

private:
  static void copy(Subexpr* result, Subexpr rhs) {
    for (Subexpr* current = result; rhs != NULL; 
         current = &(*current)->next, rhs = rhs->next)
      *current = (Subexpr)memcpy(omAlloc0Bin(sSubexpr_bin), rhs, sizeof(*rhs));
  }

  static void kill(Subexpr rhs) {
    for (Subexpr next; rhs!=NULL; rhs = next, next = rhs->next) {
      next = rhs->next;
      omFree(rhs);
    }
  }
  sleftv m_data;
};

/** @class CountedRefData
 * This class stores a reference counter as well as a Singular interpreter object.
 *
 * It also stores the Singular token number, once per type.
 **/
class CountedRefData:
  public RefCounter {
  typedef CountedRefData self;
  typedef RefCounter base;

  /// Forbit copy construction and normal assignment
  CountedRefData(const self&);
  self& operator=(const self&);

public:
  /// Construct reference for Singular object
  CountedRefData(leftv data): base(), m_data(data), m_ring(NULL) {
    if (RingDependend(data->Typ())  && (currRing != NULL) ) {
      m_ring = currRing;
      ++m_ring->ref;
    }

  }

  /// Destructor
  ~CountedRefData()  { }


  BOOLEAN get(leftv result) {
    if (m_ring && (m_ring != currRing)) {
      Werror("Can only use references from current ring.");
      return TRUE;
    }
    // dereferencing only makes sense, if something else points here, too.
    assume(count() > 1);
    return m_data.get(result);
  }

  LeftvShallow get() {
    if (m_ring && (m_ring != currRing))
      Werror("Can only use references from current ring.");
  
    return LeftvShallow(m_data);
  }

private:

  /// Singular object
  LeftvShallow m_data;

  /// Store ring for ring-dependent objects
  ring m_ring;
};


class CountedRef {
  typedef CountedRef self;

public:
  /// name type for identifiers
  typedef int id_type;

  /// Name type for handling reference data
  typedef CountedRefData data_type;

  /// Check for being reference in Singular interpreter object
  static BOOLEAN is_ref(leftv arg) { return (arg->Typ() == id()); }

  /// Construct reference data object from 
  static BOOLEAN construct(leftv result, leftv arg) {
    data_type* data = (is_ref(arg)? static_cast<data_type*>(arg->Data()):
                       new data_type(arg));
    data->reclaim();

    if (result->rtyp == IDHDL) {
      IDDATA((idhdl)result->data) = (char *)data;
      IDTYP((idhdl)result->data)  = id();
    }
    else {
      result->data = (void *)data;
      result->rtyp = id();
    }
    return (data == NULL? TRUE: FALSE);
  }

  /// Kills the link to the referenced object
  static void destruct(data_type* data) {
    if(data && !data->release())
      omFree(data);
  }

  /// Get the actual object
  /// @note The may change leftv. It is common practice, so we are fine with it.
  static BOOLEAN dereference(leftv arg) {
    assume((arg != NULL) && is_ref(arg));
    data_type* pRef = static_cast<data_type*>(arg->Data());
    assume(pRef != NULL);
    return pRef->get(arg) || resolve_tail(arg);
  }

  /// If necessary dereference.
  /// @note The may change leftv. It is common practice, so we are fine with it.
  static BOOLEAN resolve(leftv arg) {
    assume(arg != NULL);
    if (is_ref(arg)) return dereference(arg);
    return resolve_tail(arg);
  }


  /// Initialize blackbox type identifier
  static id_type init(blackbox& bbx) {  
    return identifier() = setBlackboxStuff(&bbx, "reference");
  }

  /// Get Singular type identitfier
  static id_type id() { return identifier(); }

private:
  /// Access identifier (one per class)
  static id_type& identifier() {
    static id_type g_id = 0;
    return g_id;
  }

  /// Dereference (is needed) subsequent objects of sequences
  static BOOLEAN resolve_tail(leftv arg) {
    for(leftv next = arg->next; next != NULL; next = next->next)
      if(resolve(next))
        return TRUE;
    return FALSE;
  }
};

/// blackbox support - initialization
void* countedref_Init(blackbox*)
{
  return NULL;
}

/// blackbox support - convert to string representation
void countedref_Print(blackbox *b, void* ptr)
{
  if (ptr != NULL) static_cast<CountedRefData*>(ptr)->get()->Print();
}

/// blackbox support - convert to string representation
char* countedref_String(blackbox *b, void* ptr)
{
  if (ptr != NULL) return static_cast<CountedRefData*>(ptr)->get()->String();
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
    return CountedRef::dereference(result) || CountedRef::resolve(arg) ||
      iiAssign(result, arg);
  
  // Case: new reference
  if(arg->rtyp == IDHDL) 
    return CountedRef::construct(result, arg);
  
  Werror("Can only take reference from identifier");
  return FALSE;
}
                                                                     

/// blackbox support - unary operations
BOOLEAN countedref_Op1(int op, leftv res, leftv head)
{
  if(op == TYPEOF_CMD)
    return blackboxDefaultOp1(op, res, head);

  return CountedRef::dereference(head) || 
    iiExprArith1(res, head, (op == DEF_CMD? head->Typ(): op));
}

/// blackbox support - binary operations
BOOLEAN countedref_Op2(int op, leftv res, leftv head, leftv arg)
{
  return CountedRef::dereference(head) || CountedRef::resolve(arg) ||
    iiExprArith2(res, head, op, arg);
}

/// blackbox support - ternary operations
BOOLEAN countedref_Op3(int op, leftv res, leftv head, leftv arg1, leftv arg2)
{
  return CountedRef::dereference(head) || 
    CountedRef::resolve(arg1) || CountedRef::resolve(arg2) ||
    iiExprArith3(res, op, head, arg1, arg2);
}


/// blackbox support - n-ary operations
BOOLEAN countedref_OpM(int op, leftv res, leftv args)
{
  return CountedRef::dereference(args) || iiExprArithM(res, args, op);
}

/// blackbox support - destruction
void countedref_destroy(blackbox *b, void* ptr)
{
  CountedRef::destruct(static_cast<CountedRefData*>(ptr));
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
  CountedRef::init(*bbx);
}

extern "C" { void mod_init() { countedref_init(); } }

