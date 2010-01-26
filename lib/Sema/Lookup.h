//===--- Lookup.h - Classes for name lookup ---------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the LookupResult class, which is integral to
// Sema's name-lookup subsystem.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_SEMA_LOOKUP_H
#define LLVM_CLANG_SEMA_LOOKUP_H

#include "Sema.h"

namespace clang {

/// @brief Represents the results of name lookup.
///
/// An instance of the LookupResult class captures the results of a
/// single name lookup, which can return no result (nothing found),
/// a single declaration, a set of overloaded functions, or an
/// ambiguity. Use the getKind() method to determine which of these
/// results occurred for a given lookup.
class LookupResult {
public:
  enum LookupResultKind {
    /// @brief No entity found met the criteria.
    NotFound = 0,

    /// @brief No entity found met the criteria within the current 
    /// instantiation,, but there were dependent base classes of the 
    /// current instantiation that could not be searched.
    NotFoundInCurrentInstantiation,
    
    /// @brief Name lookup found a single declaration that met the
    /// criteria.  getFoundDecl() will return this declaration.
    Found,

    /// @brief Name lookup found a set of overloaded functions that
    /// met the criteria.
    FoundOverloaded,

    /// @brief Name lookup found an unresolvable value declaration
    /// and cannot yet complete.  This only happens in C++ dependent
    /// contexts with dependent using declarations.
    FoundUnresolvedValue,

    /// @brief Name lookup results in an ambiguity; use
    /// getAmbiguityKind to figure out what kind of ambiguity
    /// we have.
    Ambiguous
  };

  enum AmbiguityKind {
    /// Name lookup results in an ambiguity because multiple
    /// entities that meet the lookup criteria were found in
    /// subobjects of different types. For example:
    /// @code
    /// struct A { void f(int); }
    /// struct B { void f(double); }
    /// struct C : A, B { };
    /// void test(C c) {
    ///   c.f(0); // error: A::f and B::f come from subobjects of different
    ///           // types. overload resolution is not performed.
    /// }
    /// @endcode
    AmbiguousBaseSubobjectTypes,

    /// Name lookup results in an ambiguity because multiple
    /// nonstatic entities that meet the lookup criteria were found
    /// in different subobjects of the same type. For example:
    /// @code
    /// struct A { int x; };
    /// struct B : A { };
    /// struct C : A { };
    /// struct D : B, C { };
    /// int test(D d) {
    ///   return d.x; // error: 'x' is found in two A subobjects (of B and C)
    /// }
    /// @endcode
    AmbiguousBaseSubobjects,

    /// Name lookup results in an ambiguity because multiple definitions
    /// of entity that meet the lookup criteria were found in different
    /// declaration contexts.
    /// @code
    /// namespace A {
    ///   int i;
    ///   namespace B { int i; }
    ///   int test() {
    ///     using namespace B;
    ///     return i; // error 'i' is found in namespace A and A::B
    ///    }
    /// }
    /// @endcode
    AmbiguousReference,

    /// Name lookup results in an ambiguity because an entity with a
    /// tag name was hidden by an entity with an ordinary name from
    /// a different context.
    /// @code
    /// namespace A { struct Foo {}; }
    /// namespace B { void Foo(); }
    /// namespace C {
    ///   using namespace A;
    ///   using namespace B;
    /// }
    /// void test() {
    ///   C::Foo(); // error: tag 'A::Foo' is hidden by an object in a
    ///             // different namespace
    /// }
    /// @endcode
    AmbiguousTagHiding
  };

  /// A little identifier for flagging temporary lookup results.
  enum TemporaryToken {
    Temporary
  };

  typedef UnresolvedSetImpl::iterator iterator;
  typedef bool (*ResultFilter)(NamedDecl*, unsigned IDNS);

  LookupResult(Sema &SemaRef, DeclarationName Name, SourceLocation NameLoc,
               Sema::LookupNameKind LookupKind,
               Sema::RedeclarationKind Redecl = Sema::NotForRedeclaration)
    : ResultKind(NotFound),
      Paths(0),
      NamingClass(0),
      SemaRef(SemaRef),
      Name(Name),
      NameLoc(NameLoc),
      LookupKind(LookupKind),
      IsAcceptableFn(0),
      IDNS(0),
      Redecl(Redecl != Sema::NotForRedeclaration),
      HideTags(true),
      Diagnose(Redecl == Sema::NotForRedeclaration)
  {
    configure();
  }

  /// Creates a temporary lookup result, initializing its core data
  /// using the information from another result.  Diagnostics are always
  /// disabled.
  LookupResult(TemporaryToken _, const LookupResult &Other)
    : ResultKind(NotFound),
      Paths(0),
      NamingClass(0),
      SemaRef(Other.SemaRef),
      Name(Other.Name),
      NameLoc(Other.NameLoc),
      LookupKind(Other.LookupKind),
      IsAcceptableFn(Other.IsAcceptableFn),
      IDNS(Other.IDNS),
      Redecl(Other.Redecl),
      HideTags(Other.HideTags),
      Diagnose(false)
  {}

