// file      : odb/relational/header.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_HEADER_HXX
#define ODB_RELATIONAL_HEADER_HXX

#include <odb/relational/context.hxx>
#include <odb/relational/common.hxx>

namespace relational
{
  namespace header
  {
    //
    // image_type
    //

    struct image_member: virtual member_base
    {
      typedef image_member base;

      image_member (string const& var = string ())
          : member_base (var, 0, string (), string ())
      {
      }

      image_member (string const& var,
                    semantics::type& t,
                    string const& fq_type,
                    string const& key_prefix)
          : member_base (var, &t, fq_type, key_prefix)
      {
      }
    };

    struct image_base: traversal::class_, virtual context
    {
      typedef image_base base;

      image_base (): first_ (true) {}

      virtual void
      traverse (type& c)
      {
        bool obj (c.count ("object"));

        // Ignore transient bases.
        //
        if (!(obj || comp_value (c)))
          return;

        if (first_)
        {
          os << ": ";
          first_ = false;
        }
        else
        {
          os << "," << endl
             << "  ";
        }

        if (obj)
          os << "object_traits< " << c.fq_name () << " >::image_type";
        else
          os << "composite_value_traits< " << c.fq_name () << " >::image_type";
      }

    private:
      bool first_;
    };

    struct image_type: traversal::class_, virtual context
    {
      typedef image_type base;

      image_type ()
      {
        *this >> names_member_ >> member_;
      }

      image_type (image_type const&)
          : root_context (), context () //@@ -Wextra
      {
        *this >> names_member_ >> member_;
      }

      virtual void
      traverse (type& c)
      {
        os << "struct image_type";

        {
          instance<image_base> b;
          traversal::inherits i (*b);
          inherits (c, i);
        }

        os << "{";

        names (c);

        if (!comp_value (c))
          os << "std::size_t version;";

        os << "};";
      }

    private:
      instance<image_member> member_;
      traversal::names names_member_;
    };

    //
    // query_type
    //

    struct query_base: traversal::class_, virtual context
    {
      typedef query_base base;

      query_base (): first_ (true) {}

      virtual void
      traverse (type& c)
      {
        // Ignore transient bases.
        //
        if (!c.count ("object"))
          return;

        if (first_)
        {
          os << ": ";
          first_ = false;
        }
        else
        {
          os << "," << endl
             << "  ";
        }

        os << "object_traits< " << c.fq_name () << " >::query_columns";
      }

    private:
      bool first_;
    };

    struct query_type: traversal::class_, virtual context
    {
      typedef query_type base;

      virtual void
      traverse (type& c)
      {
        os << "struct query_columns";

        {
          instance<query_base> b;
          traversal::inherits i (*b);
          inherits (c, i);
        }

        os << "{";

        {
          instance<query_columns> t;
          t->traverse (c);
        }

        os << "};";
      }
    };

    // Member-specific traits types for container members.
    //
    struct container_traits: object_members_base, virtual context
    {
      typedef container_traits base;

      container_traits (semantics::class_& obj) //@@ context::object
          : object_members_base (true, false)
      {
        scope_ = "object_traits< " + obj.fq_name () + " >";
      }

