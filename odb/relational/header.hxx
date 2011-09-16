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
        bool obj (object (c));

        // Ignore transient bases. Not used for views.
        //
        if (!(obj || composite (c)))
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

        if (!view (c))
        {
          instance<image_base> b;
          traversal::inherits i (*b);
          inherits (c, i);
        }

        os << "{";

        names (c);

        if (!composite (c))
          os << "std::size_t version;";

        os << "};";
      }

    private:
      instance<image_member> member_;
      traversal::names names_member_;
    };

    //
    // query_columns_type
    //

    struct query_columns_bases: traversal::class_, virtual context
    {
      typedef query_columns_bases base;

      query_columns_bases (bool ptr, bool first = true)
          : ptr_ (ptr), first_ (first)
      {
      }

      virtual void
      traverse (type& c)
      {
        // Ignore transient bases. Not used for views.
        //
        if (!object (c))
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

        os  << (ptr_ ? "pointer_query_columns" : "query_columns") <<
          "< " << c.fq_name () << ", table >";
      }

    private:
      bool ptr_;
      bool first_;
    };

    struct query_columns_base_aliases: traversal::class_, virtual context
    {
      typedef query_columns_base_aliases base;

      query_columns_base_aliases (bool ptr)
          : ptr_ (ptr)
      {
      }

      virtual void
      traverse (type& c)
      {
        // Ignore transient bases. Not used for views.
        //
        if (!object (c))
          return;

        os  << "// " << c.name () << endl
            << "//" << endl
          << "typedef " <<
          (ptr_ ? "pointer_query_columns" : "query_columns") <<
          "< " << c.fq_name () << ", table > " << c.name () << ";"
            << endl;
      }

    private:
      bool ptr_;
    };

    struct query_columns_type: traversal::class_, virtual context
    {
      typedef query_columns_type base;

      // Depending on the ptr argument, generate query_columns or
      // pointer_query_columns specialization. The latter is used
      // for object pointers where we don't support nested pointers.
      //
      query_columns_type (bool ptr): ptr_ (ptr) {}

      virtual void
      traverse (type& c)
      {
        string const& type (c.fq_name ());

        if (ptr_)
        {
          os << "template <const char* table>" << endl
             << "struct pointer_query_columns< " << type << ", table >";

          // If we don't have pointers (in the whole hierarchy), then
          // pointer_query_columns and query_columns are the same.
          //
          if (!has_a (c, test_pointer))
          {
            os << ":" << endl
               << "  query_columns< " << type << ", table >"
               << "{"
               << "};";
          }
          else
          {
            {
              instance<query_columns_bases> b (ptr_);
              traversal::inherits i (*b);
              inherits (c, i);
            }

            os << "{";

            {
              instance<query_columns_base_aliases> b (ptr_);
              traversal::inherits i (*b);
              inherits (c, i);
            }

            {
              instance<query_columns> t (ptr_);
              t->traverse (c);
            }

            os << "};";

            {
              instance<query_columns> t (ptr_, c);
              t->traverse (c);
            }
          }
        }
        else
        {
          bool has_ptr (has_a (c, test_pointer | exclude_base));

          if (has_ptr)
          {
            os << "template <>" << endl
               << "struct query_columns_base< " << type << " >"
               << "{";

            instance<query_columns_base> t;
            t->traverse (c);

            os << "};";
          }

          os << "template <const char* table>" << endl
             << "struct query_columns< " << type << ", table >";

          if (has_ptr)
            os << ":" << endl
               << "  query_columns_base< " << type << " >";

          {
            instance<query_columns_bases> b (ptr_, !has_ptr);
            traversal::inherits i (*b);
            inherits (c, i);
          }

          os << "{";

          {
            instance<query_columns_base_aliases> b (ptr_);
            traversal::inherits i (*b);
            inherits (c, i);
          }

          {
            instance<query_columns> t (ptr_);
            t->traverse (c);
          }

          os << "};";

          {
            instance<query_columns> t (ptr_, c);
            t->traverse (c);
          }
        }
      }

    public:
      bool ptr_;
    };

    // Member-specific traits types for container members.
    //
    struct container_traits: object_members_base, virtual context
    {
      typedef container_traits base;

      container_traits (semantics::class_& c)
          : object_members_base (true, false, false), c_ (c)
      {
      }

      virtual void
      traverse_composite (semantics::data_member* m, semantics::class_& c)
      {
        if (object (c_))
          object_members_base::traverse_composite (m, c);
        else
        {
          // If we are generating traits for a composite value type, then
          // we don't want to go into its bases or it composite members.
          //
          if (m == 0 && &c == &c_)
            names (c);
        }
      }

      virtual void
      container_public_extra_pre (semantics::data_member&)
      {
      }

      virtual void
      container_public_extra_post (semantics::data_member&)
      {
      }

      virtual void
      traverse_container (semantics::data_member& m, semantics::type& c)
      {
        using semantics::type;
        using semantics::class_;

        // Figure out if this member is from a base object or composite
        // value and whether it is abstract.
        //
        bool base, abst;

        if (object (c_))
        {
          base = cur_object != &c_ ||
            !object (dynamic_cast<type&> (m.scope ()));
          abst = abstract (c_);
        }
        else
        {
          base = false; // We don't go into bases.
          abst = true;  // Always abstract.
        }

        container_kind_type ck (container_kind (c));

        type& vt (container_vt (c));
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
              it = &container_it (c);
              ordered = true;
            }
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            kt = &container_kt (c);
            break;
          }
        case ck_set:
        case ck_multiset:
          {
            break;
          }
        }

        string name (flat_prefix_ + public_name (m) + "_traits");

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

            if (class_* kc = composite_wrapper (*kt))
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
            //if (class_* vc = composite_wrapper (vt))
            //  cond_columns += in_column_count (*vc);
            //else
            //  cond_columns++;

            break;
          }
        }

        if (class_* vc = composite_wrapper (vt))
          data_columns += in_column_count (*vc);
        else
          data_columns++;

        // Store column counts for the source generator.
        //
        m.set ("cond-column-count", cond_columns);
        m.set ("data-column-count", data_columns);

        os << "// " << m.name () << endl
           << "//" << endl
           << "struct " << name;

        if (base)
        {
          semantics::class_& b (dynamic_cast<semantics::class_&> (m.scope ()));

          if (object (b))
            os << ": access::object_traits< " << b.fq_name () << " >::" <<
              name;
          else
            os << ": access::composite_value_traits< " << b.fq_name () <<
              " >::" << public_name (m) << "_traits"; // No prefix_.
        }

        os << "{";

        container_public_extra_pre (m);

        if (!abst)
        {
          // column_count
          //
          os << "static const std::size_t cond_column_count = " <<
            cond_columns << "UL;"
             << "static const std::size_t data_column_count = " <<
            data_columns << "UL;"
             << endl;

          // Statements.
          //
          os << "static const char insert_one_statement[];"
             << "static const char select_all_statement[];"
             << "static const char delete_all_statement[];"
             << endl;
        }

        if (base)
        {
          container_public_extra_post (m);
          os << "};";

          return;
        }

        // container_type
        // container_traits
        // index_type
        // key_type
        // value_type
        //
        os << "typedef ";

        {
          semantics::type& t (m.type ());

          if (wrapper (t))
            // Use the hint from the wrapper.
            //
            os << c.fq_name (t.get<semantics::names*> ("wrapper-hint"));
          else
            // t and c are the same.
            //
            os << c.fq_name (m.belongs ().hint ());
        }

        os << " container_type;";

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

        // bind (cond_image)
        //
        os << "static void" << endl
           << "bind (" << bind_vector << "," << endl
           << "const " << bind_vector << " id," << endl
           << "std::size_t id_size," << endl
           << "cond_image_type&);"
           << endl;

        // bind (data_image)
        //
        os << "static void" << endl
           << "bind (" << bind_vector << "," << endl
           << "const " << bind_vector << " id," << endl
           << "std::size_t id_size," << endl
           << "data_image_type&);"
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
             << "const " << db << "::binding& id," << endl
             << "statements_type&);"
             << endl;

        // load
        //
        os << "static void" << endl
           << "load (container_type&," << endl
           << "const " << db << "::binding& id," << endl
           << "statements_type&);"
           << endl;

        // update
        //
        if (!inverse)
          os << "static void" << endl
             << "update (const container_type&," << endl
             << "const " << db << "::binding& id," << endl
             << "statements_type&);"
             << endl;

        // erase
        //
        if (!inverse)
          os << "static void" << endl
             << "erase (const " << db << "::binding& id, statements_type&);"
             << endl;

        container_public_extra_post (m);

        os << "};";
      }

    protected:
      semantics::class_& c_;
    };

    // First pass over objects, views, and composites. Some code must be
    // split into two parts to deal with yet undefined types.
    //
    struct class1: traversal::class_, virtual context
    {
      typedef class1 base;

      class1 ()
          : id_image_member_ ("id_"),
            query_columns_type_ (false),
            pointer_query_columns_type_ (true)
      {
      }

      class1 (class_ const&)
          : root_context (), //@@ -Wextra
            context (),
            id_image_member_ ("id_"),
            query_columns_type_ (false),
            pointer_query_columns_type_ (true)
      {
      }

      virtual void
      traverse (type& c)
      {
        if (c.file () != unit.file ())
          return;

        if (object (c))
          traverse_object (c);
        else if (view (c))
          traverse_view (c);
        else if (composite (c))
          traverse_composite (c);
      }

      virtual void
      object_public_extra_pre (type&)
      {
      }

      virtual void
      object_public_extra_post (type&)
      {
      }

      virtual void
      traverse_object (type& c)
      {
        string const& type (c.fq_name ());

        semantics::data_member* id (id_member (c));
        bool auto_id (id ? id->count ("auto") : false);
        bool base_id (id ? &id->scope () != &c : false); // Comes from base.

        os << "// " << c.name () << endl
           << "//" << endl;

        // class_traits
        //
        os << "template <>" << endl
           << "struct class_traits< " << type << " >"
           << "{"
           << "static const class_kind kind = class_object;"
           << "};";

        // pointer_query_columns & query_columns
        //
        if (options.generate_query ())
        {
          // If we don't have object pointers, then also generate
          // query_columns (in this case pointer_query_columns and
          // query_columns are the same and the former inherits from
          // the latter). Otherwise we have to postpone query_columns
          // generation until the second pass to deal with forward-
          // declared objects.
          //
          if (!has_a (c, test_pointer))
            query_columns_type_->traverse (c);

          pointer_query_columns_type_->traverse (c);
        }

        // object_traits
        //
        os << "template <>" << endl
           << "class access::object_traits< " << type << " >"
           << "{"
           << "public:" << endl;

        object_public_extra_pre (c);

        // object_type & pointer_type
        //
        os << "typedef " << type << " object_type;"
           << "typedef " << c.get<string> ("object-pointer") << " pointer_type;";

        // id_type & id_image_type
        //
        if (id != 0)
        {
          if (base_id)
          {
            string const& base (id->scope ().fq_name ());

            os << "typedef object_traits< " << base << " >::id_type id_type;"
               << endl
               << "typedef object_traits< " << base << " >::id_image_type " <<
              "id_image_type;"
               << endl;
          }
          else
          {
            os << "typedef " << id->type ().fq_name (id->belongs ().hint ()) <<
              " id_type;"
               << endl;

            os << "struct id_image_type"
               << "{";

            id_image_member_->traverse (*id);

            os << "std::size_t version;"
               << "};";
          }
        }

        // image_type
        //
        image_type_->traverse (c);

        //
        // Containers (abstract and concrete).
        //

        {
          instance<container_traits> t (c);
          t->traverse (c);
        }

        //
        // Functions (abstract and concrete).
        //

        // id ()
        //
        if (id != 0)
        {
          os << "static id_type" << endl
             << "id (const object_type&);"
             << endl;

          if (options.generate_query ())
            os << "static id_type" << endl
               << "id (const image_type&);"
               << endl;
        }

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
        if (id != 0)
        {
          os << "static void" << endl
             << "bind (" << bind_vector << ", id_image_type&);"
             << endl;
        }

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
        if (id != 0)
        {
          os << "static void" << endl
             << "init (id_image_type&, const id_type&);"
             << endl;
        }

        //
        // The rest only applies to concrete objects.
        //
        if (abstract (c))
        {
          object_public_extra_post (c);
          os << "};";
          return;
        }

        //
        // Query (concrete).
        //

        if (options.generate_query ())
        {
          // query_base_type
          //
          os << "typedef " << db << "::query query_base_type;"
             << endl;

          // query_type
          //
          os << "struct query_type;";
        }

        //
        // Containers (concrete).
        //

        // Statement cache (forward declaration).
        //
        os << "struct container_statement_cache_type;"
           << endl;

        // column_count
        //
        os << "static const std::size_t in_column_count = " <<
          in_column_count (c) << "UL;"
           << "static const std::size_t out_column_count = " <<
          out_column_count (c) << "UL;"
           << endl;

        // Statements.
        //
        os << "static const char persist_statement[];"
           << "static const char find_statement[];"
           << "static const char update_statement[];"
           << "static const char erase_statement[];";

        if (options.generate_query ())
        {
          os << "static const char query_clause[];"
             << "static const char erase_query_clause[];"
             << endl
             << "static const char table_name[];";
        }

        os << endl;

        //
        // Functions (concrete).
        //

        // callback ()
        //
        os << "static void" << endl
           << "callback (database&, object_type&, callback_event);"
           <<  endl;

        os << "static void" << endl
           << "callback (database&, const object_type&, callback_event);"
           <<  endl;

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
        {
          os << "template<typename T>" << endl
             << "static result<T>" << endl
             << "query (database&, const query_type&);"
             << endl;

          os << "static unsigned long long" << endl
             << "erase_query (database&, const query_type&);"
             << endl;
        }

        // create_schema ()
        //
        if (embedded_schema)
        {
          os << "static bool" << endl
             << "create_schema (database&, unsigned short pass, bool drop);"
             << endl;
        }

        object_public_extra_post (c);

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
      view_public_extra_pre (type&)
      {
      }

      virtual void
      view_public_extra_post (type&)
      {
      }

      virtual void
      traverse_view (type& c)
      {
        string const& type (c.fq_name ());

        os << "// " << c.name () << endl
           << "//" << endl;

        os << "template <>" << endl
           << "struct class_traits< " << type << " >"
           << "{"
           << "static const class_kind kind = class_view;"
           << "};";

        os << "template <>" << endl
           << "class access::view_traits< " << type << " >"
           << "{"
           << "public:" << endl;

        view_public_extra_pre (c);

        // view_type & pointer_type
        //
        os << "typedef " << type << " view_type;"
           << "typedef " << c.get<string> ("object-pointer") << " pointer_type;"
           << endl;

        // image_type
        //
        image_type_->traverse (c);

        //
        // Query.
        //

        // query_base_type
        //
        os << "typedef " << db << "::query query_base_type;"
           << endl;

        // query_type
        //
        if (c.count ("objects"))
        {
          view_objects& objs (c.get<view_objects> ("objects"));

          if (objs.size () > 1)
          {
            os << "struct query_columns"
               << "{";

            for (view_objects::const_iterator i (objs.begin ());
                 i < objs.end ();
                 ++i)
            {
              bool alias (!i->alias.empty ());
              semantics::class_& o (*i->object);
              string const& name (alias ? i->alias : o.name ());
              string const& type (o.fq_name ());

              os << "// " << name << endl
                 << "//" << endl;

              if (alias && i->alias != table_name (o))
                os << "static const char " << name << "_alias_[];"
                   << endl
                   << "typedef" << endl
                   << "odb::pointer_query_columns< " << type << ", " <<
                  name << "_alias_ >" << endl
                   << name << ";"
                   << endl;
              else
                os << "typedef" << endl
                   << "odb::pointer_query_columns<" << endl
                   << "  " << type << "," << endl
                   << "  " << "odb::access::object_traits< " << type <<
                  " >::table_name >" << endl
                   << name << ";"
                   << endl;
            }

            os << "};"
               << "struct query_type: query_base_type, query_columns"
               << "{";
          }
          else
          {
            // For a single object view we generate a shortcut without
            // an intermediate typedef.
            //
            view_object const& vo (objs[0]);

            bool alias (!vo.alias.empty ());
            semantics::class_& o (*vo.object);
            string const& type (o.fq_name ());

            if (alias && vo.alias != table_name (o))
              os << "static const char query_alias[];"
                 << endl
                 << "struct query_type:" << endl
                 << "  query_base_type," << endl
                 << "  odb::pointer_query_columns< " << type <<
                ", query_alias >"
                 << "{";
            else
              os << "struct query_type:" << endl
                 << "  query_base_type," << endl
                 << "  odb::pointer_query_columns<" << endl
                 << "    " << type << "," << endl
                 << "    odb::access::object_traits< " << type <<
                " >::table_name >"
                 << "{";
          }

          os << "query_type ();"
             << "query_type (bool);"
             << "query_type (const char*);"
             << "query_type (const std::string&);"
             << "query_type (const query_base_type&);"
             << "};";
        }
        else
          os << "typedef query_base_type query_type;"
             << endl;

        //
        // Functions.
        //

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

        // init (view, image)
        //
        os << "static void" << endl
           << "init (view_type&, const image_type&, database&);"
           << endl;

        // column_count
        //
        os << "static const std::size_t column_count = " <<
          out_column_count (c) << "UL;"
           << endl;

        // Statements.
        //
        view_query& vq (c.get<view_query> ("query"));

        if (vq.kind != view_query::runtime)
        {
          os << "static query_base_type" << endl
             << "query_statement (const query_base_type&);"
             << endl;
        }

        //
        // Functions.
        //

        // callback ()
        //
        os << "static void" << endl
           << "callback (database&, view_type&, callback_event);"
           <<  endl;

        // query ()
        //
        os << "static result<view_type>" << endl
           << "query (database&, const query_type&);"
           << endl;

        view_public_extra_post (c);

        os << "};";
      }

      virtual void
      traverse_composite (type& c)
      {
        string const& type (c.fq_name ());

        os << "// " << c.name () << endl
           << "//" << endl;

        os << "template <>" << endl
           << "struct class_traits< " << type << " >"
           << "{"
           << "static const class_kind kind = class_composite;"
           << "};";

        os << "template <>" << endl
           << "class access::composite_value_traits< " << type << " >"
           << "{"
           << "public:" << endl;

        // value_type
        //
        os << "typedef " << type << " value_type;"
           << endl;

        // image_type
        //
        image_type_->traverse (c);

        // Containers.
        //
        {
          instance<container_traits> t (c);
          t->traverse (c);
        }

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

        // init (image, value)
        //
        os << "static bool" << endl
           << "init (image_type&, const value_type&);"
           << endl;

        // init (value, image)
        //
        os << "static void" << endl
           << "init (value_type&, const image_type&, database&);"
           << endl;

        os << "};";
      }

    private:
      instance<image_type> image_type_;
      instance<image_member> id_image_member_;

      instance<query_columns_type> query_columns_type_;
      instance<query_columns_type> pointer_query_columns_type_;
    };

    // Second pass over objects, views, and composites.
    //
    struct class2: traversal::class_, virtual context
    {
      typedef class2 base;

      class2 (): query_columns_type_ (false) {}

      class2 (class_ const&)
          : root_context (), //@@ -Wextra
            context (),
            query_columns_type_ (false)
      {
      }

      virtual void
      traverse (type& c)
      {
        if (c.file () != unit.file ())
          return;

        if (object (c))
          traverse_object (c);
        else if (view (c))
          traverse_view (c);
        else if (composite (c))
          traverse_composite (c);
      }

      virtual void
      traverse_object (type& c)
      {
        bool abst (abstract (c));
        string const& type (c.fq_name ());

        if (options.generate_query ())
        {
          bool has_ptr (has_a (c, test_pointer));

          if (has_ptr || !abst)
            os << "// " << c.name () << endl
               << "//" << endl;

          // query_columns
          //
          // If we don't have any pointers, then query_columns is generated
          // in pass 1 (see the comment in class1 for details).
          //
          if (has_ptr)
            query_columns_type_->traverse (c);

          // query_type
          //
          if (!abst)
            os << "struct access::object_traits< " << type << " >::" <<
              "query_type:" << endl
               << "  query_base_type," << endl
               << "  query_columns< " << type << ", table_name >"
               << "{"
               << "query_type ();"
               << "query_type (bool);"
               << "query_type (const char*);"
               << "query_type (const std::string&);"
               << "query_type (const query_base_type&);"
               << "};";
        }

        // Move header comment out of if-block if adding any code here.
      }

      virtual void
      traverse_view (type&)
      {
      }

      virtual void
      traverse_composite (type&)
      {
      }

    private:
      instance<query_columns_type> query_columns_type_;
    };

    struct include: virtual context
    {
      typedef include base;

      virtual void
      generate ()
      {
        os << "#include <odb/details/buffer.hxx>" << endl
           << "#include <odb/details/unused.hxx>" << endl
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
