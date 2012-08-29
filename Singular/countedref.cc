/// -*- c++ -*-
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

/** @class CountedRefPtr
 * This class implements a smart pointer which handles pointer-style access
 * to a reference-counted structure and destructing the latter after use.
 *
 * The template arguments, include the pointer type @c PtrType, and two
 * integral (bool) properties: use @c isWeak to disallow destruction
 * and @c NeverNull to assume, that @c PtrType cannot be @c NULL.
 * Finally, @c CountType allows you to select a typ to represent the internal reference count.
 *
 * @note The class of @c PtrType must have an accessible integral attribute @c ref.
 * For convenience use @c RefCounter as public base.
 * In addition you must overload @c void CountedRefPtr_kill(PtrType) accordingly.
 **/
template <class PtrType, bool isWeak = false, bool NeverNull = false, class CountType = short>
class CountedRefPtr {
  typedef CountedRefPtr self;

public:
  //{ @name Name template arguments
  typedef PtrType ptr_type;
  typedef CountType count_type;
  enum { is_weak = isWeak, never_null = NeverNull };
  //}

  /// Convert from pointer
  CountedRefPtr(ptr_type ptr): m_ptr(ptr) { reclaim(); }

  /// Convert from compatible smart pointer
  template <bool Never>
  CountedRefPtr(const CountedRefPtr<ptr_type, !is_weak, Never, count_type>& rhs):
    m_ptr(rhs.m_ptr) { reclaim(); }

  /// Construct refernce copy
  CountedRefPtr(const self& rhs):
    m_ptr(rhs.m_ptr) { reclaim(); }

  /// Unlink one reference
  ~CountedRefPtr() { release(); }

  //{ @name Replace data behind reference
  self& operator=(const self& rhs) { return operator=(rhs.m_ptr); }
  self& operator=(ptr_type ptr) {
    release();
    m_ptr = ptr;
    reclaim();
    return *this;
  }
  //}

  /// Checking equality 
  bool operator==(const self& rhs) const { return m_ptr == rhs.m_ptr; }

  //{ @name Pointer-style interface
  bool operator==(ptr_type ptr) const { return m_ptr == ptr; }
  operator bool() const { return NeverNull || m_ptr; }
  operator const ptr_type() const { return m_ptr; }
  operator ptr_type() { return m_ptr; }
  const ptr_type operator->() const { return *this; }
  ptr_type operator->() { return *this; }
  //}

  //{ @name Reference count interface
  count_type count() const { return (*this? m_ptr->ref: 1); }
  void reclaim() { if (*this) ++m_ptr->ref; }
  void release() { 
    if (*this && (--m_ptr->ref <= 0) && !is_weak)
      CountedRefPtr_kill(m_ptr); 
  }
  //}

private:
  /// Store actual pointer
  ptr_type m_ptr;
};


/** @class RefCounter
 * This class implements implements a refernce counter which we can use
 * as a public base of objects managed by @CountedRefPtr.
 **/
class RefCounter {

public:
  /// Name numerical type for enumbering
  typedef short count_type;

  /// Allow our smart pointer to access internals
  template <class, bool, bool, class> friend class CountedRefPtr;

  /// Any Constructor resets the counter
  RefCounter(...): ref(0) {}

  /// Destructor
  ~RefCounter() { assume(ref == 0); }

private:
  /// Number of references
  count_type ref;  // naming consistent with other classes
};

class CountedRefEnv {
  typedef CountedRefEnv self;

public:
  static idhdl idify(leftv head, idhdl* root) {
    static unsigned int counter = 0;
    char* name = (char*) omAlloc0(512);
    sprintf(name, " :%u:%p:_shared_: ", ++counter, head->data);
    if ((*root) == NULL )
      enterid(name, 0, head->rtyp, root, TRUE, FALSE);
    else
      *root = (*root)->set(name, 0, head->rtyp, TRUE);

    IDDATA(*root) = (char*) head->data;
    return *root;
  }