      virtual void
      container (semantics::data_member& m)
      {
        using semantics::type;
        using semantics::class_;

        type& t (m.type ());
        container_kind_type ck (container_kind (t));

        type& vt (container_vt (t));
        type* it (0);
        type* kt (0);

        bool ordered (false);
        bool inverse (context::inverse (m, "value"));

        switch (ck)
        {
        case ck_ordered:
          {
            if (!unordered (m))
            {
              it = &container_it (t);
              ordered = true;
            }
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            kt = &container_kt (t);
            break;
          }
        case ck_set:
        case ck_multiset:
          {
            break;
          }
        }

        string name (prefix_ + public_name (m) + "_traits");

        // Figure out column counts.
        //
        size_t data_columns (1), cond_columns (1); // One for object id.

        switch (ck)
        {
        case ck_ordered:
          {
            // Add one for the index.
            //
            if (ordered)
            {
              data_columns++;

              // Index is not currently used (see also bind()).
              //
              // cond_columns++;
            }
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            // Add some for the key.
            //
            size_t n;

            if (class_* kc = comp_value (*kt))
              n = in_column_count (*kc);
            else
              n = 1;

            data_columns += n;

            // Key is not currently used (see also bind()).
            //
            // cond_columns += n;

            break;
          }
        case ck_set:
        case ck_multiset:
          {
            // Not currently used (see also bind())
            //
            // Value is also a key.
            //
            //if (class_* vc = comp_value (vt))
            //  cond_columns += in_column_count (*vc);
            //else
            //  cond_columns++;

            break;
          }
        }

        if (class_* vc = comp_value (vt))
          data_columns += in_column_count (*vc);
        else
          data_columns++;

        // Store column counts for the source generator.
        //
        m.set ("cond-column-count", cond_columns);
        m.set ("data-column-count", data_columns);

        os << "// " << m.name () << endl
           << "//" << endl
           << "struct " << name
           << "{";

        // container_type
        // container_traits
        // index_type
        // key_type
        // value_type
        //

        os << "typedef " << t.fq_name (m.belongs ().hint ()) <<
          " container_type;";
        os << "typedef odb::access::container_traits< container_type > " <<
          "container_traits;";

        switch (ck)
        {
        case ck_ordered:
          {
            os << "typedef container_traits::index_type index_type;";
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "typedef container_traits::key_type key_type;";
          }
        case ck_set:
        case ck_multiset:
          {
            break;
          }
        }

        os << "typedef container_traits::value_type value_type;"
           << endl;

        // functions_type
        //
        switch (ck)
        {
        case ck_ordered:
          {
            os << "typedef ordered_functions<index_type, value_type> " <<
              "functions_type;";
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "typedef map_functions<key_type, value_type> " <<
              "functions_type;";
            break;
          }
        case ck_set:
        case ck_multiset:
          {
            os << "typedef set_functions<value_type> functions_type;";
            break;
          }
        }

        os << "typedef " << db << "::container_statements< " << name <<
          " > statements_type;"
           << endl;

        // column_count
        //
        os << "static const std::size_t cond_column_count = " <<
          cond_columns << "UL;"
           << "static const std::size_t data_column_count = " <<
          data_columns << "UL;"
           << endl;

        // id_image_type
        //
        os << "typedef " << scope_ << "::id_image_type id_image_type;"
           << endl;

        // cond_image_type (object id is taken from the object image)
        //
        os << "struct cond_image_type"
           << "{";

        switch (ck)
        {
        case ck_ordered:
          {
            if (ordered)
            {
              os << "// index" << endl
                 << "//" << endl;
              instance<image_member> im ("index_", *it, "index_type", "index");
              im->traverse (m);
            }
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "// key" << endl
               << "//" << endl;
            instance<image_member> im ("key_", *kt, "key_type", "key");
            im->traverse (m);
            break;
          }
        case ck_set:
        case ck_multiset:
          {
            os << "// value" << endl
               << "//" << endl;
            instance<image_member> im ("value_", vt, "value_type", "value");
            im->traverse (m);
            break;
          }
        }

        os << "std::size_t version;"
           << "};";

        // data_image_type (object id is taken from the object image)
        //
        os << "struct data_image_type"
           << "{";

        switch (ck)
        {
        case ck_ordered:
          {
            if (ordered)
            {
              os << "// index" << endl
                 << "//" << endl;
              instance<image_member> im ("index_", *it, "index_type", "index");
              im->traverse (m);
            }
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "// key" << endl
               << "//" << endl;
            instance<image_member> im ("key_", *kt, "key_type", "key");
            im->traverse (m);
            break;
          }
        case ck_set:
        case ck_multiset:
          {
            break;
          }
        }

        os << "// value" << endl
           << "//" << endl;
        instance<image_member> im ("value_", vt, "value_type", "value");
        im->traverse (m);

        os << "std::size_t version;"
           << "};";

        // Statements.
        //
        os << "static const char* const insert_one_statement;"
           << "static const char* const select_all_statement;"
           << "static const char* const delete_all_statement;"
           << endl;

        // bind (cond_image)
        //
        os << "static void" << endl
           << "bind (" << bind_vector << ", id_image_type*, cond_image_type&);"
           << endl;

        // bind (data_image)
        //
        os << "static void" << endl
           << "bind (" << bind_vector << ", id_image_type*, data_image_type&);"
           << endl;

        // grow ()
        //
        os << "static void" << endl
           << "grow (data_image_type&, " << truncated_vector << ");"
           << endl;

        // init (data_image)
        //
        if (!inverse)
        {
          os << "static void" << endl;

          switch (ck)
          {
          case ck_ordered:
            {
              if (ordered)
                os << "init (data_image_type&, index_type, const value_type&);";
              else
                os << "init (data_image_type&, const value_type&);";
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              os << "init (data_image_type&, const key_type&, const value_type&);";
              break;
            }
          case ck_set:
          case ck_multiset:
            {
              os << "init (data_image_type&, const value_type&);";
              break;
            }
          }

          os << endl;
        }

        // init (data)
        //
        os << "static void" << endl;

        switch (ck)
        {
        case ck_ordered:
          {
            if (ordered)
              os << "init (index_type&, value_type&, ";
            else
              os << "init (value_type&, ";
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "init (key_type&, value_type&, ";
            break;
          }
        case ck_set:
        case ck_multiset:
          {
            os << "init (value_type&, ";
            break;
          }
        }

        os << "const data_image_type&, database&);"
           << endl;

        // insert_one
        //
        os << "static void" << endl;

        switch (ck)
        {
        case ck_ordered:
          {
            os << "insert_one (index_type, const value_type&, void*);";
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "insert_one (const key_type&, const value_type&, void*);";
            break;
          }
        case ck_set:
        case ck_multiset:
          {
            os << "insert_one (const value_type&, void*);";
            break;
          }
        }

        os << endl;

        // load_all
        //
        os << "static bool" << endl;

        switch (ck)
        {
        case ck_ordered:
          {
            os << "load_all (index_type&, value_type&, void*);";
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "load_all (key_type&, value_type&, void*);";
            break;
          }
        case ck_set:
        case ck_multiset:
          {
            os << "load_all (value_type&, void*);";
            break;
          }
        }

        os << endl;

        // delete_all
        //
        os << "static void" << endl
           << "delete_all (void*);"
           << endl;

        // persist
        //
        if (!inverse)
          os << "static void" << endl
             << "persist (const container_type&," << endl
             << "id_image_type&," << endl
             << "statements_type&);"
             << endl;

        // load
        //
        os << "static void" << endl
           << "load (container_type&," << endl
           << "id_image_type&," << endl
           << "statements_type&);"
           << endl;

        // update
        //
        if (!inverse)
          os << "static void" << endl
             << "update (const container_type&," << endl
             << "id_image_type&," << endl
             << "statements_type&);"
             << endl;

        // erase
        //
        if (!inverse)
          os << "static void" << endl
             << "erase (id_image_type&, statements_type&);"
             << endl;

        os << "};";
      }

