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
idhdl* getmyroot() {
  static idhdl myroot = NULL;
  if (myroot == NULL) {
    myroot = enterid(" _shared_data_ ", 0, PACKAGE_CMD, &IDROOT, TRUE);
  }
  return &myroot;
}

class RefCounter {
  typedef RefCounter self;

  /// Name numerical type for enumbering
  typedef unsigned long count_type;

public:
  /// Default Constructor
  RefCounter(): m_count(0) {}

  /// Copying resets the counter
  RefCounter(const self&): m_count(0) {}

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
  LeftvShallow(): m_data(allocate()) { }
  LeftvShallow(leftv data): 
    m_data(allocate()) { init(m_data, data); }
  LeftvShallow(const self& rhs):
    m_data(allocate()) { init(m_data, rhs.m_data); }

  ~LeftvShallow() {  
    kill();
    omFree(m_data);
  }
  self& operator=(leftv rhs) {
    kill();
    init(m_data, rhs);
    return *this;
  }

  self& operator=(const self& rhs) {
    return (*this) = rhs.m_data;
  }

  BOOLEAN get(leftv result) {
    if (broken())
      return TRUE;      

    leftv next = result->next;
    result->next = NULL;
    result->CleanUp();
    init(result, m_data);
    result->next = next;
    return FALSE;
  }

  /// Access to object
  leftv operator->() { return *this; }
  operator leftv() { 
    broken();
    return m_data;
  }
private:
  /// In case of identifier, ensure that the handle was not killed yet
  /// @note: This may fail, if m_data.data were completely deallocated.
  /// But this should not occur
  BOOLEAN broken() const {
    if((m_data->rtyp ==IDHDL) && brokenid(getmyroot()) && brokenid(&IDROOT) && 
       brokenid(&currRing->idroot) ) {
      Werror("Referenced identifier not available in current context");
      return TRUE;
    }
    return FALSE;
  }

  BOOLEAN brokenid(idhdl* root) const {
    idhdl handle = (idhdl) m_data->data;
    for(idhdl current = *root; current != NULL; current = IDNEXT(current))
      if (current == handle) return FALSE;
    return TRUE;
  }
protected:
  static leftv allocate() { return (leftv)omAlloc0(sizeof(sleftv)); }
  static void init(leftv result, leftv data) {
    memcpy(result, data, sizeof(sleftv));
    copy(result->e, data->e);
  }
  static void copy(Subexpr& current, Subexpr rhs)  {
    if (rhs == NULL) return;
    current = (Subexpr)memcpy(omAlloc0Bin(sSubexpr_bin), rhs, sizeof(*rhs));
    copy(current->next, rhs->next);
  }
  void kill() { kill(m_data->e); }
  static void kill(Subexpr current) {
    if(current == NULL) return;
    kill(current->next);
    omFree(current);
  }
protected:
  leftv m_data;
};


class LeftvDeep:
  public LeftvShallow {
  typedef LeftvDeep self;
  typedef LeftvShallow base;

public:
  LeftvDeep(): base() {}
  LeftvDeep(leftv data): base() {  init(data); }
  LeftvDeep(const self& rhs): base() { init(rhs.m_data); }

  ~LeftvDeep() { m_data->CleanUp(); }

  self& operator=(const self& rhs) {
    return operator=(rhs.m_data);
  }
  self& operator=(leftv rhs) {
    m_data->CleanUp();
    m_data->Copy(rhs);
    return *this;
  }
private:  
  void init(leftv rhs) {
    if(rhs->rtyp == IDHDL)
      base::init(m_data, rhs);
    else
      m_data->Copy(rhs);
  }
};


class CountedRefRing {
  typedef CountedRefRing self;
public:
  /// Default constructor
  CountedRefRing(): m_ring(NULL) {}

  /// Construct from given ring
  template <class Type>
  CountedRefRing(const Type& rhs): m_ring(rhs) { reclaim(); }

  /// Destructor
  ~CountedRefRing() { release(); }

  /// Automatically converting to @c ring
  operator ring() { return m_ring; }

  /// Pointer-stylishly cccessing @c ring
  ring operator->() { return *this; }

  /// Assign other ring
  template <class Type>
  self& operator=(const Type& rhs) {
    release();
    m_ring = rhs;
    reclaim();
    return *this;
  }