  ~LookupResult() {
    if (Diagnose) diagnose();
    if (Paths) deletePaths(Paths);
  }

  /// Gets the name to look up.
  DeclarationName getLookupName() const {
    return Name;
  }

  /// \brief Sets the name to look up.
  void setLookupName(DeclarationName Name) {
    this->Name = Name;
  }

  /// Gets the kind of lookup to perform.
  Sema::LookupNameKind getLookupKind() const {
    return LookupKind;
  }

  /// True if this lookup is just looking for an existing declaration.
  bool isForRedeclaration() const {
    return Redecl;
  }

  /// Sets whether tag declarations should be hidden by non-tag
  /// declarations during resolution.  The default is true.
  void setHideTags(bool Hide) {
    HideTags = Hide;
  }

  bool isAmbiguous() const {
    return getResultKind() == Ambiguous;
  }

  /// Determines if this names a single result which is not an
  /// unresolved value using decl.  If so, it is safe to call
  /// getFoundDecl().
  bool isSingleResult() const {
    return getResultKind() == Found;
  }

  /// Determines if the results are overloaded.
  bool isOverloadedResult() const {
    return getResultKind() == FoundOverloaded;
  }

  bool isUnresolvableResult() const {
    return getResultKind() == FoundUnresolvedValue;
  }

  LookupResultKind getResultKind() const {
    sanity();
    return ResultKind;
  }

  AmbiguityKind getAmbiguityKind() const {
    assert(isAmbiguous());
    return Ambiguity;
  }

  const UnresolvedSetImpl &asUnresolvedSet() const {
    return Decls;
  }

  iterator begin() const { return iterator(Decls.begin()); }
  iterator end() const { return iterator(Decls.end()); }

  /// \brief Return true if no decls were found
  bool empty() const { return Decls.empty(); }

  /// \brief Return the base paths structure that's associated with
  /// these results, or null if none is.
  CXXBasePaths *getBasePaths() const {
    return Paths;
  }

  /// \brief Tests whether the given declaration is acceptable.
  bool isAcceptableDecl(NamedDecl *D) const {
    assert(IsAcceptableFn);
    return IsAcceptableFn(D, IDNS);
  }

  /// \brief Returns the identifier namespace mask for this lookup.
  unsigned getIdentifierNamespace() const {
    return IDNS;
  }

  /// \brief Returns whether these results arose from performing a
  /// lookup into a class.
  bool isClassLookup() const {
    return NamingClass != 0;
  }

  /// \brief Returns the 'naming class' for this lookup, i.e. the
  /// class which was looked into to find these results.
  ///
  /// C++0x [class.access.base]p5:
  ///   The access to a member is affected by the class in which the
  ///   member is named. This naming class is the class in which the
  ///   member name was looked up and found. [Note: this class can be
  ///   explicit, e.g., when a qualified-id is used, or implicit,
  ///   e.g., when a class member access operator (5.2.5) is used
  ///   (including cases where an implicit "this->" is added). If both
  ///   a class member access operator and a qualified-id are used to
  ///   name the member (as in p->T::m), the class naming the member
  ///   is the class named by the nested-name-specifier of the
  ///   qualified-id (that is, T). -- end note ]
  ///
  /// This is set by the lookup routines when they find results in a class.
  CXXRecordDecl *getNamingClass() const {
    return NamingClass;
  }

  /// \brief Sets the 'naming class' for this lookup.
  void setNamingClass(CXXRecordDecl *Record) {
    NamingClass = Record;
  }

  /// \brief Add a declaration to these results with its natural access.
  /// Does not test the acceptance criteria.
  void addDecl(NamedDecl *D) {
    addDecl(D, D->getAccess());
  }

  /// \brief Add a declaration to these results with the given access.
  /// Does not test the acceptance criteria.
  void addDecl(NamedDecl *D, AccessSpecifier AS) {
    Decls.addDecl(D, AS);
    ResultKind = Found;
  }

  /// \brief Add all the declarations from another set of lookup
  /// results.
  void addAllDecls(const LookupResult &Other) {
    Decls.append(Other.Decls.begin(), Other.Decls.end());
    ResultKind = Found;
  }

  /// \brief Determine whether no result was found because we could not
  /// search into dependent base classes of the current instantiation.
  bool wasNotFoundInCurrentInstantiation() const {
    return ResultKind == NotFoundInCurrentInstantiation;
  }
  
  /// \brief Note that while no result was found in the current instantiation,
  /// there were dependent base classes that could not be searched.
  void setNotFoundInCurrentInstantiation() {
    assert(ResultKind == NotFound && Decls.empty());
    ResultKind = NotFoundInCurrentInstantiation;
  }
  