    private:
      string scope_;
    };

    //
    //
    struct class_: traversal::class_, virtual context
    {
      typedef class_ base;

      class_ (): id_image_member_ ("id_") {}

      class_ (class_ const&)
          : root_context (), //@@ -Wextra
            context (),
            id_image_member_ ("id_")
      {
      }

      virtual void
      traverse (type& c)
      {
        if (c.file () != unit.file ())
          return;

        if (c.count ("object"))
          traverse_object (c);
        else if (comp_value (c))
          traverse_value (c);
      }

      virtual void
      traverse_object (type& c)
      {
        string const& type (c.fq_name ());

        semantics::data_member& id (id_member (c));
        bool auto_id (id.count ("auto"));
        bool base_id (&id.scope () != &c); // Id comes from a base class.

        os << "// " << c.name () << endl
           << "//" << endl;

        os << "template <>" << endl
           << "class access::object_traits< " << type << " >"
           << "{"
           << "public:" << endl;

        // object_type & pointer_type
        //
        os << "typedef " << type << " object_type;"
           << "typedef " << c.get<string> ("object-pointer") << " pointer_type;";

        // id_type & id_image_type
        //
        if (base_id)
        {
          string const& base (id.scope ().fq_name ());

          os << "typedef object_traits< " << base << " >::id_type id_type;"
             << endl
             << "typedef object_traits< " << base << " >::id_image_type " <<
            "id_image_type;"
             << endl;
        }
        else
        {
          os << "typedef " << id.type ().fq_name (id.belongs ().hint ()) <<
            " id_type;"
             << endl;

          os << "struct id_image_type"
             << "{";

          id_image_member_->traverse (id);

          os << "std::size_t version;"
             << "};";
        }

        // image_type
        //
        image_type_->traverse (c);

        // query types
        //
        if (options.generate_query ())
        {
          // query_columns
          //
          query_type_->traverse (c);

          // query_base_type
          //
          os << "typedef " << db << "::query query_base_type;"
             << endl;

          // query_type
          //
          os << "struct query_type: query_base_type, query_columns"
             << "{"
             << "query_type ();"
             << "query_type (const std::string&);"
             << "query_type (const query_base_type&);"
             << "};";
        }

        // column_count
        //
        os << "static const std::size_t in_column_count = " <<
          in_column_count (c) << "UL;"
           << "static const std::size_t out_column_count = " <<
          out_column_count (c) << "UL;"
           << endl;

        // Statements.
        //
        os << "static const char* const persist_statement;"
           << "static const char* const find_statement;"
           << "static const char* const update_statement;"
           << "static const char* const erase_statement;";

        if (options.generate_query ())
          os << "static const char* const query_clause;";

        os << endl;

        //
        // Containers.
        //

        // Traits types.
        //
        {
          instance<container_traits> t (c);
          t->traverse (c);
        }

        // Statement cache (forward declaration).
        //
        os << "struct container_statement_cache_type;"
           << endl;

        //
        // Functions.
        //

        // id ()
        //
        os << "static id_type" << endl
           << "id (const object_type&);"
           << endl;

        if (options.generate_query ())
          os << "static id_type" << endl
             << "id (const image_type&);"
             << endl;

        // grow ()
        //
        os << "static bool" << endl
           << "grow (image_type&, " << truncated_vector << ");"
           << endl;

        // bind (image_type)
        //
        os << "static void" << endl
           << "bind (" << bind_vector << ", image_type&, bool);"
           << endl;

        // bind (id_image_type)
        //
        os << "static void" << endl
           << "bind (" << bind_vector << ", id_image_type&);"
           << endl;

        // init (image, object)
        //
        os << "static bool" << endl
           << "init (image_type&, const object_type&);"
           << endl;

        // init (object, image)
        //
        os << "static void" << endl
           << "init (object_type&, const image_type&, database&);"
           << endl;

        // init (id_image, id)
        //
        os << "static void" << endl
           << "init (id_image_type&, const id_type&);"
           << endl;

        // persist ()
        //
        os << "static void" << endl
           << "persist (database&, " << (auto_id ? "" : "const ") <<
          "object_type&);"
           << endl;

        // update ()
        //
        os << "static void" << endl
           << "update (database&, const object_type&);"
           << endl;

        // erase ()
        //
        os << "static void" << endl
           << "erase (database&, const id_type&);"
           << endl;

        // find ()
        //
        if (c.default_ctor ())
          os << "static pointer_type" << endl
             << "find (database&, const id_type&);"
             << endl;

        os << "static bool" << endl
           << "find (database&, const id_type&, object_type&);"
           << endl;

        // query ()
        //
        if (options.generate_query ())
          os << "template<typename T>" << endl
             << "static result<T>" << endl
             << "query (database&, const query_type&);"
             << endl;

        // create_schema ()
        //
        if (embedded_schema)
        {
          os << "static void" << endl
             << "create_schema (database&);"
             << endl;
        }

        // Implementation details.
        //
        os << "public:" << endl;

        // Load the object image.
        //
        os << "static bool" << endl
           << "find_ (" << db << "::object_statements< object_type >&, " <<
          "const id_type&);"
           << endl;

        // Load the rest of the object (containers, etc). Expects the id
        // image in the object statements to be initialized to the object
        // id.
        //
        os << "static void" << endl
           << "load_ (" << db << "::object_statements< object_type >&, " <<
          "object_type&);"
           << endl;

        if (options.generate_query ())
          os << "static void" << endl
             << "query_ (database&," << endl
             << "const query_type&," << endl
             << db << "::object_statements< object_type >&," << endl
             << "details::shared_ptr< " << db << "::select_statement >&);"
             << endl;

        os << "};";
      }

