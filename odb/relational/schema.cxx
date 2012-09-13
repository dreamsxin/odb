// file      : odb/relational/schema.cxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v3; see accompanying LICENSE file

#include <cassert>
#include <limits>
#include <sstream>

#include <odb/emitter.hxx>

#include <odb/relational/schema.hxx>
#include <odb/relational/generate.hxx>

using namespace std;

namespace relational
{
  namespace schema
  {
    // cxx_object
    //
    schema_format cxx_object::format_embedded (schema_format::embedded);

    void
    generate_prologue ()
    {
      instance<sql_file> file;
      file->prologue ();
    }

    void
    generate_epilogue ()
    {
      instance<sql_file> file;
      file->epilogue ();
    }

    void
    generate_drop ()
    {
      context ctx;
      instance<sql_emitter> em;
      emitter_ostream emos (*em);

      schema_format f (schema_format::sql);

      instance<drop_model> model (*em, emos, f);
      instance<drop_table> table (*em, emos, f);
      trav_rel::qnames names;

      model >> names >> table;

      // Pass 1 and 2.
      //
      for (unsigned short pass (1); pass < 3; ++pass)
      {
        model->pass (pass);
        table->pass (pass);

        model->traverse (*ctx.model);
      }
    }

    void
    generate_create ()
    {
      context ctx;
      instance<sql_emitter> em;
      emitter_ostream emos (*em);

      schema_format f (schema_format::sql);

      instance<create_model> model (*em, emos, f);
      instance<create_table> table (*em, emos, f);
      trav_rel::qnames names;

      model >> names >> table;

      // Pass 1 and 2.
      //
      for (unsigned short pass (1); pass < 3; ++pass)
      {
        model->pass (pass);
        table->pass (pass);

        model->traverse (*ctx.model);
      }
    }
  }
}
