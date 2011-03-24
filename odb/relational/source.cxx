// file      : odb/relational/source.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/relational/source.hxx>
#include <odb/relational/generate.hxx>

using namespace std;

namespace relational
{
  namespace source
  {
    void
    generate ()
    {
      context ctx;
      ostream& os (ctx.os);

      traversal::unit unit;
      traversal::defines unit_defines;
      traversal::namespace_ ns;
      instance<class_> c;

      unit >> unit_defines >> ns;
      unit_defines >> c;

      traversal::defines ns_defines;

      ns >> ns_defines >> ns;
      ns_defines >> c;

      instance<include> i;
      i->generate ();

      os << "namespace odb"
         << "{";

      unit.dispatch (ctx.unit);

      os << "}";
    }
  }
}