  /// Determines this is not the current ring
  BOOLEAN deactivated() const {
    return (m_ring != NULL) && (m_ring != currRing) && complain();
  }

private:
  /// Clip taken ring
  void reclaim() { if (m_ring) ++m_ring->ref; }
  /// Relieve taken ring
  void release() { if(m_ring && --m_ring->ref) rKill(m_ring); }
  /// Raise error if this is not the current ring
  static BOOLEAN complain() {
     Werror("Can only use references from current ring.");
     return TRUE;
  }

  /// Store pointer of taken ring
  ring m_ring;
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

public:
  /// Default constructor
  CountedRefData(): base(), m_data(), m_ring() {}

  /// Construct reference for Singular object
  CountedRefData(leftv data): 
    base(), m_data(data), m_ring(parent(data)) { }

  /// Construct deep copy
  CountedRefData(const self& rhs):
    base(), m_data(rhs.m_data), m_ring(rhs.m_ring) { }
  
  /// Destruct
  ~CountedRefData()  { }

  /// Replace data
  self& operator=(const self& rhs) {
    m_data = rhs.m_data;
    m_ring = rhs.m_ring;
    return *this;
  }
 
  /// Replace with other Singular data
  self& operator=(leftv rhs) {
    m_data = rhs;
    m_ring = parent(rhs);
    return *this;
  }

  /// Write (shallow) copy to given handle
  BOOLEAN get(leftv res) { return m_ring.deactivated() || m_data.get(res); }

  /// Extract (shallow) copy of stored data
  LeftvShallow operator*() { return (m_ring.deactivated()? LeftvShallow(): m_data); }

  /// Dangerours!
  leftv access() { return m_data.operator->(); }

private:
  /// Detect ring for current object
  static ring parent(leftv data) {
    return ((data->rtyp != IDHDL) && data->RingDependend()? currRing: NULL);
  }

  /// Singular object
  LeftvDeep m_data;

  /// Store ring for ring-dependent objects
  CountedRefRing m_ring;
};

/// blackbox support - initialization
/// @note deals as marker for compatible references, too.
void* countedref_Init(blackbox*)
{
  return NULL;
}

class CountedRef {
  typedef CountedRef self;

public:
  /// name type for identifiers
  typedef int id_type;

  /// Name type for handling reference data
  typedef CountedRefData data_type;

  /// Check whether argument is already a reference type
  static BOOLEAN is_ref(int typ) {
    return ((typ > MAX_TOK) &&
           (getBlackboxStuff(typ)->blackbox_Init == countedref_Init));
  }

  /// Construct new reference from Singular data  
  CountedRef(leftv arg):  m_data(new data_type(arg)) { m_data->reclaim(); }

  /// Recover previously constructed reference
  CountedRef(data_type* arg):  m_data(arg) { assume(arg); m_data->reclaim(); }

  /// Construct copy
  CountedRef(const self& rhs): m_data(rhs.m_data) { m_data->reclaim(); }

  /// Replace reference
  self& operator=(const self& rhs) {
    destruct();
    m_data = rhs.m_data;
    m_data->reclaim();
    return *this;
  }

  /// Replace data that reference is pointing to
  self& operator=(leftv rhs) {
    (*m_data) = rhs;
    return *this;
  }

  /// Extract (shallow) copy of stored data
  LeftvShallow operator*() { return m_data->operator*(); }

  /// Construct reference data object from
  BOOLEAN checkout(leftv result) {
    m_data->reclaim();
    if (result->rtyp == IDHDL)
      IDDATA((idhdl)result->data) = (char *)m_data;
    else
      result->data = (void *)m_data;
    return FALSE;
  }

  /// Kills a link to the referenced object
  void destruct() { if(!m_data->release()) delete m_data; }

  /// Kills the link to the referenced object
  ~CountedRef() { destruct(); }

  /// Get the actual object
  /// @note It may change leftv. It is common practice, so we are fine with it.
  static BOOLEAN dereference(leftv arg) {
    assume((arg != NULL) && is_ref(arg->Typ()));
    do {
      assume(arg->Data() != NULL);
      if(static_cast<data_type*>(arg->Data())->get(arg)){ return TRUE; }
    } while (is_ref(arg->Typ()));
    return (arg->next != NULL) && resolve(arg->next);
  }

