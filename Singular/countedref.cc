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
    m_data(init(allocate(), data)) { }
  LeftvShallow(const self& rhs):
    m_data(init(allocate(), rhs.m_data)) { }

  ~LeftvShallow() {  
    kill();
    omFree(m_data);
  }
  self& operator=(leftv rhs) {
    kill();
    init(m_data, rhs);
    return *this;
  }

  self& operator=(const self& rhs) { return (*this) = rhs.m_data; }

  BOOLEAN get(leftv result) {
    leftv next = result->next;
    result->next = NULL;
    result->CleanUp();
    init(result, m_data);
    result->next = next;
    return FALSE;
  }

  /// Access to object
  leftv operator->() { return m_data;  }

protected:
  static leftv allocate() { return (leftv)omAlloc0(sizeof(sleftv)); }
  static leftv init(leftv result, leftv data) {
    memcpy(result, data, sizeof(sleftv));
    copy(result->e, data->e);
    result-> next = NULL;
    return result;
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
  LeftvDeep(leftv data): base(data) { }
  LeftvDeep(const self& rhs): base(rhs) { }

  ~LeftvDeep() { m_data->CleanUp(); }

  self& operator=(const self& rhs) { return operator=(rhs.m_data); }
  self& operator=(leftv rhs) {
    m_data->CleanUp();
    m_data->Copy(rhs);
    return *this;
  }
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
  /// Construct reference for Singular object
  explicit CountedRefData(leftv data, idhdl* ctx = &IDROOT):
    base(), m_data(data), m_context(ctx) { context(); }

  /// Construct deep copy
  CountedRefData(const self& rhs):
    base(), m_data(rhs.m_data), m_context(rhs.m_context) { }
  
  /// Destruct
  ~CountedRefData()  { }

  /// Replace data
  self& operator=(const self& rhs) {
    m_data = rhs.m_data;
    m_context = rhs.m_context;
    return *this;
  }
 
  /// Replace with other Singular data
  void set(leftv rhs, idhdl* ctx = &IDROOT) {
    m_data = rhs;
    m_context = ctx;
    context();
  }

  /// Write (shallow) copy to given handle
  BOOLEAN get(leftv res) {
    reclaim();
    BOOLEAN b = broken() || m_data.get(res); 
    release();
    return b;
  }

  /// Extract (shallow) copy of stored data
  LeftvShallow operator*() { return (broken()? LeftvShallow(): m_data); }

private:
  /// Check whether identifier became invalid
  /// @note Sergio Leone memorial function
  BOOLEAN broken() {
    if( (m_context == getmyroot()) || (m_context == &currRing->idroot))
      return FALSE;                  // the good, 

    if (m_data->RingDependend())     // the bad,
      return complain("Referenced identifier not available in current ring");

    return (brokenid(m_context) &&   // and the ugly (case)
            ((m_context == &basePack->idroot) || brokenid())) &&
      complain("Referenced identifier not found in current context");
  }

  /// Determine corresponding context
  /// @note for ring-dependent object we always store @c currRing's root as marker 
  void context() { if (m_data->RingDependend()) m_context = &currRing->idroot; }

  /// 
  BOOLEAN complain(const char* text) {
    Werror(text);
    return TRUE;
  }
  BOOLEAN brokenid(idhdl* root = &basePack->idroot) {
    idhdl handle = (idhdl) m_data->data;
    for(idhdl current = *root; current != NULL; current = IDNEXT(current))
      if (current == handle) return FALSE;
    return TRUE;
  }


  /// Singular object
  LeftvDeep m_data;

  /// Store namespace for ring-dependent objects
  idhdl* m_context;
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
  static BOOLEAN is_ref(leftv arg) {
    int typ = arg->Typ();
    return ((typ > MAX_TOK) &&
           (getBlackboxStuff(typ)->blackbox_Init == countedref_Init));
  }

  /// Construct new reference from Singular data  
  CountedRef(leftv arg):  m_data(new data_type(arg)) { m_data->reclaim(); }

protected:
  /// Recover previously constructed reference
  CountedRef(data_type* arg):  m_data(arg) { assume(arg); m_data->reclaim(); }

public:
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
    m_data->set(rhs);
    return *this;
  }

  /// Extract (shallow) copy of stored data
  LeftvShallow operator*() { return m_data->operator*(); }

  /// Construct reference data object from
  BOOLEAN outcast(leftv result) {
    m_data->reclaim();
    if (result->rtyp == IDHDL)
      IDDATA((idhdl)result->data) = (char *)m_data;
    else
      result->data = (void *)m_data;
    return FALSE;
  }
  data_type* outcast() { 
    m_data->reclaim();
    return m_data;
  }
  /// Kills a link to the referenced object
  void destruct() { if(!m_data->release()) delete m_data; }

  /// Kills the link to the referenced object
  ~CountedRef() { destruct(); }

  BOOLEAN dereference(leftv arg) {
    assume(is_ref(arg));
    return m_data->get(arg) || ((arg->next != NULL) && resolve(arg->next));
  }


  /// Get the actual object
  /// @note It may change leftv. It is common practice, so we are fine with it.

  static self cast(void* data) {
    assume(data != NULL);
    return self(static_cast<data_type*>(data));
  }

  static self cast(leftv arg) {
    assume((arg != NULL) && is_ref(arg));
    return self::cast(arg->Data());
  }

  /// If necessary dereference.
  /// @note The may change leftv. It is common practice, so we are fine with it.
  static BOOLEAN resolve(leftv arg) {
    assume(arg != NULL);
    while (is_ref(arg)) { if(CountedRef::cast(arg).dereference(arg)) return TRUE; };
    return (arg->next != NULL) && resolve(arg->next);
  }

  //private:
