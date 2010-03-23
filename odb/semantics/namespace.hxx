// file      : odb/semantics/namespace.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_NAMESPACE_HXX
#define ODB_SEMANTICS_NAMESPACE_HXX

#include <semantics/elements.hxx>

namespace semantics
{
  class namespace_: public scope
  {
  public:
    namespace_ (path const& file, size_t line, size_t column)
        : node (file, line, column)
    {
    }

    namespace_ ()
    {
    }

    // Resolve conflict between scope::scope and nameable::scope.
    //
    using nameable::scope;
  };
}

#endif // ODB_SEMANTICS_NAMESPACE_HXX