  static void clearid(idhdl handle, idhdl* root) {
    IDDATA(handle)=NULL;
    IDTYP(handle)=NONE;
    killhdl2(handle, root, NULL);
  }
  static int& ref_id() {
    static int g_ref_id = 0;
    return g_ref_id;
  }

  static int& sh_id() {
    static int g_sh_id = 0;
    return g_sh_id;
  }
  static int& idx_id() {
    static int g_sh_id = 0;
    return g_sh_id;
  }

};

/// Overloading ring destruction
inline void CountedRefPtr_kill(ring r) { rKill(r); }


class LeftvShallow {
  typedef LeftvShallow self;
  
public:
  LeftvShallow(): m_data(allocate()) { }
  LeftvShallow(leftv data): 
    m_data(allocate()) { init(data); }

  LeftvShallow(const self& rhs):
    m_data(allocate()) {
    copy(m_data, rhs.m_data);
  }

  ~LeftvShallow() {  
    kill(m_data->e);
    omFree(m_data);
  }
  self& operator=(leftv rhs) {
    kill(m_data->e);
    return init(rhs);
  }

  self& operator=(const self& rhs) { return (*this) = rhs.m_data; }



  BOOLEAN get(leftv result) {
    leftv next = result->next;
    result->next = NULL;
    result->CleanUp();

    copy(result, m_data);
    result->next = next;
    return FALSE;
  }

  /// Access to object
  const leftv operator->() const { return m_data;  }
  leftv operator->() { return m_data;  }

protected:
  static leftv allocate() { return (leftv)omAlloc0(sizeof(sleftv)); }

  self& init(leftv data) {
    memcpy(m_data, data, sizeof(sleftv));
    data->e = NULL;
    m_data->next = NULL;
    return *this;
  }

  static void copy(leftv result, leftv data)  {
    memcpy(result, data, sizeof(sleftv));
    copy(result->e, data->e);
  }

 static void copy(Subexpr& current, Subexpr rhs)  {
    if (rhs == NULL) return;
    current = (Subexpr)memcpy(omAlloc0Bin(sSubexpr_bin), rhs, sizeof(*rhs));
    copy(current->next, rhs->next);
  }

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
  LeftvDeep(leftv data): base(data) {
    if(m_data->rtyp != IDHDL) {
      m_data->data = data->CopyD();
    }
  }

  LeftvDeep(leftv data,int,int): base() {  m_data->Copy(data);  }

  LeftvDeep(const self& rhs): base(rhs) {
    if(m_data->rtyp != IDHDL)
      m_data->Copy(rhs.m_data);
  }

  ~LeftvDeep() { m_data->CleanUp(); }

  self& operator=(const self& rhs) { return operator=(rhs.m_data); }
  self& operator=(leftv rhs) {
    m_data->e = rhs->e;
    rhs->e=NULL;
    if(m_data->rtyp == IDHDL) {
      IDTYP((idhdl)m_data->data) =  rhs->Typ();
      IDDATA((idhdl)m_data->data) = (char*) rhs->CopyD();
    }
    else {
      m_data->CleanUp();
      m_data->rtyp = rhs->Typ();
      m_data->data =  rhs->CopyD();
    }

    return *this;
  }

   leftv access() { return m_data; }
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
  explicit CountedRefData(leftv data):
    base(), m_data(data), m_ring(parent(data)) { }

  CountedRefData(leftv data, BOOLEAN global, int):
    base(), m_data(data, global,0), m_ring(parent(data)) { }

  /// Construct deep copy
  CountedRefData(const self& rhs):
    base(), m_data(rhs.m_data), m_ring(rhs.m_ring)  { }
  
  /// Destruct
  ~CountedRefData()  { }

  /// Replace data
  self& operator=(const self& rhs) {
    m_data = rhs.m_data;
    m_ring = rhs.m_ring;
    return *this;
  }