protected:
  /// Store pointer to actual data
  data_type* m_data;
};

/// blackbox support - convert to string representation
void countedref_Print(blackbox *b, void* ptr)
{
  if (ptr == NULL)  return;
  (*CountedRef::cast(ptr))->Print();
}

/// blackbox support - convert to string representation
char* countedref_String(blackbox *b, void* ptr)
{
  if (ptr == NULL) return NULL;
  return (*CountedRef::cast(ptr))->String();
}

/// blackbox support - copy element
void* countedref_Copy(blackbox*b, void* ptr)
{ 
  if (ptr) return CountedRef::cast(ptr).outcast();
  return NULL;
}

/// blackbox support - assign element
BOOLEAN countedref_Assign(leftv result, leftv arg)
{
  // Case: replace assignment behind reference
  if (result->Data() != NULL) {
    return CountedRef::cast(result).dereference(result) ||
      CountedRef::resolve(arg) ||
      iiAssign(result, arg);
  }
  
  // Case: new reference
  if(arg->rtyp == IDHDL) 
    return (result->Typ() == arg->Typ()?
            CountedRef::cast(arg):
            CountedRef(arg)).outcast(result);

  Werror("Can only take reference from identifier");
  return FALSE;
}
                                                                     
/// blackbox support - unary operations
BOOLEAN countedref_Op1(int op, leftv res, leftv head)
{
  if(op == TYPEOF_CMD)
    return blackboxDefaultOp1(op, res, head);

  return CountedRef::cast(head).dereference(head) || 
    iiExprArith1(res, head, (op == DEF_CMD? head->Typ(): op));
}

/// blackbox support - binary operations
BOOLEAN countedref_Op2(int op, leftv res, leftv head, leftv arg)
{

  return CountedRef::cast(head).dereference(head) || CountedRef::resolve(arg) ||
    iiExprArith2(res, head, op, arg);
}

/// blackbox support - ternary operations
BOOLEAN countedref_Op3(int op, leftv res, leftv head, leftv arg1, leftv arg2)
{
  return  CountedRef::cast(head).dereference(head) || 
    CountedRef::resolve(arg1) || CountedRef::resolve(arg2) ||
    iiExprArith3(res, op, head, arg1, arg2);
}


/// blackbox support - n-ary operations
BOOLEAN countedref_OpM(int op, leftv res, leftv args)
{
  return CountedRef::cast(args).dereference(args) || iiExprArithM(res, args, op);
}

/// blackbox support - destruction
void countedref_destroy(blackbox *b, void* ptr)
{
  if (ptr) CountedRef::cast(ptr).destruct();
}


class CountedRefShared:
  public CountedRef {
  typedef CountedRefShared self;
  typedef CountedRef base;
public:
  /// Construct new reference from Singular data  
  CountedRefShared(leftv arg):  base(new data_type(wrap(arg), getmyroot())) { }

private:
  /// Recover previously constructed shared data
  CountedRefShared(data_type* arg):  base(arg) { }
  CountedRefShared(const base& rhs):  base(rhs) { }
public:
  /// Construct copy
  CountedRefShared(const self& rhs): base(rhs) { }

  ~CountedRefShared() {  kill(); }

  self& operator=(const self& rhs) {
    kill();
    base::operator=(rhs);
    return *this;
  }

  /// Replace data that reference is pointing to
  self& operator=(leftv rhs) {
    m_data->set(wrap(rhs), getmyroot());
    return *this;
  }
  void destruct() {
    kill();
    base::destruct();
  }

  static self cast(leftv arg) { return base::cast(arg); }
  static self cast(void* arg) { return base::cast(arg); }
private:

  static leftv wrap(leftv arg) {
      char* name = (char*)omAlloc0(512);
      static unsigned int counter = 0;
      idhdl* myroot=getmyroot();
      sprintf(name, " :%u:%p:_shared_: ", ++counter, arg->Data());
      assume((*myroot)->get(name, 0) == NULL); 
      idhdl handle = (*myroot)->set(name, 0, arg->Typ(), FALSE);
      ++(*myroot)->ref;

      IDDATA(handle) = (char*) arg->CopyD();
      arg->CleanUp();
      arg->data = handle;
      arg->rtyp = IDHDL;
      arg->name = name;

    return arg;
  }

 void kill() {
   if (m_data->count() > 1) return;

   LeftvShallow data = base::operator*();
   idhdl* myroot = getmyroot();
   killhdl2((idhdl)(data->data), myroot, currRing);
   data->data = NULL;
   data->rtyp = NONE;
   
   if(--((*myroot)->ref)) {
     killhdl2(*myroot, &IDROOT, currRing);
     (*myroot) = NULL;
   }
 }
};


/// blackbox support - assign element
BOOLEAN countedref_AssignShared(leftv result, leftv arg)
{
  /// Case: replace assignment behind reference
  if ((result->Data()) != NULL) {
    if (CountedRefShared::resolve(arg)) return TRUE;
    CountedRefShared::cast(result) = arg;
    return FALSE;
  }
  
  /// Case: new shared data
  if (result->Typ() == arg->Typ()) 
    return CountedRefShared::cast(arg).outcast(result);

  return CountedRefShared(arg).outcast(result);
}

/// blackbox support - destruction
void countedref_destroyShared(blackbox *b, void* ptr)
{
  if (ptr) CountedRefShared::cast(ptr).destruct();
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