  /// \brief Resolves the result kind of the lookup, possibly hiding
  /// decls.
  ///
  /// This should be called in any environment where lookup might
  /// generate multiple lookup results.
  void resolveKind();

  /// \brief Re-resolves the result kind of the lookup after a set of
  /// removals has been performed.
  void resolveKindAfterFilter() {
    if (Decls.empty()) {
      if (ResultKind != NotFoundInCurrentInstantiation)
        ResultKind = NotFound;
    } else {
      ResultKind = Found;
      resolveKind();
    }
  }

  template <class DeclClass>
  DeclClass *getAsSingle() const {
    if (getResultKind() != Found) return 0;
    return dyn_cast<DeclClass>(getFoundDecl());
  }

  /// \brief Fetch the unique decl found by this lookup.  Asserts
  /// that one was found.
  ///
  /// This is intended for users who have examined the result kind
  /// and are certain that there is only one result.
  NamedDecl *getFoundDecl() const {
    assert(getResultKind() == Found
           && "getFoundDecl called on non-unique result");
    return (*begin())->getUnderlyingDecl();
  }

  /// Fetches a representative decl.  Useful for lazy diagnostics.
  NamedDecl *getRepresentativeDecl() const {
    assert(!Decls.empty() && "cannot get representative of empty set");
    return *begin();
  }

  /// \brief Asks if the result is a single tag decl.
  bool isSingleTagDecl() const {
    return getResultKind() == Found && isa<TagDecl>(getFoundDecl());
  }

  /// \brief Make these results show that the name was found in
  /// base classes of different types.
  ///
  /// The given paths object is copied and invalidated.
  void setAmbiguousBaseSubobjectTypes(CXXBasePaths &P);

  /// \brief Make these results show that the name was found in
  /// distinct base classes of the same type.
  ///
  /// The given paths object is copied and invalidated.
  void setAmbiguousBaseSubobjects(CXXBasePaths &P);

  /// \brief Make these results show that the name was found in
  /// different contexts and a tag decl was hidden by an ordinary
  /// decl in a different context.
  void setAmbiguousQualifiedTagHiding() {
    setAmbiguous(AmbiguousTagHiding);
  }

  /// \brief Clears out any current state.
  void clear() {
    ResultKind = NotFound;
    Decls.clear();
    if (Paths) deletePaths(Paths);
    Paths = NULL;
  }

  /// \brief Clears out any current state and re-initializes for a
  /// different kind of lookup.
  void clear(Sema::LookupNameKind Kind) {
    clear();
    LookupKind = Kind;
    configure();
  }

  void print(llvm::raw_ostream &);

  /// Suppress the diagnostics that would normally fire because of this
  /// lookup.  This happens during (e.g.) redeclaration lookups.
  void suppressDiagnostics() {
    Diagnose = false;
  }

  /// Sets a 'context' source range.
  void setContextRange(SourceRange SR) {
    NameContextRange = SR;
  }

  /// Gets the source range of the context of this name; for C++
  /// qualified lookups, this is the source range of the scope
  /// specifier.
  SourceRange getContextRange() const {
    return NameContextRange;
  }

  /// Gets the location of the identifier.  This isn't always defined:
  /// sometimes we're doing lookups on synthesized names.
  SourceLocation getNameLoc() const {
    return NameLoc;
  }

  /// \brief Get the Sema object that this lookup result is searching
  /// with.
  Sema &getSema() const { return SemaRef; }

  /// A class for iterating through a result set and possibly
  /// filtering out results.  The results returned are possibly
  /// sugared.
  class Filter {
    LookupResult &Results;
    LookupResult::iterator I;
    bool Changed;
#ifndef NDEBUG
    bool CalledDone;
#endif
    
    friend class LookupResult;
    Filter(LookupResult &Results)
      : Results(Results), I(Results.begin()), Changed(false)
#ifndef NDEBUG
      , CalledDone(false)
#endif
    {}

  public:
#ifndef NDEBUG
    ~Filter() {
      assert(CalledDone &&
             "LookupResult::Filter destroyed without done() call");
    }
#endif

    bool hasNext() const {
      return I != Results.end();
    }

    NamedDecl *next() {
      assert(I != Results.end() && "next() called on empty filter");
      return *I++;
    }

    /// Erase the last element returned from this iterator.
    void erase() {
      Results.Decls.erase(--I);
      Changed = true;
    }

    /// Replaces the current entry with the given one, preserving the
    /// access bits.
    void replace(NamedDecl *D) {
      Results.Decls.replace(I-1, D);
      Changed = true;
    }

    /// Replaces the current entry with the given one.
    void replace(NamedDecl *D, AccessSpecifier AS) {
      Results.Decls.replace(I-1, D, AS);
      Changed = true;
    }