  /// Replace with other Singular data
  void set(leftv rhs) {
    if (m_data->rtyp==IDHDL)
      m_data = rhs;
   else
     m_data->Copy(rhs);

    m_ring = parent(rhs);
  }

  /// Write (shallow) copy to given handle
  BOOLEAN get(leftv res) { return broken() || m_data.get(res);  }

  /// Extract (shallow) copy of stored data
  LeftvShallow operator*() const { return (broken()? LeftvShallow(): LeftvShallow(m_data)); }


  BOOLEAN rering() {
    if (m_ring ^ m_data->RingDependend()) m_ring = (m_ring? NULL: currRing);
    return FALSE;
  }

  /// Get the current context
  idhdl* root() { return  (m_ring? &m_ring->idroot: &IDROOT); }

  /// Check whether identifier became invalid
  BOOLEAN broken() const {
    if (m_ring) {
      if (m_ring != currRing) 
        return complain("Referenced identifier not from current ring");    

      return (m_data->rtyp == IDHDL)  && brokenid(currRing->idroot) &&
        complain("Referenced identifier not available in ring anymore");  
    }

   if (m_data->rtyp != IDHDL) return FALSE;
   return brokenid(IDROOT) &&
     ((currPack == basePack) || brokenid(basePack->idroot)) &&
     complain("Referenced identifier not available in current context");
  }

  BOOLEAN assign(leftv result, leftv arg) {
    return get(result) || iiAssign(result, arg) || rering();
  }

  /// @note Enables write-access via identifier
  idhdl idify() {  return CountedRefEnv::idify(m_data.access(), root());  }

  /// @note Only call, if @c idify had been called before!
  void clearid() {  CountedRefEnv::clearid((idhdl)m_data.access()->data, root());  }

  BOOLEAN retrieve(leftv res) {
    if (res->data == m_data.access()->data)  {
      memcpy(m_data.access(), res, sizeof(sleftv));
      res->Init();
      return TRUE;
    }
    return FALSE;
  }

  CountedRefPtr<ring, true> Ring() { return m_ring; }

private:
  /// Raise error message and return @c TRUE
  BOOLEAN complain(const char* text) const  {
    Werror(text);
    return TRUE;
  }

  /// Check a given context for our identifier
  BOOLEAN brokenid(idhdl context) const {
    return (context == NULL) || 
      ((context != (idhdl) m_data->data) && brokenid(IDNEXT(context)));
  }

  /// Store ring for ring-dependent objects
  static ring parent(leftv rhs) { 
    return (rhs->RingDependend()? currRing: NULL); 
  }
protected:
  /// Singular object
  LeftvDeep m_data;

  /// Store namespace for ring-dependent objects
  CountedRefPtr<ring, true> m_ring;
};

/// blackbox support - initialization
/// @note deals as marker for compatible references, too.
void* countedref_Init(blackbox*)
{
  return NULL;
}


inline void CountedRefPtr_kill(CountedRefData* data) { delete data; }

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
  CountedRef(leftv arg):  m_data(new data_type(arg)) { }

protected:
  /// Recover previously constructed reference
  CountedRef(data_type* arg):  m_data(arg) { assume(arg); }