      virtual void
      traverse_value (type& c)
      {
        string const& type (c.fq_name ());

        os << "// " << c.name () << endl
           << "//" << endl;

        os << "template <>" << endl
           << "class access::composite_value_traits< " << type << " >"
           << "{"
           << "public:" << endl;

        // object_type
        //
        os << "typedef " << type << " value_type;"
           << endl;

        // image_type
        //
        image_type_->traverse (c);

        // grow ()
        //
        os << "static bool" << endl
           << "grow (image_type&, " << truncated_vector << ");"
           << endl;

        // bind (image_type)
        //
        os << "static void" << endl
           << "bind (" << bind_vector << ", image_type&);"
           << endl;

        // init (image, object)
        //
        os << "static bool" << endl
           << "init (image_type&, const value_type&);"
           << endl;

        // init (object, image)
        //
        os << "static void" << endl
           << "init (value_type&, const image_type&, database&);"
           << endl;

        os << "};";
      }

    private:
      instance<image_type> image_type_;
      instance<query_type> query_type_;
      instance<image_member> id_image_member_;
    };

    struct include: virtual context
    {
      typedef include base;

      virtual void
      generate ()
      {
        os << "#include <odb/details/buffer.hxx>" << endl
           << endl;

        os << "#include <odb/" << db << "/version.hxx>" << endl
           << "#include <odb/" << db << "/forward.hxx>" << endl
           << "#include <odb/" << db << "/" << db << "-types.hxx>" << endl;

        if (options.generate_query ())
          os << "#include <odb/" << db << "/query.hxx>" << endl;

        os << endl;
      }
    };
  }
}

#endif // ODB_RELATIONAL_HEADER_HXX