    void done() {
#ifndef NDEBUG
      assert(!CalledDone && "done() called twice");
      CalledDone = true;
#endif

      if (Changed)
        Results.resolveKindAfterFilter();
    }
  };

  /// Create a filter for this result set.
  Filter makeFilter() {
    return Filter(*this);
  }

private:
  void diagnose() {
    if (isAmbiguous())
      SemaRef.DiagnoseAmbiguousLookup(*this);
    else if (isClassLookup() && SemaRef.getLangOptions().AccessControl)
      SemaRef.CheckAccess(*this);
  }

  void setAmbiguous(AmbiguityKind AK) {
    ResultKind = Ambiguous;
    Ambiguity = AK;
  }

  void addDeclsFromBasePaths(const CXXBasePaths &P);
  void configure();

  // Sanity checks.
  void sanity() const {
    assert(ResultKind != NotFound || Decls.size() == 0);
    assert(ResultKind != Found || Decls.size() == 1);
    assert(ResultKind != FoundOverloaded || Decls.size() > 1 ||
           (Decls.size() == 1 &&
            isa<FunctionTemplateDecl>((*begin())->getUnderlyingDecl())));
    assert(ResultKind != FoundUnresolvedValue || sanityCheckUnresolved());
    assert(ResultKind != Ambiguous || Decls.size() > 1 ||
           (Decls.size() == 1 && Ambiguity == AmbiguousBaseSubobjects));
    assert((Paths != NULL) == (ResultKind == Ambiguous &&
                               (Ambiguity == AmbiguousBaseSubobjectTypes ||
                                Ambiguity == AmbiguousBaseSubobjects)));
  }

  bool sanityCheckUnresolved() const {
    for (iterator I = begin(), E = end(); I != E; ++I)
      if (isa<UnresolvedUsingValueDecl>(*I))
        return true;
    return false;
  }

  static void deletePaths(CXXBasePaths *);

  // Results.
  LookupResultKind ResultKind;
  AmbiguityKind Ambiguity; // ill-defined unless ambiguous
  UnresolvedSet<8> Decls;
  CXXBasePaths *Paths;
  CXXRecordDecl *NamingClass;

  // Parameters.
  Sema &SemaRef;
  DeclarationName Name;
  SourceLocation NameLoc;
  SourceRange NameContextRange;
  Sema::LookupNameKind LookupKind;
  ResultFilter IsAcceptableFn; // set by configure()
  unsigned IDNS; // set by configure()

  bool Redecl;

  /// \brief True if tag declarations should be hidden if non-tags
  ///   are present
  bool HideTags;

  bool Diagnose;
};

  /// \brief Consumes visible declarations found when searching for
  /// all visible names within a given scope or context.
  ///
  /// This abstract class is meant to be subclassed by clients of \c
  /// Sema::LookupVisibleDecls(), each of which should override the \c
  /// FoundDecl() function to process declarations as they are found.
  class VisibleDeclConsumer {
  public:
    /// \brief Destroys the visible declaration consumer.
    virtual ~VisibleDeclConsumer();

    /// \brief Invoked each time \p Sema::LookupVisibleDecls() finds a
    /// declaration visible from the current scope or context.
    ///
    /// \param ND the declaration found.
    ///
    /// \param Hiding a declaration that hides the declaration \p ND,
    /// or NULL if no such declaration exists.
    ///
    /// \param InBaseClass whether this declaration was found in base
    /// class of the context we searched.
    virtual void FoundDecl(NamedDecl *ND, NamedDecl *Hiding, 
                           bool InBaseClass) = 0;
  };

/// \brief A class for storing results from argument-dependent lookup.
class ADLResult {
private:
  /// A map from canonical decls to the 'most recent' decl.
  llvm::DenseMap<NamedDecl*, NamedDecl*> Decls;

public:
  /// Adds a new ADL candidate to this map.
  void insert(NamedDecl *D);

  /// Removes any data associated with a given decl.
  void erase(NamedDecl *D) {
    Decls.erase(cast<NamedDecl>(D->getCanonicalDecl()));
  }

  class iterator {
    typedef llvm::DenseMap<NamedDecl*,NamedDecl*>::iterator inner_iterator;
    inner_iterator iter;

    friend class ADLResult;
    iterator(const inner_iterator &iter) : iter(iter) {}
  public:
    iterator() {}

    iterator &operator++() { ++iter; return *this; }
    iterator operator++(int) { return iterator(iter++); }

    NamedDecl *operator*() const { return iter->second; }

    bool operator==(const iterator &other) const { return iter == other.iter; }
    bool operator!=(const iterator &other) const { return iter != other.iter; }
  };

  iterator begin() { return iterator(Decls.begin()); }
  iterator end() { return iterator(Decls.end()); }
};

}

#endif