public:
  /// Construct copy
  CountedRef(const self& rhs): m_data(rhs.m_data) { }

  /// Replace reference
  self& operator=(const self& rhs) {
    m_data = rhs.m_data;
    return *this;
  }

  /// Replace data that reference is pointing to
  self& operator=(leftv rhs) {
    m_data->set(rhs);
    return *this;
  }

  BOOLEAN assign(leftv result, leftv arg) { 
    return m_data->assign(result,arg);
  }

  /// Extract (shallow) copy of stored data
  LeftvShallow operator*() { return m_data->operator*(); }

  /// Construct reference data object from
  BOOLEAN outcast(leftv result) {
    if (result->rtyp == IDHDL)
      IDDATA((idhdl)result->data) = (char *)outcast();
    else
      result->data = (void *)outcast();
    return FALSE;
  }
  data_type* outcast() { 
    m_data.reclaim();
    return m_data;
  }

  /// Kills a link to the referenced object
  void destruct() { m_data.release(); }

  /// Kills the link to the referenced object
  ~CountedRef() { }

  /// Replaces argument by a shallow copy of the references data
  BOOLEAN dereference(leftv arg) {
    //    assume(is_ref(arg));
    m_data.reclaim();
    BOOLEAN b= m_data->get(arg) || ((arg->next != NULL) && resolve(arg->next));
    m_data.release();
    return b;
  }

  BOOLEAN broken() {return m_data->broken(); }

  /// Get number of references pointing here, too
  BOOLEAN count(leftv res) { return construct(res, m_data.count() - 1); }

  // Get internal indentifier
  BOOLEAN hash(leftv res) { return construct(res, (long)(data_type*)m_data); }

  /// Check for likewise identifiers
  BOOLEAN likewise(leftv res, leftv arg) {
    return resolve(arg) || construct(res, operator*()->data == arg->data); 
  }

  /// Check for identical reference objects
  BOOLEAN same(leftv res, leftv arg) { 
    return construct(res, m_data == arg->Data());
  }

  /// Get type of references data
  BOOLEAN type(leftv res) { 
    return construct(res, Tok2Cmdname(operator*()->Typ()));
  };

  /// Get (possibly) internal identifier name
  BOOLEAN name(leftv res) { return construct(res, operator*()->Name()); }

  /// Recover the actual object from raw Singular data
  static self cast(void* data) {
    assume(data != NULL);
    return self(static_cast<data_type*>(data));
  }

  /// Recover the actual object from Singular interpreter object
  static self cast(leftv arg) {
    assume((arg != NULL) && is_ref(arg));
    return self::cast(arg->Data());
  }

  /// If necessary dereference.
  static BOOLEAN resolve(leftv arg) {
    assume(arg != NULL);
    while (is_ref(arg)) { if(CountedRef::cast(arg).dereference(arg)) return TRUE; };
    return (arg->next != NULL) && resolve(arg->next);
  }

protected:
  /// Construct integer value
  static BOOLEAN construct(leftv res, long data) {
    res->data = (void*) data;
    res->rtyp = INT_CMD;
    return FALSE;
  }

  /// Construct string
  static BOOLEAN construct(leftv res, const char* data) {
    res->data = (void*)omStrDup(data);
    res->rtyp = STRING_CMD;
    return FALSE;
  }

  /// Store pointer to actual data
  CountedRefPtr<data_type*> m_data;
};

/// blackbox support - convert to string representation
void countedref_Print(blackbox *b, void* ptr)
{
  if (ptr) (*CountedRef::cast(ptr))->Print();
  else PrintS("<unassigned reference or shared memory>");
}

/// blackbox support - convert to string representation
char* countedref_String(blackbox *b, void* ptr)
{
  if (ptr == NULL) return omStrDup(sNoName);
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
    return CountedRef::resolve(arg) || 
      CountedRef::cast(result).assign(result, arg);	
  }
  
  // Case: copy reference
  if (result->Typ() == arg->Typ())
    return CountedRef::cast(arg).outcast(result);

  // Case: new reference
  if ((arg->rtyp == IDHDL) || CountedRef::is_ref(arg))
    return CountedRef(arg).outcast(result);

  Werror("Can only take reference from identifier");
  return TRUE;
}

BOOLEAN countedref_CheckInit(leftv res, leftv arg)
{
  if (arg->Data() != NULL) return FALSE;
  res->rtyp = NONE;
  Werror("Noninitialized access");
  return TRUE;
}
                                                                 
