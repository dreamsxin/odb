// file      : odb/semantics/unit.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_UNIT_HXX
#define ODB_SEMANTICS_UNIT_HXX

#include <map>

#include <semantics/elements.hxx>
#include <semantics/namespace.hxx>

namespace semantics
{
  class unit: public graph<node, edge>, public namespace_
  {
  public:
    unit (path const& file)
        : node (file, 1, 1), graph_ (*this)
    {
      // Use a special edge to get this->name() return the global
      // namespace name ("").
      //
      new_edge<global_names> (*this, *this);
    }

    // Mapping from GCC tree node to semantic graph node.
    //
  public:
    node*
    find (gcc_tree key) const
    {
      gcc_tree_node_map::const_iterator i (gcc_tree_node_map_.find (key));
      return i != gcc_tree_node_map_.end () ? i->second : 0;
    }

    void
    insert (gcc_tree key, node& value)
    {
      gcc_tree_node_map_[key] = &value;
    }

  public:
    template <typename T>
    T&
    new_node (path const& file, size_t line, size_t column)
    {
      return graph_.new_node<T> (file, line, column);
    }

    template <typename T, typename A0>
    T&
    new_node (path const& file, size_t line, size_t column, A0 const& a0)
    {
      return graph_.new_node<T> (file, line, column, a0);
    }

    template <typename T, typename A0, typename A1>
    T&
    new_node (path const& file, size_t line, size_t column,
              A0 const& a0, A1 const& a1)
    {
      return graph_.new_node<T> (file, line, column, a0, a1);
    }

    template <typename T, typename A0, typename A1, typename A2>
    T&
    new_node (path const& file, size_t line, size_t column,
              A0 const& a0, A1 const& a1, A2 const& a2)
    {
      return graph_.new_node<T> (file, line, column, a0, a1, a2);
    }

    template <typename T, typename A0, typename A1, typename A2, typename A3>
    T&
    new_node (path const& file, size_t line, size_t column,
              A0 const& a0, A1 const& a1, A2 const& a2, A3 const& a3)
    {
      return graph_.new_node<T> (file, line, column, a0, a1, a2, a3);
    }

    // For fundamental types.
    //
    template <typename T>
    T&
    new_fund_node ()
    {
      return graph_.new_node<T> ();
    }

  protected:
    // Special names edge for the global namespace.
    //
    class global_names: public names
    {
    public:
      global_names ()
          : names ("")
      {
        scope_ = 0;
      }

      void
      set_left_node (unit&)
      {
      }

      void
      set_right_node (nameable& n)
      {
        named_ = &n;
      }

      void
      clear_left_node (unit&)
      {
      }

      void
      clear_right_node (nameable& n)
      {
        assert (named_ == &n);
        named_ = 0;
      }
    };

  public:
    void
    add_edge_left (global_names&)
    {
    }

  private:
    graph<node, edge>& graph_;

    typedef std::map<gcc_tree, node*> gcc_tree_node_map;
    gcc_tree_node_map gcc_tree_node_map_;
  };
}

#endif // ODB_SEMANTICS_UNIT_HXX