  /// If necessary dereference.
  /// @note The may change leftv. It is common practice, so we are fine with it.
  static BOOLEAN resolve(leftv arg) {
    assume(arg != NULL);
    while (is_ref(arg->Typ())) { if(dereference(arg)) return TRUE; };
    return (arg->next != NULL) && resolve(arg->next);
  }

private:
  /// Store pointer to actual data
  data_type* m_data;
};

/// blackbox support - convert to string representation
void countedref_Print(blackbox *b, void* ptr)
{
  if (ptr != NULL) (*CountedRef(static_cast<CountedRefData*>(ptr)))->Print();
}

/// blackbox support - convert to string representation
char* countedref_String(blackbox *b, void* ptr)
{
  if (ptr != NULL) return (*CountedRef(static_cast<CountedRefData*>(ptr)))->String();
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
    return (result->Typ() == arg->Typ()?
            CountedRef(static_cast<CountedRefData*>(arg->Data())):
            CountedRef(arg)).checkout(result);

  Werror("Can only take reference from identifier");
  return FALSE;
}
                                                                     
/// blackbox support - unary operations
BOOLEAN countedref_Op1(int op, leftv res, leftv head)
{
  if(op == TYPEOF_CMD)
    return blackboxDefaultOp1(op, res, head);
  return  CountedRef::dereference(head) || 
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
  if (ptr) CountedRef(static_cast<CountedRefData*>(ptr)).destruct();
}


/// blackbox support - assign element
BOOLEAN countedref_AssignShared(leftv result, leftv arg)
{
  /// Case: replace assignment behind reference
  if ((result->Data()) != NULL) {
    if (CountedRef::resolve(arg)) return TRUE;
    CountedRef(static_cast<CountedRefData*>(result->Data())) = arg;
    return FALSE;
  }

  
  /// Case: new
  int rt = arg->Typ();
  if (result->Typ() == rt) 
    return CountedRef(static_cast<CountedRefData*>(arg->Data())).checkout(result);

  blackbox *bbx = (rt > MAX_TOK? getBlackboxStuff(rt): NULL);

  if( (rt == LIST_CMD) || (rt==MATRIX_CMD) || (rt==INTMAT_CMD)
      || (rt==BIGINTMAT_CMD) || (rt==INTVEC_CMD) ||(rt==MODUL_CMD)
      || (rt==RESOLUTION_CMD) || ((bbx != NULL) && BB_LIKE_LIST(bbx))  ) {

    char* name = (char*)omAlloc0(512);

    unsigned int counter = 0;
    idhdl* myroot=getmyroot();
    char* data = (char*)arg->CopyD();
    do {
      sprintf(name, ":%s:%s:%p:%u:\0", result->Name(), arg->Name(),
              data, ++counter);
    } while(((*myroot)->get(name, myynest) !=NULL) && (counter < 100) );

    if (counter >= 100) {
      Werror("No temporary identifier for shared data found.");
      omFree(name);
      return TRUE;
    }

    idhdl handle = enterid(name, 0, rt, myroot, FALSE);
    ++(*myroot)->ref;

    IDDATA(handle) = data;
    
    arg->CleanUp();
    arg->data = handle;
    arg->rtyp = IDHDL;
    arg->name = name;
  }
  return CountedRef(arg).checkout(result);
}

/// blackbox support - destruction
void countedref_destroyShared(blackbox *b, void* ptr)
{
  CountedRefData* data = static_cast<CountedRefData*>(ptr);

  if(data && !data->release()) {
    if (data->access()->rtyp == IDHDL) // We made the identifier, so we clean up
    {
      idhdl* myroot = getmyroot();
      killhdl2((idhdl)(data->access()->data), myroot, currRing);
      assume(*myroot != NULL);
      if(--((*myroot)->ref)) {
          killhdl2(*myroot, &IDROOT, currRing);
          (*myroot) = NULL;
        }
      
    }
    delete data;
  }
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
  setBlackboxStuff(bbx, "reference");

  /// The @c shared type is "inherited" from @c reference.
  /// It just uses another constructor (to make its own copy of the).
  blackbox *bbxshared = 
    (blackbox*)memcpy(omAlloc(sizeof(blackbox)), bbx, sizeof(blackbox));
  bbxshared->blackbox_Assign  = countedref_AssignShared;
  bbxshared->blackbox_destroy  = countedref_destroyShared;

  setBlackboxStuff(bbxshared, "shared");
}

extern "C" { void mod_init() { countedref_init(); } }