/// blackbox support - unary operations
BOOLEAN countedref_Op1(int op, leftv res, leftv head)
{
  if(op == TYPEOF_CMD)
    return blackboxDefaultOp1(op, res, head);

  if (countedref_CheckInit(res, head)) return TRUE;

  if(op == head->Typ()) {
    res->rtyp = op;
    return iiAssign(res, head);
  }

  if(op == DEF_CMD) {
    res->rtyp = DEF_CMD;
    return CountedRef::cast(head).dereference(head) || iiAssign(res, head);
  }

  return CountedRef::cast(head).dereference(head) || iiExprArith1(res, head, op);
}

/// blackbox support - binary operations
BOOLEAN countedref_Op2(int op, leftv res, leftv head, leftv arg)
{
   return countedref_CheckInit(res, head) ||
    CountedRef::cast(head).dereference(head) ||
    CountedRef::resolve(arg) ||
    iiExprArith2(res, head, op, arg);
}

class CountedRefDataIndexed:
  public CountedRefData {
  typedef CountedRefData base;
  typedef CountedRefDataIndexed self;

public:
  CountedRefDataIndexed(idhdl handle, CountedRefPtr<base*> back):
    base(init(handle)), m_back(back) { 
    m_ring = back->Ring(); 
  } 

  CountedRefDataIndexed(const self& rhs): base(rhs), m_back(rhs.m_back) { } 

  ~CountedRefDataIndexed() { clearid();}

  BOOLEAN assign(leftv result, leftv arg) {
    return base::assign(result, arg) || m_back->rering();
  }

private:
  static leftv init(idhdl handle) {
    assume(handle);
    leftv res = (leftv)omAlloc0(sizeof(*res));
    res->data =(void*) handle;
    res->rtyp = IDHDL;
    return res;
  }

  CountedRefPtr<CountedRefData*> m_back;
};
inline void CountedRefPtr_kill(CountedRefDataIndexed* data) { delete data; }



/// blackbox support - ternary operations
BOOLEAN countedref_Op3(int op, leftv res, leftv head, leftv arg1, leftv arg2)
{
  return countedref_CheckInit(res, head) ||
    CountedRef::cast(head).dereference(head) || 
    CountedRef::resolve(arg1) || CountedRef::resolve(arg2) ||
    iiExprArith3(res, op, head, arg1, arg2);
}



/// blackbox support - destruction
void countedref_destroy(blackbox *b, void* ptr)
{
  if (ptr) CountedRef::cast(ptr).destruct();
}


/// blackbox support - assign element
BOOLEAN countedref_AssignIndexed(leftv result, leftv arg)
{
  // Case: replace assignment behind reference
  if (result->Data() != NULL) {
    CountedRefPtr<CountedRefDataIndexed*> indexed =(CountedRefDataIndexed*)(result->Data());
    return CountedRef::resolve(arg) || indexed->assign(result, arg);
  }
  
  // Case: copy reference
  if (result->Typ() == arg->Typ())
    return CountedRef::cast(arg).outcast(result);

  Werror("Cannot generate subscripted shared from plain type. Use shared[i] or shared.attr");
  return TRUE;
}


/// blackbox support - destruction
void countedref_destroyIndexed(blackbox *b, void* ptr)
{
  if (ptr) {
    CountedRefPtr<CountedRefDataIndexed*> data =
    static_cast<CountedRefDataIndexed*>(ptr);
    data.release();
  }
}

class CountedRefShared:
  public CountedRef {
  typedef CountedRefShared self;
  typedef CountedRef base;

  CountedRefShared(const base& rhs):  base(rhs) { }

public:
  CountedRefShared(leftv arg):  base(new data_type(arg, FALSE,0)) { }

  /// Construct copy
  CountedRefShared(const self& rhs): base(rhs) { }

  ~CountedRefShared() { }

  self& operator=(const self& rhs) {
    return static_cast<self&>(base::operator=(rhs));
  }

  /// Replace data that reference is pointing to
  self& operator=(leftv rhs) {
    return static_cast<self&>(base::operator=(rhs));
  }

  static self cast(leftv arg) { return base::cast(arg); }
  static self cast(void* arg) { return base::cast(arg); }

  CountedRefPtr<CountedRefDataIndexed*> subscripted() {
    return new CountedRefDataIndexed(m_data->idify(), m_data);
  }
};



/// blackbox support - binary operations
BOOLEAN countedref_Op2Shared(int op, leftv res, leftv head, leftv arg)
{
  if  ((op == '[') || (op == '.')) {
    if (countedref_CheckInit(res, head))  return TRUE;
    CountedRefPtr<CountedRefDataIndexed*> indexed = CountedRefShared::cast(head).subscripted();
    if(indexed->operator*().get(head)) return TRUE;
 
    if (CountedRef::resolve(arg) || iiExprArith2(res, head, op, arg)) return
      TRUE;

    if(indexed->retrieve(res)) {
      indexed.reclaim();
      res->rtyp = CountedRefEnv::idx_id();
      res->data = (void *)indexed;
    }
    return FALSE;
  }

  return countedref_Op2(op, res, head, arg);
}

/// blackbox support - n-ary operations
BOOLEAN countedref_OpM(int op, leftv res, leftv args)
{
  if (args->Data() == NULL) return FALSE;

  if(op == SYSTEM_CMD) {
    if (args->next) {
      leftv next = args->next;
      args->next = NULL;
      CountedRef obj = CountedRef::cast(args);
      char* name = (next->Typ() == STRING_CMD? 
                    (char*) next->Data(): (char*)next->Name());
      next = next->next;
      if (next) {
        if (strcmp(name, "same") == 0) return obj.same(res, next);
        if (strncmp(name, "like", 4) == 0) return obj.likewise(res, next);
      }
      if (strncmp(name, "count", 5) == 0) return obj.count(res);
      if (strcmp(name, "hash") == 0) return obj.hash(res);
      if (strcmp(name, "name") == 0) return obj.name(res);
      if (strncmp(name, "type", 4) == 0) return obj.type(res);
    }
    return TRUE;
  }
  return CountedRef::cast(args).dereference(args) || iiExprArithM(res, args, op);
}
/// blackbox support - assign element
BOOLEAN countedref_AssignShared(leftv result, leftv arg)
{
  /// Case: replace assignment behind reference
  if ((result->Data()) != NULL) {
    if (CountedRefShared::resolve(arg)) return TRUE;
    CountedRefShared::cast(result) = arg;
    return FALSE;
  }
  
  /// Case: new reference to already shared data
  if (result->Typ() == arg->Typ()) 
    return CountedRefShared::cast(arg).outcast(result);

  /// Case: new shared data
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
  CountedRefEnv::ref_id()=setBlackboxStuff(bbx, "reference");

  /// The @c shared type is "inherited" from @c reference.
  /// It just uses another constructor (to make its own copy of the).
  blackbox *bbxshared = 
    (blackbox*)memcpy(omAlloc(sizeof(blackbox)), bbx, sizeof(blackbox));
  bbxshared->blackbox_Assign  = countedref_AssignShared;
  bbxshared->blackbox_destroy = countedref_destroyShared;
  bbxshared->blackbox_Op2     = countedref_Op2Shared;
  bbxshared->data             = omAlloc0(newstruct_desc_size());
  CountedRefEnv::sh_id()=setBlackboxStuff(bbxshared, "shared");

  blackbox *bbxindexed = 
    (blackbox*)memcpy(omAlloc(sizeof(blackbox)), bbx, sizeof(blackbox));
  bbxindexed->blackbox_destroy = countedref_destroyIndexed;
  bbxindexed->blackbox_Assign = countedref_AssignIndexed;
  bbxindexed->data             = omAlloc0(newstruct_desc_size());
  CountedRefEnv::idx_id()=setBlackboxStuff(bbxindexed, "shared_subexpr");

}

extern "C" { void mod_init() { countedref_init(); } }

